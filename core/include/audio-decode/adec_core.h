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

#ifndef _ADEC_CORE_H_
#define _ADEC_CORE_H_

#include <stdint.h>
#include <unistd.h>

#include <audio-defs/adefs.h>
#include <media-buffers/mbuf_audio_frame.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* To be used for all public API */
#ifdef ADEC_API_EXPORTS
#	ifdef _WIN32
#		define ADEC_API __declspec(dllexport)
#	else /* !_WIN32 */
#		define ADEC_API __attribute__((visibility("default")))
#	endif /* !_WIN32 */
#else /* !ADEC_API_EXPORTS */
#	define ADEC_API
#endif /* !ADEC_API_EXPORTS */


/**
 * mbuf ancillary data key for the input timestamp.
 *
 * Content is a 64bits microseconds value on a monotonic clock
 */
#define ADEC_ANCILLARY_KEY_INPUT_TIME "adec.input_time"

/**
 * mbuf ancillary data key for the dequeue timestamp.
 *
 * Content is a 64bits microseconds value on a monotonic clock
 */
#define ADEC_ANCILLARY_KEY_DEQUEUE_TIME "adec.dequeue_time"

/**
 * mbuf ancillary data key for the output timestamp.
 *
 * Content is a 64bits microseconds value on a monotonic clock
 */
#define ADEC_ANCILLARY_KEY_OUTPUT_TIME "adec.output_time"


/* Forward declarations */
struct adec_decoder;


/* Supported decoder implementations */
enum adec_decoder_implem {
	/* Automatically select decoder */
	ADEC_DECODER_IMPLEM_AUTO = 0,

	/* Fraunhofer FDK AAC encoder */
	ADEC_DECODER_IMPLEM_FDK_AAC,

	/* End of supported implementation */
	ADEC_DECODER_IMPLEM_MAX,
};


/* Decoder initial configuration, implementation specific extension
 * Each implementation might provide implementation specific configuration with
 * a structure compatible with this base structure (i.e. which starts with the
 * same implem field). */
struct adec_config_impl {
	/* Decoder implementation for this extension */
	enum adec_decoder_implem implem;
};


/* Decoder initial configuration */
struct adec_config {
	/* Decoder instance name (optional, can be null, copied internally) */
	const char *name;

	/* Decoder implementation (AUTO means no preference,
	 * use the default implementation for the platform) */
	enum adec_decoder_implem implem;

	/* Encoding type */
	enum adef_encoding encoding;

	/* Input buffer pool preferred minimum buffer count, used
	 * only if the implementation uses its own input buffer pool
	 * (0 means no preference, use the default value) */
	unsigned int preferred_min_in_buf_count;

	/* Output buffer pool preferred minimum buffer count
	 * (0 means no preference, use the default value) */
	unsigned int preferred_min_out_buf_count;

	/* Preferred decoding thread count (0 means no preference,
	 * use the default value; 1 means no multi-threading;
	 * only relevant for CPU decoding implementations) */
	unsigned int preferred_thread_count;

	/* Favor low delay decoding (e.g. for a live stream) */
	int low_delay;

	/* Preferred output buffers data format (optional, 0 means any) */
	struct adef_format preferred_output_format;

	/* Implementation specific extensions (optional, can be NULL)
	 * If not null, implem_cfg must match the following requirements:
	 *  - this->implem_cfg->implem == this->implem
	 *  - this->implem != ADEC_DECODER_IMPLEM_AUTO
	 *  - The real type of implem_cfg must be the implementation specific
	 *    structure, not struct adec_config_impl */
	struct adec_config_impl *implem_cfg;
};


/* Decoder callback functions */
struct adec_cbs {
	/* Frame output callback function (mandatory).
	 * The library retains ownership of the output frame and the
	 * application must reference it if needed after returning from the
	 * callback function. The status is 0 in case of success, a negative
	 * errno otherwise. In case of error no frame is output and frame
	 * is NULL. An error -EBADMSG means a resync is required (IDR frame).
	 * @param dec: decoder instance handle
	 * @param status: frame output status
	 * @param frame: output frame
	 * @param userdata: user data pointer */
	void (*frame_output)(struct adec_decoder *dec,
			     int status,
			     struct mbuf_audio_frame *frame,
			     void *userdata);

	/* Flush callback function, called when flushing is complete (optional).
	 * @param dec: decoder instance handle
	 * @param userdata: user data pointer */
	void (*flush)(struct adec_decoder *dec, void *userdata);

	/* Stop callback function, called when stopping is complete (optional).
	 * @param dec: decoder instance handle
	 * @param userdata: user data pointer */
	void (*stop)(struct adec_decoder *dec, void *userdata);
};


/**
 * ToString function for enum adec_decoder_implem.
 * @param implem: implementation value to convert
 * @return a string description of the implementation
 */
ADEC_API const char *adec_decoder_implem_str(enum adec_decoder_implem implem);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* !_ADEC_CORE_H_ */
