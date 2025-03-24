/**
 * Copyright (c) 2023 Parrot Drones SAS
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of the Parrot Drones SAS Company nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE PARROT DRONES SAS COMPANY BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef _WIN32
#	include <winsock2.h>
#	include <windows.h>
#else /* !_WIN32 */
#	include <arpa/inet.h>
#	include <sys/mman.h>
#endif /* !_WIN32 */

#include <aac/aac.h>
#include <audio-decode/adec.h>
#include <audio-raw/araw.h>
#include <futils/futils.h>
#include <libpomp.h>
#include <media-buffers/mbuf_audio_frame.h>
#include <media-buffers/mbuf_mem_generic.h>
#define ULOG_TAG adec_prog
#include <ulog.h>
ULOG_DECLARE_TAG(adec_prog);

/* Win32 stubs */
#ifdef _WIN32
static inline const char *strsignal(int signum)
{
	return "??";
}
#endif /* _WIN32 */


#define DEFAULT_IN_BUF_COUNT 25
#define DEFAULT_TS_INC 33333
#define AAC_FRAME_LENGTH 1024


struct adec_prog {
	char *input_file;
#ifdef _WIN32
	HANDLE in_file;
	HANDLE in_file_map;
#else
	int in_fd;
#endif
	void *in_data;
	size_t in_len;
	size_t in_off;
	size_t in_frame_size;
	struct pomp_loop *loop;
	union {
		struct aac_reader *aac;
	} reader;
	struct adec_decoder *decoder;
	struct adec_config config;
	int configured;
	int finishing;
	int stopped;
	int last_in_frame;
	int first_out_frame;
	int input_finished;
	int output_finished;
	unsigned int input_count;
	unsigned int output_count;
	unsigned int frame_index;
	unsigned int start_index;
	unsigned int max_count;
	int adts_ready;
	struct aac_adts adts;
	size_t total_bytes;
	uint64_t ts_inc;
	struct adef_frame in_info;
	struct mbuf_pool *in_pool;
	int in_pool_allocated;
	struct mbuf_audio_frame_queue *in_queue;
	struct mbuf_mem *in_mem;
	struct mbuf_audio_frame *in_frame;
	struct araw_writer *writer;
	struct araw_writer_config writer_cfg;
	char *output_file;
	uint8_t *pending_frame;
	size_t pending_frame_len;
	struct aac_adts pending_frame_adts;
};


static struct adec_prog *s_self;
static int s_stopping;


static void aac_parse_idle(void *userdata);
static void pool_event_cb(struct pomp_evt *evt, void *userdata);


static void unmap_file(struct adec_prog *self)
{
#ifdef _WIN32
	if (self->in_data != NULL)
		UnmapViewOfFile(self->in_data);
	self->in_data = NULL;
	if (self->in_file_map != INVALID_HANDLE_VALUE)
		CloseHandle(self->in_file_map);
	self->in_file_map = INVALID_HANDLE_VALUE;
	if (self->in_file != INVALID_HANDLE_VALUE)
		CloseHandle(self->in_file);
	self->in_file = INVALID_HANDLE_VALUE;
#else
	if (self->in_fd >= 0) {
		if (self->in_data != NULL && self->in_len > 0)
			munmap(self->in_data, self->in_len);
		self->in_data = NULL;
		close(self->in_fd);
		self->in_fd = -1;
	}
#endif
}


static int map_file(struct adec_prog *self)
{
	int res;

#ifdef _WIN32
	BOOL ret;
	LARGE_INTEGER filesize;

	self->in_file = CreateFileA(self->input_file,
				    GENERIC_READ,
				    0,
				    NULL,
				    OPEN_EXISTING,
				    FILE_ATTRIBUTE_NORMAL,
				    NULL);
	if (self->in_file == INVALID_HANDLE_VALUE) {
		res = -EIO;
		ULOG_ERRNO("CreateFileA('%s')", -res, self->input_file);
		goto error;
	}

	self->in_file_map = CreateFileMapping(
		self->in_file, NULL, PAGE_READONLY, 0, 0, NULL);
	if (self->in_file_map == INVALID_HANDLE_VALUE) {
		res = -EIO;
		ULOG_ERRNO("CreateFileMapping('%s')", -res, self->input_file);
		goto error;
	}

	ret = GetFileSizeEx(self->in_file, &filesize);
	if (ret == FALSE) {
		res = -EIO;
		ULOG_ERRNO("GetFileSizeEx('%s')", -res, self->input_file);
		goto error;
	}
	self->in_len = filesize.QuadPart;

	self->in_data =
		MapViewOfFile(self->in_file_map, FILE_MAP_READ, 0, 0, 0);
	if (self->in_data == NULL) {
		res = -EIO;
		ULOG_ERRNO("MapViewOfFile('%s')", -res, self->input_file);
		goto error;
	}
#else
	off_t size;

	/* Try to open input file */
	self->in_fd = open(self->input_file, O_RDONLY);
	if (self->in_fd < 0) {
		res = -errno;
		ULOG_ERRNO("open('%s')", -res, self->input_file);
		goto error;
	}

	/* Get size and map it */
	size = lseek(self->in_fd, 0, SEEK_END);
	if (size < 0) {
		res = -errno;
		ULOG_ERRNO("lseek", -res);
		self->in_len = 0;
		goto error;
	}
	self->in_len = (size_t)size;

	self->in_data = mmap(
		NULL, self->in_len, PROT_READ, MAP_PRIVATE, self->in_fd, 0);
	if (self->in_data == MAP_FAILED) {
		res = -errno;
		ULOG_ERRNO("mmap", -res);
		goto error;
	}
#endif

	return 0;

error:
	unmap_file(self);
	return res;
}


static int configure(struct adec_prog *self)
{
	int res;

	switch (self->config.encoding) {
	case ADEF_ENCODING_AAC_LC:
		/* Set the input format */
		res = aac_adts_to_adef_format(&self->adts,
					      &self->in_info.format);
		if (res < 0) {
			ULOGE("unable to read ADTS header");
			return res;
		}
		/* As AAC frame length might slightly vary, frame size is
		 * arbitrary set to twice the first AAC frame length */
		self->in_frame_size = self->adts.aac_frame_length * 2;
		/* Configure the decoder */
		res = adec_set_aac_asc(self->decoder,
				       NULL,
				       0,
				       self->in_info.format.aac.data_format);
		if (res < 0) {
			ULOG_ERRNO("adec_set_aac_asc", -res);
			return res;
		}
		break;
	default:
		break;
	}

	if (!self->in_pool_allocated) {
		/* Input buffer pool */
		self->in_pool = adec_get_input_buffer_pool(self->decoder);
		if (self->in_pool == NULL) {
			res = mbuf_pool_new(mbuf_mem_generic_impl,
					    self->in_frame_size,
					    1,
					    MBUF_POOL_SMART_GROW,
					    DEFAULT_IN_BUF_COUNT,
					    "adec_default_pool",
					    &self->in_pool);
			if (res < 0) {
				ULOG_ERRNO("mbuf_pool_new:input", -res);
				return res;
			}
			self->in_pool_allocated = 1;
		}
	}

	/* Input buffer queue */
	self->in_queue = adec_get_input_buffer_queue(self->decoder);
	if (self->in_queue == NULL) {
		res = -EPROTO;
		ULOG_ERRNO("adec_get_input_buffer_queue", -res);
		return res;
	}

	self->configured = 1;
	return 0;
}


static int append_to_frame(struct adec_prog *self,
			   struct mbuf_audio_frame *frame,
			   struct mbuf_mem *mem,
			   const uint8_t *_data,
			   size_t len)
{
	int res;
	size_t capacity;
	uint8_t *frame_data, *data;

	ULOG_ERRNO_RETURN_ERR_IF(frame == NULL, EINVAL);
	ULOG_ERRNO_RETURN_ERR_IF(mem == NULL, EINVAL);
	ULOG_ERRNO_RETURN_ERR_IF(_data == NULL, EINVAL);
	ULOG_ERRNO_RETURN_ERR_IF(len == 0, EINVAL);

	res = mbuf_mem_get_data(mem, (void **)&frame_data, &capacity);
	if (res < 0) {
		ULOG_ERRNO("mbuf_mem_get_data", -res);
		return res;
	}
	if (capacity < len) {
		ULOGE("memory too small for frame");
		return -ENOBUFS;
	}
	if (frame_data == NULL) {
		ULOG_ERRNO("mbuf_mem_get_data", EPROTO);
		return -EPROTO;
	}
	data = frame_data;
	memcpy(data, _data, len);

	switch (self->in_info.format.encoding) {
	case ADEF_ENCODING_AAC_LC:
		/* nothing to do */
		break;
	default:
		ULOGE("unsupported encoding %s",
		      adef_encoding_to_str(self->in_info.format.encoding));
		return -EPROTO;
	}
	res = mbuf_audio_frame_set_buffer(frame, mem, 0, len);
	if (res < 0) {
		ULOG_ERRNO("mbuf_audio_frame_set_buffer", -res);
		return res;
	}

	return 0;
}


static void stop_reader(struct adec_prog *self)
{
	int res;
	switch (self->config.encoding) {
	case ADEF_ENCODING_AAC_LC:
		res = aac_reader_stop(self->reader.aac);
		if (res < 0)
			ULOG_ERRNO("aac_reader_stop", -res);
		break;
	default:
		break;
	}
}


static int decode_frame(struct adec_prog *self,
			const uint8_t *buf,
			size_t len,
			const struct aac_adts *adts)
{
	int res, err;
	size_t capacity;
	uint8_t *frame_data, *data;
	ssize_t frame_len;

	if (self->finishing) {
		stop_reader(self);
		return 0;
	}

	/* Configure the decoder */
	if (!self->configured && self->adts_ready) {
		res = configure(self);
		if (res < 0) {
			ULOG_ERRNO("configure", -res);
			return 0;
		}
	}

	/* Start decoding at start_index */
	if (self->frame_index < self->start_index)
		return 0;

	/* Stop decoding at max_count */
	if ((self->max_count > 0) && (self->input_count >= self->max_count))
		return 0;

	/* Get an input buffer (non-blocking) */
	if ((self->in_pool != NULL) && (self->in_mem == NULL)) {
		res = mbuf_pool_get(self->in_pool, &self->in_mem);
		if (res < 0) {
			if (res != -EAGAIN)
				ULOG_ERRNO("mbuf_pool_get:input", -res);

			/* Stop the parser */
			stop_reader(self);

			/* Copy data in a buffer */
			uint8_t *frame_bug = realloc(self->pending_frame, len);
			if (!frame_bug)
				return -ENOMEM;
			self->pending_frame = frame_bug;
			self->pending_frame_len = len;
			self->pending_frame_adts = *adts;
			memcpy(self->pending_frame, buf, len);
			return -EAGAIN;
		}
	}
	if (self->in_mem == NULL)
		return -EPROTO;

	/* Create the frame */
	res = mbuf_audio_frame_new(&self->in_info, &self->in_frame);
	if (res < 0) {
		ULOG_ERRNO("mbuf_audio_frame_new:input", -res);
		goto cleanup;
	}

	res = mbuf_mem_get_data(self->in_mem, (void **)&frame_data, &capacity);
	if (res < 0) {
		ULOG_ERRNO("mbuf_mem_get_data", -res);
		goto cleanup;
	}
	if (capacity < len) {
		res = -ENOBUFS;
		ULOGE("memory too small for frame");
		goto cleanup;
	}
	if (frame_data == NULL) {
		res = -EPROTO;
		ULOG_ERRNO("mbuf_mem_get_data", -res);
		goto cleanup;
	}
	data = frame_data;
	memcpy(data, buf, len);

	switch (self->in_info.format.encoding) {
	case ADEF_ENCODING_AAC_LC:
		/* nothing to do */
		break;
	default:
		res = -EPROTO;
		ULOGE("unsupported encoding %s",
		      adef_encoding_to_str(self->in_info.format.encoding));
		goto cleanup;
	}
	res = mbuf_audio_frame_set_buffer(self->in_frame, self->in_mem, 0, len);
	if (res < 0) {
		ULOG_ERRNO("mbuf_audio_frame_set_buffer", -res);
		goto cleanup;
	}

	res = mbuf_audio_frame_finalize(self->in_frame);
	if (res < 0) {
		ULOG_ERRNO("mbuf_audio_frame_finalize:input", -res);
		goto cleanup;
	}

	res = mbuf_audio_frame_queue_push(self->in_queue, self->in_frame);
	if (res < 0) {
		ULOG_ERRNO("mbuf_audio_frame_queue_push:input", -res);
		goto cleanup;
	}

	frame_len = mbuf_audio_frame_get_size(self->in_frame);
	if (frame_len < 0)
		ULOG_ERRNO("mbuf_audio_frame_get_size", (int)-frame_len);
	else
		self->total_bytes += frame_len;
	self->input_count++;
	self->output_finished = 0;

cleanup:
	err = mbuf_audio_frame_unref(self->in_frame);
	if (err < 0)
		ULOG_ERRNO("mbuf_audio_frame_unref:input", -err);
	self->in_frame = NULL;
	err = mbuf_mem_unref(self->in_mem);
	if (err < 0)
		ULOG_ERRNO("mbuf_mem_unref:input", -err);
	self->in_mem = NULL;
	self->in_info.info.index++;
	self->in_info.info.timestamp += self->ts_inc;

	return res;
}


static void adts_frame_begin_cb(struct aac_ctx *ctx,
				const uint8_t *buf,
				size_t len,
				const struct aac_adts *adts,
				void *userdata)
{
	struct adec_prog *self = userdata;
	if (!self->adts_ready) {
		self->adts = *adts;
		self->adts_ready = true;
	}
}


static void adts_frame_end_cb(struct aac_ctx *ctx,
			      const uint8_t *buf,
			      size_t len,
			      const struct aac_adts *adts,
			      void *userdata)
{
	struct adec_prog *self = userdata;
	int res = decode_frame(userdata, buf, len, adts);
	if (res < 0) {
		res = aac_reader_stop(self->reader.aac);
		if (res < 0)
			ULOG_ERRNO("aac_reader_stop", -res);
	}
	self->frame_index++;
}


static const struct aac_ctx_cbs aac_cbs = {
	.adts_frame_begin = &adts_frame_begin_cb,
	.adts_frame_end = &adts_frame_end_cb,
};


static int wav_output(struct adec_prog *self,
		      struct mbuf_audio_frame *out_frame)
{
	int res;
	struct araw_frame frame = {0};
	struct adef_frame info;
	const void *nalu_data;
	size_t nalu;
	const uint8_t *data;

	if (out_frame == NULL)
		return -EINVAL;

	if (self->output_file == NULL)
		return 0;

	res = mbuf_audio_frame_get_frame_info(out_frame, &info);
	if (res < 0) {
		ULOG_ERRNO("mbuf_audio_frame_get_frame_info", -res);
		return res;
	}

	if (self->writer == NULL) {
		/* Initialize the writer on first frame */
		self->writer_cfg.format = info.format;
		res = araw_writer_new(
			self->output_file, &self->writer_cfg, &self->writer);
		if (res < 0) {
			ULOG_ERRNO("araw_writer_new", -res);
			return res;
		}
		char *fmt = adef_format_to_str(&self->writer_cfg.format);
		ULOGI("WAV output file format is %s", fmt);
		free(fmt);
	}

	res = mbuf_audio_frame_get_buffer(out_frame, &nalu_data, &nalu);
	if (res < 0) {
		ULOG_ERRNO("mbuf_audio_frame_get_buffer", -res);
		goto out;
	}
	data = nalu_data;

	frame.frame = info;
	frame.cdata = data;
	frame.cdata_length = nalu;

	/* Write the frame */
	res = araw_writer_frame_write(self->writer, &frame);
	if (res < 0) {
		ULOG_ERRNO("araw_writer_frame_write", -res);
		goto out;
	}

out:
	if (frame.cdata != NULL) {
		int err =
			mbuf_audio_frame_release_buffer(out_frame, frame.cdata);
		if (err < 0)
			ULOG_ERRNO("mbuf_audio_frame_release_buffer", -err);
	}
	return res;
}


static uint64_t get_timestamp(struct mbuf_audio_frame *frame, const char *key)
{
	int res;
	struct mbuf_ancillary_data *data;
	uint64_t ts = 0;
	const void *raw_data;
	size_t len;

	res = mbuf_audio_frame_get_ancillary_data(frame, key, &data);
	if (res < 0)
		return 0;

	raw_data = mbuf_ancillary_data_get_buffer(data, &len);
	if (!raw_data || len != sizeof(ts))
		goto out;
	memcpy(&ts, raw_data, sizeof(ts));

out:
	mbuf_ancillary_data_unref(data);
	return ts;
}


static int frame_output(struct adec_prog *self,
			struct mbuf_audio_frame *out_frame)
{
	int res = 0;
	uint64_t input_time, dequeue_time, output_time;
	struct adef_frame info;

	res = wav_output(self, out_frame);
	if (res < 0)
		ULOG_ERRNO("wav_output", -res);

	input_time = get_timestamp(out_frame, ADEC_ANCILLARY_KEY_INPUT_TIME);
	dequeue_time =
		get_timestamp(out_frame, ADEC_ANCILLARY_KEY_DEQUEUE_TIME);
	output_time = get_timestamp(out_frame, ADEC_ANCILLARY_KEY_OUTPUT_TIME);

	res = mbuf_audio_frame_get_frame_info(out_frame, &info);
	if (res < 0)
		ULOG_ERRNO("mbuf_audio_frame_get_frame_info", -res);

	ULOGI("decoded frame #%d, dequeue: %.2fms, decode: %.2fms, "
	      "overall: %.2fms",
	      info.info.index,
	      (float)(dequeue_time - input_time) / 1000.,
	      (float)(output_time - dequeue_time) / 1000.,
	      (float)(output_time - input_time) / 1000.);

	self->first_out_frame = 0;

	return 0;
}


static void frame_output_cb(struct adec_decoder *dec,
			    int status,
			    struct mbuf_audio_frame *out_frame,
			    void *userdata)
{
	int res;
	struct adec_prog *self = userdata;

	ULOG_ERRNO_RETURN_IF(self == NULL, EINVAL);
	ULOG_ERRNO_RETURN_IF(out_frame == NULL, EINVAL);

	if (status != 0) {
		ULOGE("decoder error, resync required");
		return;
	}

	res = frame_output(self, out_frame);
	if (res < 0)
		ULOG_ERRNO("frame_output", -res);

	self->output_count++;

	if ((self->input_finished) &&
	    (self->output_count == self->input_count)) {
		ULOGI("decoding is finished (output, count=%d)",
		      self->output_count);
		self->output_finished = 1;
		return;
	}

	/* TODO: This should be triggered by an event in the in_pool signaling
	 * that a memory is available */
	if (self->pending_frame_len && self->in_pool && !self->in_mem) {
		res = mbuf_pool_get(self->in_pool, &self->in_mem);
		if (res < 0)
			return;

		res = decode_frame(self,
				   self->pending_frame,
				   self->pending_frame_len,
				   &self->pending_frame_adts);
		if (res < 0) {
			ULOG_ERRNO("decode_frame", -res);
			return;
		}

		res = pomp_loop_idle_add_with_cookie(
			self->loop, &aac_parse_idle, self, self);
		if (res < 0)
			ULOG_ERRNO("pomp_loop_idle_add_with_cookie", -res);
		self->pending_frame_len = 0;
	}
}


static void flush_cb(struct adec_decoder *dec, void *userdata)
{
	int res;
	struct adec_prog *self = userdata;

	ULOG_ERRNO_RETURN_IF(self == NULL, EINVAL);

	ULOGI("decoder is flushed");

	/* Stop the decoder */
	res = adec_stop(self->decoder);
	if (res < 0)
		ULOG_ERRNO("adec_stop", -res);
}


static void stop_cb(struct adec_decoder *dec, void *userdata)
{
	struct adec_prog *self = userdata;

	ULOG_ERRNO_RETURN_IF(self == NULL, EINVAL);

	ULOGI("decoder is stopped");
	self->stopped = 1;

	pomp_loop_wakeup(self->loop);
}


static const struct adec_cbs adec_cbs = {
	.frame_output = frame_output_cb,
	.flush = flush_cb,
	.stop = stop_cb,
};


static void finish_idle(void *userdata)
{
	int res;
	struct adec_prog *self = userdata;

	ULOG_ERRNO_RETURN_IF(self == NULL, EINVAL);

	if (self->finishing)
		return;

	if ((s_stopping) || (self->input_finished)) {
		self->finishing = 1;

		/* Stop the parser */
		stop_reader(self);

		/* Flush the decoder */
		res = adec_flush(self->decoder, (s_stopping) ? 1 : 0);
		if (res < 0)
			ULOG_ERRNO("adec_flush", -res);
	}
}


static void aac_parse_idle(void *userdata)
{
	int res;
	size_t off = 0;
	struct adec_prog *self = userdata;

	ULOG_ERRNO_RETURN_IF(self == NULL, EINVAL);

	/* Waiting for input memory buffer */
	if (self->pending_frame_len && self->in_pool && !self->in_mem)
		return;

	switch (self->config.encoding) {
	case ADEF_ENCODING_AAC_LC:
		res = aac_reader_parse(self->reader.aac,
				       0,
				       (uint8_t *)self->in_data + self->in_off,
				       self->in_len - self->in_off,
				       &off);
		if (res < 0) {
			ULOG_ERRNO("aac_reader_parse", -res);
			return;
		}
		break;
	default:
		break;
	}

	self->in_off += off;

	if (((self->in_off >= self->in_len) &&
	     (self->pending_frame_len == 0)) ||
	    ((self->max_count > 0) && (self->input_count >= self->max_count))) {
		ULOGI("decoding is finished (input, count=%d)",
		      self->input_count);
		self->input_finished = 1;

		/* Stop the parser now, no point continuing */
		stop_reader(self);

		res = pomp_loop_idle_add_with_cookie(
			self->loop, &finish_idle, self, self);
		if (res < 0)
			ULOG_ERRNO("pomp_loop_idle_add_with_cookie", -res);
	}

	if (!self->input_finished && !self->finishing) {
		res = pomp_loop_idle_add_with_cookie(
			self->loop, &aac_parse_idle, self, self);
		if (res < 0)
			ULOG_ERRNO("pomp_loop_idle_add_with_cookie", -res);
	}
}


static void sig_handler(int signum)
{
	int res;

	ULOGI("signal %d(%s) received", signum, strsignal(signum));
	printf("Stopping...\n");

	s_stopping = 1;
	signal(SIGINT, SIG_DFL);

	if (s_self == NULL)
		return;

	res = pomp_loop_idle_add_with_cookie(
		s_self->loop, &finish_idle, s_self, s_self);
	if (res < 0)
		ULOG_ERRNO("pomp_loop_idle_add_with_cookie", -res);
}


static const char short_options[] = "hi:o:s:n:";


static const struct option long_options[] = {
	{"help", no_argument, NULL, 'h'},
	{"infile", required_argument, NULL, 'i'},
	{"outfile", required_argument, NULL, 'o'},
	{"start", required_argument, NULL, 's'},
	{"count", required_argument, NULL, 'n'},
	{0, 0, 0, 0},
};


static void welcome(char *prog_name)
{
	printf("\n%s - Audio decoding program\n"
	       "Copyright (c) 2023 Parrot Drones SAS\n\n",
	       prog_name);
}


static void usage(char *prog_name)
{
	printf("Usage: %s [options]\n"
	       "Options:\n"
	       "  -h | --help                        "
	       "Print this message\n"
	       "  -i | --infile <file_name>          "
	       "Advanced Audio Coding (AAC) byte stream input file (.aac)\n"
	       "  -o | --outfile <file_name>         "
	       "WAVE output file (.wav)\n"
	       "  -s | --start <i>                   "
	       "Start decoding at frame index i\n"
	       "  -n | --count <n>                   "
	       "Decode at most n frames\n"
	       "\n",
	       prog_name);
}


static int is_suffix(const char *suffix, const char *s)
{
	size_t suffix_len = strlen(suffix);
	size_t s_len = strlen(s);

	return s_len >= suffix_len &&
	       (strcasecmp(suffix, &s[s_len - suffix_len]) == 0);
}


int main(int argc, char **argv)
{
	int err = 0, status = EXIT_SUCCESS;
	int idx, c;
	struct adec_prog *self = NULL;
	struct timespec cur_ts = {0, 0};
	uint64_t start_time = 0, end_time = 0;

	s_self = NULL;
	s_stopping = 0;

	welcome(argv[0]);

	/* Context allocation */
	self = calloc(1, sizeof(*self));
	if (self == NULL) {
		ULOG_ERRNO("calloc", ENOMEM);
		status = EXIT_FAILURE;
		goto out;
	}
	s_self = self;
#ifdef _WIN32
	self->in_file = INVALID_HANDLE_VALUE;
	self->in_file_map = INVALID_HANDLE_VALUE;
#else
	self->in_fd = -1;
#endif
	self->first_out_frame = 1;
	self->ts_inc = DEFAULT_TS_INC;

	/* Command-line parameters */
	while ((c = getopt_long(
			argc, argv, short_options, long_options, &idx)) != -1) {
		switch (c) {
		case 0:
			break;

		case 'h':
			usage(argv[0]);
			status = EXIT_SUCCESS;
			goto out;

		case 'i':
			self->input_file = optarg;
			if (is_suffix(".aac", self->input_file))
				self->config.encoding = ADEF_ENCODING_AAC_LC;
			break;

		case 'o':
			self->output_file = optarg;
			break;

		case 's':
			self->start_index = atoi(optarg);
			break;

		case 'n':
			self->max_count = atoi(optarg);
			break;

		default:
			usage(argv[0]);
			status = EXIT_FAILURE;
			goto out;
		}
	}

	/* Check the parameters */
	if (self->input_file == NULL) {
		ULOGE("invalid input file");
		usage(argv[0]);
		status = EXIT_FAILURE;
		goto out;
	}

	/* Setup signal handlers */
	signal(SIGINT, &sig_handler);
	signal(SIGTERM, &sig_handler);
#ifndef _WIN32
	signal(SIGPIPE, SIG_IGN);
#endif

	/* Loop */
	self->loop = pomp_loop_new();
	if (self->loop == NULL) {
		err = -ENOMEM;
		ULOG_ERRNO("pomp_loop_new", -err);
		status = EXIT_FAILURE;
		goto out;
	}

	/* Map the input file */
	err = map_file(self);
	if (err < 0) {
		status = EXIT_FAILURE;
		goto out;
	}

	/* Create reader */
	switch (self->config.encoding) {
	case ADEF_ENCODING_AAC_LC: {
		err = aac_reader_new(&aac_cbs, self, &self->reader.aac);
		if (err < 0) {
			ULOG_ERRNO("aac_reader_new", -err);
			status = EXIT_FAILURE;
			goto out;
		}
		break;
	}
	default:
		break;
	}

	self->in_info.info.timescale = 1000000;
	if (self->config.implem == ADEC_DECODER_IMPLEM_AUTO)
		self->config.implem = adec_get_auto_implem();

	if (self->config.implem == ADEC_DECODER_IMPLEM_AUTO) {
		ULOGE("unsupported audio encoding");
		status = EXIT_FAILURE;
		goto out;
	}

	/* Create the decoder */
	err = adec_new(
		self->loop, &self->config, &adec_cbs, self, &self->decoder);
	if (err < 0) {
		ULOG_ERRNO("adec_new", -err);
		status = EXIT_FAILURE;
		goto out;
	}

	/* Start */
	err = pomp_loop_idle_add_with_cookie(
		self->loop, aac_parse_idle, self, self);
	if (err < 0) {
		ULOG_ERRNO("pomp_loop_idle_add_with_cookie", -err);
		status = EXIT_FAILURE;
		goto out;
	}

	time_get_monotonic(&cur_ts);
	time_timespec_to_us(&cur_ts, &start_time);

	/* Main loop */
	while (!self->stopped) {
		err = pomp_loop_wait_and_process(self->loop, 100);
		if (err == -ETIMEDOUT)
			ULOGI("pomp_loop_wait_and_process");
	}

	time_get_monotonic(&cur_ts);
	time_timespec_to_us(&cur_ts, &end_time);
	printf("\nTotal frames: input=%u output=%u\n",
	       self->input_count,
	       self->output_count);
	printf("Overall time: %.2fs\n",
	       (float)(end_time - start_time) / 1000000.);
	if ((self->in_info.format.sample_rate != 0) &&
	    (self->total_bytes > 0) && (self->output_count > 0) &&
	    (self->input_count == self->output_count)) {
		double bitrate_scaled =
			(double)self->total_bytes * 8 / self->output_count *
			self->in_info.format.sample_rate / AAC_FRAME_LENGTH;
		char *bitrate_str = "";
		if (bitrate_scaled > 1000.) {
			bitrate_scaled /= 1000.;
			bitrate_str = "K";
		}
		if (bitrate_scaled > 1000.) {
			bitrate_scaled /= 1000.;
			bitrate_str = "M";
		}
		printf("Sample rate: %uKbit/s, bitrate: %.1f%sbit/s\n",
		       self->in_info.format.sample_rate / 1000,
		       bitrate_scaled,
		       bitrate_str);
	}

out:
	/* Cleanup and exit */
	if (self != NULL) {
		if (self->loop) {
			err = pomp_loop_idle_remove_by_cookie(self->loop, self);
			if (err < 0)
				ULOG_ERRNO("pomp_loop_idle_remove_by_cookie",
					   -err);
		}
		unmap_file(self);
		err = araw_writer_destroy(self->writer);
		if (err < 0)
			ULOG_ERRNO("araw_writer_destroy", -err);
		if (self->in_frame != NULL) {
			err = mbuf_audio_frame_unref(self->in_frame);
			if (err < 0)
				ULOG_ERRNO("mbuf_audio_frame_unref:input",
					   -err);
		}
		if (self->in_mem != NULL) {
			err = mbuf_mem_unref(self->in_mem);
			if (err < 0)
				ULOG_ERRNO("mbuf_mem_unref:input", -err);
		}

		switch (self->config.encoding) {
		case ADEF_ENCODING_AAC_LC:
			if (self->reader.aac != NULL) {
				err = aac_reader_destroy(self->reader.aac);
				if (err < 0)
					ULOG_ERRNO("aac_reader_destroy", -err);
			}
			break;
		default:
			break;
		}

		if (self->decoder != NULL) {
			err = adec_destroy(self->decoder);
			if (err < 0)
				ULOG_ERRNO("adec_destroy", -err);
		}
		if (self->in_pool_allocated) {
			err = mbuf_pool_destroy(self->in_pool);
			if (err < 0)
				ULOG_ERRNO("mbuf_pool_destroy:input", -err);
		}
		if (self->loop) {
			err = pomp_loop_destroy(self->loop);
			if (err < 0)
				ULOG_ERRNO("pomp_loop_destroy", -err);
		}
		free(self->pending_frame);
		free(self);
	}

	printf("\n%s\n", (status == EXIT_SUCCESS) ? "Finished!" : "Failed!");
	exit(status);
}
