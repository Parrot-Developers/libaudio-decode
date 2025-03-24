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

#include <stdio.h>

#define ULOG_TAG adec_fdk_aac
#include <ulog.h>
ULOG_DECLARE_TAG(ULOG_TAG);


#include "adec_fdk_aac_priv.h"


#define NB_SUPPORTED_FORMATS 48
static struct adef_format supported_formats[NB_SUPPORTED_FORMATS];
static pthread_once_t supported_formats_is_init = PTHREAD_ONCE_INIT;
static void initialize_supported_formats(void)
{
	/* Note: The FDK library is based on fixed-point math and only supports
	 * 16-bit integer AAC input. */
	int i = 0;
	supported_formats[i++] = adef_aac_lc_16b_8000hz_mono_raw;
	supported_formats[i++] = adef_aac_lc_16b_8000hz_stereo_raw;
	supported_formats[i++] = adef_aac_lc_16b_11025hz_mono_raw;
	supported_formats[i++] = adef_aac_lc_16b_11025hz_stereo_raw;
	supported_formats[i++] = adef_aac_lc_16b_12000hz_mono_raw;
	supported_formats[i++] = adef_aac_lc_16b_12000hz_stereo_raw;
	supported_formats[i++] = adef_aac_lc_16b_16000hz_mono_raw;
	supported_formats[i++] = adef_aac_lc_16b_16000hz_stereo_raw;
	supported_formats[i++] = adef_aac_lc_16b_22050hz_mono_raw;
	supported_formats[i++] = adef_aac_lc_16b_22050hz_stereo_raw;
	supported_formats[i++] = adef_aac_lc_16b_24000hz_mono_raw;
	supported_formats[i++] = adef_aac_lc_16b_24000hz_stereo_raw;
	supported_formats[i++] = adef_aac_lc_16b_32000hz_mono_raw;
	supported_formats[i++] = adef_aac_lc_16b_32000hz_stereo_raw;
	supported_formats[i++] = adef_aac_lc_16b_44100hz_mono_raw;
	supported_formats[i++] = adef_aac_lc_16b_44100hz_stereo_raw;
	supported_formats[i++] = adef_aac_lc_16b_48000hz_mono_raw;
	supported_formats[i++] = adef_aac_lc_16b_48000hz_stereo_raw;
	supported_formats[i++] = adef_aac_lc_16b_64000hz_mono_raw;
	supported_formats[i++] = adef_aac_lc_16b_64000hz_stereo_raw;
	supported_formats[i++] = adef_aac_lc_16b_88200hz_mono_raw;
	supported_formats[i++] = adef_aac_lc_16b_88200hz_stereo_raw;
	supported_formats[i++] = adef_aac_lc_16b_96000hz_mono_raw;
	supported_formats[i++] = adef_aac_lc_16b_96000hz_stereo_raw;
	supported_formats[i++] = adef_aac_lc_16b_8000hz_mono_adts;
	supported_formats[i++] = adef_aac_lc_16b_8000hz_stereo_adts;
	supported_formats[i++] = adef_aac_lc_16b_11025hz_mono_adts;
	supported_formats[i++] = adef_aac_lc_16b_11025hz_stereo_adts;
	supported_formats[i++] = adef_aac_lc_16b_12000hz_mono_adts;
	supported_formats[i++] = adef_aac_lc_16b_12000hz_stereo_adts;
	supported_formats[i++] = adef_aac_lc_16b_16000hz_mono_adts;
	supported_formats[i++] = adef_aac_lc_16b_16000hz_stereo_adts;
	supported_formats[i++] = adef_aac_lc_16b_22050hz_mono_adts;
	supported_formats[i++] = adef_aac_lc_16b_22050hz_stereo_adts;
	supported_formats[i++] = adef_aac_lc_16b_24000hz_mono_adts;
	supported_formats[i++] = adef_aac_lc_16b_24000hz_stereo_adts;
	supported_formats[i++] = adef_aac_lc_16b_32000hz_mono_adts;
	supported_formats[i++] = adef_aac_lc_16b_32000hz_stereo_adts;
	supported_formats[i++] = adef_aac_lc_16b_44100hz_mono_adts;
	supported_formats[i++] = adef_aac_lc_16b_44100hz_stereo_adts;
	supported_formats[i++] = adef_aac_lc_16b_48000hz_mono_adts;
	supported_formats[i++] = adef_aac_lc_16b_48000hz_stereo_adts;
	supported_formats[i++] = adef_aac_lc_16b_64000hz_mono_adts;
	supported_formats[i++] = adef_aac_lc_16b_64000hz_stereo_adts;
	supported_formats[i++] = adef_aac_lc_16b_88200hz_mono_adts;
	supported_formats[i++] = adef_aac_lc_16b_88200hz_stereo_adts;
	supported_formats[i++] = adef_aac_lc_16b_96000hz_mono_adts;
	supported_formats[i++] = adef_aac_lc_16b_96000hz_stereo_adts;
}


static const char *aac_decoder_error_to_str(AAC_DECODER_ERROR err)
{
	switch (err) {
	case AAC_DEC_OK:
		return "OK";
	case AAC_DEC_OUT_OF_MEMORY:
		return "OUT_OF_MEMORY";
	case AAC_DEC_UNKNOWN:
		return "UNKNOWN";
	case AAC_DEC_TRANSPORT_SYNC_ERROR:
		return "TRANSPORT_SYNC_ERROR";
	case AAC_DEC_NOT_ENOUGH_BITS:
		return "NOT_ENOUGH_BITS";
	case AAC_DEC_INVALID_HANDLE:
		return "INVALID_HANDLE";
	case AAC_DEC_UNSUPPORTED_AOT:
		return "UNSUPPORTED_AOT";
	case AAC_DEC_UNSUPPORTED_FORMAT:
		return "UNSUPPORTED_FORMAT";
	case AAC_DEC_UNSUPPORTED_ER_FORMAT:
		return "UNSUPPORTED_ER_FORMAT";
	case AAC_DEC_UNSUPPORTED_EPCONFIG:
		return "UNSUPPORTED_EPCONFIG";
	case AAC_DEC_UNSUPPORTED_MULTILAYER:
		return "UNSUPPORTED_MULTILAYER";
	case AAC_DEC_UNSUPPORTED_CHANNELCONFIG:
		return "UNSUPPORTED_CHANNELCONFIG";
	case AAC_DEC_UNSUPPORTED_SAMPLINGRATE:
		return "UNSUPPORTED_SAMPLINGRATE";
	case AAC_DEC_INVALID_SBR_CONFIG:
		return "INVALID_SBR_CONFIG";
	case AAC_DEC_SET_PARAM_FAIL:
		return "SET_PARAM_FAIL";
	case AAC_DEC_NEED_TO_RESTART:
		return "NEED_TO_RESTART";
	case AAC_DEC_OUTPUT_BUFFER_TOO_SMALL:
		return "OUTPUT_BUFFER_TOO_SMALL";
	case AAC_DEC_TRANSPORT_ERROR:
		return "TRANSPORT_ERROR";
	case AAC_DEC_PARSE_ERROR:
		return "PARSE_ERROR";
	case AAC_DEC_UNSUPPORTED_EXTENSION_PAYLOAD:
		return "UNSUPPORTED_EXTENSION_PAYLOAD";
	case AAC_DEC_DECODE_FRAME_ERROR:
		return "DECODE_FRAME_ERROR";
	case AAC_DEC_CRC_ERROR:
		return "CRC_ERROR";
	case AAC_DEC_INVALID_CODE_BOOK:
		return "INVALID_CODE_BOOK";
	case AAC_DEC_UNSUPPORTED_PREDICTION:
		return "UNSUPPORTED_PREDICTION";
	case AAC_DEC_UNSUPPORTED_CCE:
		return "UNSUPPORTED_CCE";
	case AAC_DEC_UNSUPPORTED_LFE:
		return "UNSUPPORTED_LFE";
	case AAC_DEC_UNSUPPORTED_GAIN_CONTROL_DATA:
		return "UNSUPPORTED_GAIN_CONTROL_DATA";
	case AAC_DEC_UNSUPPORTED_SBA:
		return "UNSUPPORTED_SBA";
	case AAC_DEC_TNS_READ_ERROR:
		return "TNS_READ_ERROR";
	case AAC_DEC_RVLC_ERROR:
		return "RVLC_ERROR";
	case AAC_DEC_ANC_DATA_ERROR:
		return "ANC_DATA_ERROR";
	case AAC_DEC_TOO_SMALL_ANC_BUFFER:
		return "TOO_SMALL_ANC_BUFFER";
	case AAC_DEC_TOO_MANY_ANC_ELEMENTS:
		return "TOO_MANY_ANC_ELEMENTS";
	default:
		return "UNKNOWN";
	}
	return NULL;
}


static void call_flush_done(void *userdata)
{
	struct adec_fdk_aac *self = userdata;

	adec_call_flush_cb(self->base);
}


static void call_stop_done(void *userdata)
{
	struct adec_fdk_aac *self = userdata;

	adec_call_stop_cb(self->base);
}


static void mbox_cb(int fd, uint32_t revents, void *userdata)
{
	struct adec_fdk_aac *self = userdata;
	int ret, err;
	char message;

	do {
		/* Read from the mailbox */
		ret = mbox_peek(self->mbox, &message);
		if (ret < 0) {
			if (ret != -EAGAIN)
				ADEC_LOG_ERRNO("mbox_peek", -ret);
			break;
		}

		switch (message) {
		case ADEC_MSG_FLUSH:
			err = pomp_loop_idle_add_with_cookie(
				self->base->loop, call_flush_done, self, self);
			if (err < 0)
				ADEC_LOG_ERRNO("pomp_loop_idle_add_with_cookie",
					       -err);
			break;
		case ADEC_MSG_STOP:
			err = pomp_loop_idle_add_with_cookie(
				self->base->loop, call_stop_done, self, self);
			if (err < 0)
				ADEC_LOG_ERRNO("pomp_loop_idle_add_with_cookie",
					       -err);
			break;
		default:
			ADEC_LOGE("unknown message: %c", message);
			break;
		}
	} while (ret == 0);
}


static void out_queue_evt_cb(struct pomp_evt *evt, void *userdata)
{
	struct adec_fdk_aac *self = userdata;
	struct mbuf_audio_frame *out_frame;
	int err;

	do {
		err = mbuf_audio_frame_queue_pop(self->out_queue, &out_frame);
		if (err == -EAGAIN) {
			return;
		} else if (err < 0) {
			ADEC_LOG_ERRNO("mbuf_audio_frame_queue_pop:out_queue",
				       -err);
			return;
		}
		struct adef_frame out_info = {};
		err = mbuf_audio_frame_get_frame_info(out_frame, &out_info);
		if (err < 0)
			ADEC_LOG_ERRNO("mbuf_audio_frame_get_frame_info", -err);

		if (!atomic_load(&self->flush_discard))
			adec_call_frame_output_cb(self->base, 0, out_frame);
		else
			ADEC_LOGD("discarding frame %d", out_info.info.index);
		mbuf_audio_frame_unref(out_frame);
	} while (err == 0);
}


static int complete_flush(struct adec_fdk_aac *self)
{
	int ret;

	if (atomic_load(&self->flush_discard)) {
		/* Flush the decoder input queue */
		ret = mbuf_audio_frame_queue_flush(self->in_queue);
		if (ret < 0) {
			ADEC_LOG_ERRNO("mbuf_audio_frame_queue_flush:input",
				       -ret);
			return ret;
		}
		/* Flush the decoder output queue */
		ret = mbuf_audio_frame_queue_flush(self->out_queue);
		if (ret < 0) {
			ADEC_LOG_ERRNO("mbuf_audio_frame_queue_flush:out_queue",
				       -ret);
			return ret;
		}
	}

	atomic_store(&self->flushing, 0);
	atomic_store(&self->flush_discard, 0);

	/* Call the flush callback on the loop */
	char message = ADEC_MSG_FLUSH;
	ret = mbox_push(self->mbox, &message);
	if (ret < 0)
		ADEC_LOG_ERRNO("mbox_push", -ret);

	return ret;
}


static int get_stream_info(struct adec_fdk_aac *self)
{
	int ret;

	if (self->output_format_valid)
		return 0;

	self->info = aacDecoder_GetStreamInfo(self->handle);
	if (self->info == NULL) {
		ret = -EINVAL;
		ADEC_LOG_ERRNO("aacDecoder_GetStreamInfo", -ret);
		return ret;
	}

	/* TODO */
	self->output_format.encoding = ADEF_ENCODING_PCM;
	self->output_format.sample_rate = self->info->sampleRate;
	self->output_format.channel_count = self->info->numChannels;
	self->output_format.bit_depth = 16;
	self->output_format.pcm.interleaved = true;
	self->output_format.pcm.signed_val = true;
	self->output_format.pcm.little_endian = true;
	self->output_format.aac.data_format = ADEF_AAC_DATA_FORMAT_UNKNOWN;

	self->output_size = self->output_format.channel_count *
			    self->output_format.bit_depth / 8 *
			    self->info->frameSize;

	self->output_format_valid = true;

	return 0;
}


static int decode_frame(struct adec_fdk_aac *self,
			struct mbuf_audio_frame *in_frame)
{
	int ret = 0;
	AAC_DECODER_ERROR err;
	struct adef_frame in_info;
	struct timespec cur_ts = {0, 0};
	uint64_t ts_us;
	const void *frame_data = NULL;
	size_t frame_len = 0;
	unsigned char *in_buffer[1] = {0};
	unsigned int in_buffer_length[1] = {0};
	unsigned int valid[1] = {0};
	struct mbuf_mem *mem = NULL;
	size_t mem_size;
	uint8_t *data;
	struct mbuf_audio_frame *out_frame = NULL;
	struct adef_frame out_info;
	bool has_decoded = false;

	if (in_frame == NULL)
		return 0;

	ret = mbuf_audio_frame_get_frame_info(in_frame, &in_info);
	if (ret < 0) {
		ADEC_LOG_ERRNO("mbuf_audio_frame_get_frame_info", -ret);
		goto out;
	}

	if (!adef_format_intersect(
		    &in_info.format, supported_formats, NB_SUPPORTED_FORMATS)) {
		ret = -ENOSYS;
		char *fmt = adef_format_to_str(&in_info.format);
		ADEC_LOG_ERRNO("unsupported format: %s", -ret, fmt);
		free(fmt);
		goto out;
	}

	ret = mbuf_audio_frame_get_buffer(in_frame, &frame_data, &frame_len);
	if (ret != 0) {
		ADEC_LOG_ERRNO("mbuf_audio_frame_get_buffer", -ret);
		goto out;
	}

	time_get_monotonic(&cur_ts);
	time_timespec_to_us(&cur_ts, &ts_us);

	in_buffer[0] = (unsigned char *)frame_data;
	in_buffer_length[0] = frame_len;
	valid[0] = frame_len;
	/* Loop until the whole frame has been digested. */
	while (valid[0] > 0) {
		/* Push the frame */
		err = aacDecoder_Fill(
			self->handle, in_buffer, in_buffer_length, valid);
		if (err != AAC_DEC_OK) {
			ret = -EPROTO;
			ADEC_LOGE("aacDecoder_Fill: %s",
				  aac_decoder_error_to_str(err));
			goto out;
		}
	}

	err = mbuf_audio_frame_add_ancillary_buffer(
		in_frame,
		ADEC_ANCILLARY_KEY_DEQUEUE_TIME,
		&ts_us,
		sizeof(ts_us));
	if (err != 0)
		ADEC_LOG_ERRNO("mbuf_audio_frame_add_ancillary_buffer", -err);

	self->base->counters.pushed++;

	/* Loop as long as the decoder outputs frames */
	while (true) {
		/* Decoder is not configured (yet), output buffer size is
		 * unknown: allocate a large-enough buffer. */
		if (self->output_size == 0)
			mem_size = ADEC_DEFAULT_OUTPUT_SIZE;
		else
			mem_size = self->output_size;
		ret = mbuf_mem_generic_new(mem_size, &mem);
		if (ret < 0) {
			ADEC_LOG_ERRNO("mbuf_mem_generic_new", -ret);
			goto out;
		}
		ret = mbuf_mem_get_data(mem, (void **)&data, &mem_size);
		if (ret < 0) {
			ADEC_LOG_ERRNO("mbuf_mem_get_data", -ret);
			goto out;
		}

		/* Decode frame */
		err = aacDecoder_DecodeFrame(
			self->handle, (INT_PCM *)data, (mem_size / 2), 0);
		switch (err) {
		case AAC_DEC_OK:
			/* OK */
			break;
		case AAC_DEC_NOT_ENOUGH_BITS:
			ret = has_decoded ? 0 : -ENOSPC;
			goto out;
			break;
		default:
			ret = -EPROTO;
			ADEC_LOGE("aacDecoder_DecodeFrame: %s",
				  aac_decoder_error_to_str(err));
			goto out;
		}

		has_decoded = true;
		self->base->counters.pulled++;

		if (!self->output_format_valid) {
			/* Read stream info once one frame was decoded */
			ret = get_stream_info(self);
			if (ret < 0 || !self->output_format_valid) {
				ADEC_LOG_ERRNO("get_stream_info", -ret);
				goto out;
			}

			mem_size = self->output_size;
		}

		/* Fill PCM frame info */
		out_info.info = in_info.info;
		out_info.format = self->output_format;

		ret = mbuf_audio_frame_new(&out_info, &out_frame);
		if (ret < 0) {
			ADEC_LOG_ERRNO("mbuf_audio_frame_new", -ret);
			goto out;
		}

		ret = mbuf_audio_frame_foreach_ancillary_data(
			in_frame,
			mbuf_audio_frame_ancillary_data_copier,
			out_frame);
		if (ret < 0) {
			ADEC_LOG_ERRNO(
				"mbuf_audio_frame_foreach_ancillary_data",
				-ret);
			goto out;
		}

		ret = mbuf_audio_frame_set_buffer(
			out_frame, mem, 0, self->output_size);
		if (ret < 0) {
			ADEC_LOG_ERRNO("mbuf_audio_frame_set_buffer", -ret);
			goto out;
		}

		time_get_monotonic(&cur_ts);
		time_timespec_to_us(&cur_ts, &ts_us);

		ret = mbuf_audio_frame_add_ancillary_buffer(
			out_frame,
			ADEC_ANCILLARY_KEY_OUTPUT_TIME,
			&ts_us,
			sizeof(ts_us));
		if (ret < 0)
			ADEC_LOG_ERRNO("mbuf_audio_frame_add_ancillary_buffer",
				       -ret);

		ret = mbuf_audio_frame_finalize(out_frame);
		if (ret < 0)
			ADEC_LOG_ERRNO("mbuf_audio_frame_finalize", -ret);

		ret = mbuf_audio_frame_queue_push(self->out_queue, out_frame);
		if (ret < 0) {
			ADEC_LOG_ERRNO("mbuf_audio_frame_queue_push:decoder",
				       -ret);
			goto out;
		}

		if (mem != NULL) {
			err = mbuf_mem_unref(mem);
			if (err != 0)
				ADEC_LOG_ERRNO("mbuf_mem_unref", -err);
			mem = NULL;
		}
		if (out_frame != NULL) {
			err = mbuf_audio_frame_unref(out_frame);
			if (err != 0)
				ADEC_LOG_ERRNO("mbuf_audio_frame_unref", -err);
			out_frame = NULL;
		}
	}

out:
	if (mem) {
		err = mbuf_mem_unref(mem);
		if (err != 0)
			ADEC_LOG_ERRNO("mbuf_mem_unref", -err);
	}
	if (out_frame) {
		err = mbuf_audio_frame_unref(out_frame);
		if (err != 0)
			ADEC_LOG_ERRNO("mbuf_audio_frame_unref", -err);
	}
	if (frame_data)
		mbuf_audio_frame_release_buffer(in_frame, frame_data);
	return ret;
}


static int start_flush(struct adec_fdk_aac *self)
{
	int ret = 0;

	if (atomic_load(&self->flush_discard)) {
		/* Flush the input queue */
		ret = mbuf_audio_frame_queue_flush(self->in_queue);
		if (ret < 0) {
			ADEC_LOG_ERRNO("mbuf_audio_frame_queue_flush:input",
				       -ret);
			return ret;
		}
		ret = aacDecoder_SetParam(
			self->handle, AAC_TPDEC_CLEAR_BUFFER, 1);
		if (ret != AAC_DEC_OK) {
			ret = -EPROTO;
			ADEC_LOGE("aacDecoder_SetParam: %s",
				  aac_decoder_error_to_str(ret));
			return -ret;
		}
	}

	atomic_store(&self->flush, 0);
	atomic_store(&self->flushing, 1);

	return ret;
}


static void check_input_queue(struct adec_fdk_aac *self)
{
	int ret, err = 0;
	struct mbuf_audio_frame *in_frame;

	ret = mbuf_audio_frame_queue_peek(self->in_queue, &in_frame);
	while (ret == 0) {
		/* Push the input frame */
		ret = decode_frame(self, in_frame);
		if (ret < 0 && ret != -ENOSPC) {

			if (ret != -EAGAIN)
				ADEC_LOG_ERRNO("decode_frame", -ret);
			err = -ENOSPC;
		}
		if (in_frame != NULL) {
			mbuf_audio_frame_unref(in_frame);
			/* Pop the frame for real */
			if (err == 0) {
				ret = mbuf_audio_frame_queue_pop(self->in_queue,
								 &in_frame);
				if (ret < 0) {
					ADEC_LOG_ERRNO(
						"mbuf_audio_frame_queue_pop",
						-ret);
					break;
				}
				mbuf_audio_frame_unref(in_frame);
			}
		}
		if (err)
			break;
		if (atomic_load(&self->flush)) {
			ret = start_flush(self);
			if (ret < 0)
				ADEC_LOG_ERRNO("start_flush", -ret);
		}
		/* Peek the next frame */
		ret = mbuf_audio_frame_queue_peek(self->in_queue, &in_frame);
		if (ret < 0 && ret != -EAGAIN && ret != -ENOSPC)
			ADEC_LOG_ERRNO("mbuf_audio_frame_queue_peek", -ret);
	}
	if (ret != -EAGAIN && ret != -ENOSPC)
		ADEC_LOG_ERRNO("mbuf_audio_frame_queue_peek", -ret);
	if (atomic_load(&self->flush)) {
		ret = start_flush(self);
		if (ret < 0)
			ADEC_LOG_ERRNO("start_flush", -ret);
	}
}


static void input_event_cb(struct pomp_evt *evt, void *userdata)
{
	struct adec_fdk_aac *self = userdata;
	check_input_queue(self);
}


static void *adec_fdk_aac_decoder_thread(void *ptr)
{
	int ret;
	struct adec_fdk_aac *self = ptr;
	struct pomp_loop *loop = NULL;
	struct pomp_evt *in_queue_evt = NULL;
	int timeout;
	char message;

#if defined(__APPLE__)
#	if !TARGET_OS_IPHONE
	ret = pthread_setname_np("adec_fdkaac");
	if (ret != 0)
		ADEC_LOG_ERRNO("pthread_setname_np", ret);
#	endif
#else
	ret = pthread_setname_np(pthread_self(), "adec_fdkaac");
	if (ret != 0)
		ADEC_LOG_ERRNO("pthread_setname_np", ret);
#endif

	loop = pomp_loop_new();
	if (!loop) {
		ADEC_LOG_ERRNO("pomp_loop_new", ENOMEM);
		goto exit;
	}
	ret = mbuf_audio_frame_queue_get_event(self->in_queue, &in_queue_evt);
	if (ret != 0) {
		ADEC_LOG_ERRNO("mbuf_audio_frame_queue_get_event", -ret);
		goto exit;
	}
	ret = pomp_evt_attach_to_loop(in_queue_evt, loop, input_event_cb, self);
	if (ret != 0) {
		ADEC_LOG_ERRNO("pomp_evt_attach_to_loop", -ret);
		goto exit;
	}

	while (!atomic_load(&self->should_stop) ||
	       atomic_load(&self->flushing)) {
		/* Start flush, discarding all frames */
		if ((atomic_load(&self->flushing))) {
			ret = complete_flush(self);
			if (ret < 0)
				ADEC_LOG_ERRNO("complete_flush", -ret);
			continue;
		}

		timeout = (atomic_load(&self->flushing) &&
			   !atomic_load(&self->flush_discard))
				  ? 0
				  : 5;

		ret = pomp_loop_wait_and_process(loop, timeout);
		if (ret < 0 && ret != -ETIMEDOUT) {
			ADEC_LOG_ERRNO("pomp_loop_wait_and_process", -ret);
			if (!atomic_load(&self->should_stop)) {
				/* Avoid looping on errors */
				usleep(5000);
			}
			continue;
		} else if (ret == -ETIMEDOUT) {
			check_input_queue(self);
		}
	}

	/* Call the stop callback on the loop */
	message = ADEC_MSG_STOP;
	ret = mbox_push(self->mbox, &message);
	if (ret < 0)
		ADEC_LOG_ERRNO("mbox_push", -ret);

exit:
	if (in_queue_evt != NULL) {
		ret = pomp_evt_detach_from_loop(in_queue_evt, loop);
		if (ret != 0)
			ADEC_LOG_ERRNO("pomp_evt_detach_from_loop", -ret);
	}
	if (loop != NULL) {
		ret = pomp_loop_destroy(loop);
		if (ret != 0)
			ADEC_LOG_ERRNO("pomp_loop_destroy", -ret);
	}

	return NULL;
}


static int get_supported_input_formats(const struct adef_format **formats)
{
	(void)pthread_once(&supported_formats_is_init,
			   initialize_supported_formats);
	*formats = supported_formats;
	return NB_SUPPORTED_FORMATS;
}


static int stop(struct adec_decoder *base)
{
	struct adec_fdk_aac *self = NULL;

	ADEC_LOG_ERRNO_RETURN_ERR_IF(base == NULL, EINVAL);

	self = base->derived;

	/* Stop the decoding thread */
	atomic_store(&self->should_stop, true);
	self->base->configured = 0;

	return 0;
}


static int destroy(struct adec_decoder *base)
{
	int err;
	struct adec_fdk_aac *self = NULL;

	ADEC_LOG_ERRNO_RETURN_ERR_IF(base == NULL, EINVAL);

	self = base->derived;
	if (self == NULL)
		return 0;

	/* Stop and join the decoding thread */
	err = stop(base);
	if (err < 0)
		ADEC_LOG_ERRNO("adec_fdk_aac_stop", -err);
	if (atomic_load(&self->thread_launched)) {
		err = pthread_join(self->thread, NULL);
		if (err != 0)
			ADEC_LOG_ERRNO("pthread_join", err);
	}

	/* Free the resources */
	if (self->out_queue_evt != NULL) {
		err = pomp_evt_detach_from_loop(self->out_queue_evt,
						base->loop);
		if (err < 0)
			ADEC_LOG_ERRNO("pomp_evt_detach_from_loop", -err);
	}
	if (self->out_queue != NULL) {
		err = mbuf_audio_frame_queue_destroy(self->out_queue);
		if (err < 0)
			ADEC_LOG_ERRNO("mbuf_audio_frame_queue_destroy", -err);
	}
	if (self->in_queue != NULL) {
		err = mbuf_audio_frame_queue_destroy(self->in_queue);
		if (err < 0)
			ADEC_LOG_ERRNO("mbuf_audio_frame_queue_destroy", -err);
	}
	if (self->mbox != NULL) {
		err = pomp_loop_remove(base->loop,
				       mbox_get_read_fd(self->mbox));
		if (err < 0)
			ADEC_LOG_ERRNO("pomp_loop_remove", -err);
		mbox_destroy(self->mbox);
	}

	/* Close instance */
	if (self->handle != NULL)
		aacDecoder_Close(self->handle);

	err = pomp_loop_idle_remove_by_cookie(base->loop, self);
	if (err < 0)
		ADEC_LOG_ERRNO("pomp_loop_idle_remove_by_cookie", -err);

	free(self);
	base->derived = NULL;

	return 0;
}


static bool input_filter(struct mbuf_audio_frame *frame, void *userdata)
{
	const void *tmp;
	size_t tmplen;
	struct adef_frame info;
	int ret;
	struct adec_fdk_aac *self = userdata;

	ADEC_LOG_ERRNO_RETURN_ERR_IF(self == NULL, false);

	if (atomic_load(&self->flushing) || atomic_load(&self->should_stop))
		return false;

	ret = mbuf_audio_frame_get_frame_info(frame, &info);
	if (ret != 0)
		return false;

	/* Pass default filters first */
	if (!adec_default_input_filter_internal(self->base,
						frame,
						&info,
						supported_formats,
						NB_SUPPORTED_FORMATS))
		return false;

	/* Input frame must be packed */
	ret = mbuf_audio_frame_get_buffer(frame, &tmp, &tmplen);
	if (ret != 0)
		return false;

	mbuf_audio_frame_release_buffer(frame, tmp);

	adec_default_input_filter_internal_confirm_frame(
		self->base, frame, &info);

	return true;
}


static int create(struct adec_decoder *base)
{
	int ret = 0;
	struct adec_fdk_aac *self = NULL;
	struct mbuf_audio_frame_queue_args queue_args = {
		.filter = input_filter,
	};

	ADEC_LOG_ERRNO_RETURN_ERR_IF(base == NULL, EINVAL);

	(void)pthread_once(&supported_formats_is_init,
			   initialize_supported_formats);

	/* Check the configuration */
	if (base->config.encoding != ADEF_ENCODING_AAC_LC) {
		ret = -EINVAL;
		ADEC_LOG_ERRNO("invalid encoding: %s",
			       -ret,
			       adef_encoding_to_str(base->config.encoding));
		return ret;
	}

	self = calloc(1, sizeof(*self));
	if (self == NULL)
		return -ENOMEM;
	self->base = base;
	base->derived = self;
	queue_args.filter_userdata = self;

	/* Initialize the mailbox for inter-thread messages  */
	self->mbox = mbox_new(1);
	if (self->mbox == NULL) {
		ret = -ENOMEM;
		ADEC_LOG_ERRNO("mbox_new", -ret);
		goto error;
	}
	ret = pomp_loop_add(base->loop,
			    mbox_get_read_fd(self->mbox),
			    POMP_FD_EVENT_IN,
			    &mbox_cb,
			    self);
	if (ret < 0) {
		ADEC_LOG_ERRNO("pomp_loop_add", -ret);
		goto error;
	}

	ADEC_LOGI("FDK_AAC implementation");

	/* Create the ouput buffers queue */
	ret = mbuf_audio_frame_queue_new(&self->out_queue);
	if (ret < 0) {
		ADEC_LOG_ERRNO("mbuf_audio_frame_queue_new:output", -ret);
		goto error;
	}
	ret = mbuf_audio_frame_queue_get_event(self->out_queue,
					       &self->out_queue_evt);
	if (ret != 0) {
		ADEC_LOG_ERRNO("mbuf_audio_frame_queue_get_event", -ret);
		goto error;
	}
	ret = pomp_evt_attach_to_loop(
		self->out_queue_evt, base->loop, &out_queue_evt_cb, self);
	if (ret < 0) {
		ADEC_LOG_ERRNO("pomp_evt_attach_to_loop", -ret);
		goto error;
	}

	/* Create the input buffers queue */
	ret = mbuf_audio_frame_queue_new_with_args(&queue_args,
						   &self->in_queue);
	if (ret < 0) {
		ADEC_LOG_ERRNO("mbuf_audio_frame_queue_new_with_args", -ret);
		goto error;
	}

	ret = pthread_create(
		&self->thread, NULL, adec_fdk_aac_decoder_thread, self);
	if (ret != 0) {
		ret = -ret;
		ADEC_LOG_ERRNO("pthread_create", -ret);
		goto error;
	}

	atomic_store(&self->thread_launched, true);

	return 0;

error:
	/* Cleanup on error */
	destroy(base);
	base->derived = NULL;
	return ret;
}


static int flush(struct adec_decoder *base, int discard)
{
	struct adec_fdk_aac *self = NULL;

	ADEC_LOG_ERRNO_RETURN_ERR_IF(base == NULL, EINVAL);

	self = base->derived;

	atomic_store(&self->flush, 1);
	atomic_store(&self->flush_discard, discard);

	return 0;
}


static int set_aac_asc(struct adec_decoder *base,
		       const uint8_t *asc,
		       size_t asc_size,
		       enum adef_aac_data_format data_format)
{
	int err, ret;
	TRANSPORT_TYPE tt;
	struct adec_fdk_aac *self = NULL;
	ADEC_LOG_ERRNO_RETURN_ERR_IF(base == NULL, EINVAL);
	self = base->derived;
	int conceal_method = 0; /* TODO */

	switch (data_format) {
	case ADEF_AAC_DATA_FORMAT_RAW:
		tt = TT_MP4_RAW;
		break;
	case ADEF_AAC_DATA_FORMAT_ADIF:
		tt = TT_MP4_ADIF;
		break;
	case ADEF_AAC_DATA_FORMAT_ADTS:
		tt = TT_MP4_ADTS;
		break;
	default:
		ret = -ENOSYS;
		ADEC_LOG_ERRNO("unsupported data format", -ret);
		return ret;
	}

	/* Initialize the decoder */
	self->handle = aacDecoder_Open(tt, 1);
	if (self->handle == NULL) {
		ret = -EPROTO;
		ADEC_LOG_ERRNO("aacDecoder_Open", -ret);
		return ret;
	}

	if (tt == TT_MP4_RAW) {
		unsigned char *conf[] = {(uint8_t *)asc};
		unsigned int conf_len[] = {asc_size};
		err = aacDecoder_ConfigRaw(self->handle, conf, conf_len);
		if (err != AAC_DEC_OK) {
			ret = -EPROTO;
			ADEC_LOGE("aacDecoder_ConfigRaw: %s",
				  aac_decoder_error_to_str(err));
			return ret;
		}
	}

	/* Set decoder params */
	err = aacDecoder_SetParam(
		self->handle, AAC_CONCEAL_METHOD, conceal_method);
	if (err != AAC_DEC_OK) {
		ret = -EPROTO;
		ADEC_LOGE("aacDecoder_SetParam:AAC_CONCEAL_METHOD: %s",
			  aac_decoder_error_to_str(err));
		return ret;
	}

	return 0;
}


static struct mbuf_pool *get_input_buffer_pool(struct adec_decoder *base)
{
	struct adec_fdk_aac *self = NULL;

	ADEC_LOG_ERRNO_RETURN_VAL_IF(base == NULL, EINVAL, NULL);
	self = base->derived;

	/* No input buffer pool allocated: use the application's */
	return NULL;
}


static struct mbuf_audio_frame_queue *
get_input_buffer_queue(struct adec_decoder *base)
{
	struct adec_fdk_aac *self = NULL;

	ADEC_LOG_ERRNO_RETURN_VAL_IF(base == NULL, EINVAL, NULL);
	self = base->derived;

	return self->in_queue;
}


const struct adec_ops adec_fdk_aac_ops = {
	.get_supported_input_formats = get_supported_input_formats,
	.create = create,
	.flush = flush,
	.stop = stop,
	.destroy = destroy,
	.set_aac_asc = set_aac_asc,
	.get_input_buffer_pool = get_input_buffer_pool,
	.get_input_buffer_queue = get_input_buffer_queue,
};
