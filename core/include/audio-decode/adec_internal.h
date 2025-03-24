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

#ifndef _ADEC_INTERNAL_H_
#define _ADEC_INTERNAL_H_

#include <stdatomic.h>
#include <stdio.h>

#include <audio-decode/adec_core.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* To be used for all public API */
#ifdef ADEC_API_EXPORTS
#	ifdef _WIN32
#		define ADEC_INTERNAL_API __declspec(dllexport)
#	else /* !_WIN32 */
#		define ADEC_INTERNAL_API __attribute__((visibility("default")))
#	endif /* !_WIN32 */
#else /* !ADEC_API_EXPORTS */
#	define ADEC_INTERNAL_API
#endif /* !ADEC_API_EXPORTS */


/* Specific logging functions : log the instance ID before the log message */
#define ADEC_LOG_INT(_pri, _fmt, ...)                                          \
	do {                                                                   \
		char *prefix = (self != NULL && self->base != NULL)            \
				       ? self->base->dec_name                  \
				       : "";                                   \
		ULOG_PRI(_pri,                                                 \
			 "%s%s" _fmt,                                          \
			 prefix != NULL ? prefix : "",                         \
			 prefix != NULL ? ": " : "",                           \
			 ##__VA_ARGS__);                                       \
	} while (0)
#define ADEC_LOGD(_fmt, ...) ADEC_LOG_INT(ULOG_DEBUG, _fmt, ##__VA_ARGS__)
#define ADEC_LOGI(_fmt, ...) ADEC_LOG_INT(ULOG_INFO, _fmt, ##__VA_ARGS__)
#define ADEC_LOGW(_fmt, ...) ADEC_LOG_INT(ULOG_WARN, _fmt, ##__VA_ARGS__)
#define ADEC_LOGE(_fmt, ...) ADEC_LOG_INT(ULOG_ERR, _fmt, ##__VA_ARGS__)
#define ADEC_LOG_ERRNO(_fmt, _err, ...)                                        \
	do {                                                                   \
		char *prefix = (self != NULL && self->base != NULL)            \
				       ? self->base->dec_name                  \
				       : "";                                   \
		ULOGE_ERRNO((_err),                                            \
			    "%s%s" _fmt,                                       \
			    prefix != NULL ? prefix : "",                      \
			    prefix != NULL ? ": " : "",                        \
			    ##__VA_ARGS__);                                    \
	} while (0)
#define ADEC_LOGW_ERRNO(_fmt, _err, ...)                                       \
	do {                                                                   \
		char *prefix = (self != NULL && self->base != NULL)            \
				       ? self->base->dec_name                  \
				       : "";                                   \
		ULOGW_ERRNO((_err),                                            \
			    "%s%s" _fmt,                                       \
			    prefix != NULL ? prefix : "",                      \
			    prefix != NULL ? ": " : "",                        \
			    ##__VA_ARGS__);                                    \
	} while (0)
#define ADEC_LOG_ERRNO_RETURN_IF(_cond, _err)                                  \
	do {                                                                   \
		if (ULOG_UNLIKELY(_cond)) {                                    \
			ADEC_LOG_ERRNO("", (_err));                            \
			return;                                                \
		}                                                              \
	} while (0)
#define ADEC_LOG_ERRNO_RETURN_ERR_IF(_cond, _err)                              \
	do {                                                                   \
		if (ULOG_UNLIKELY(_cond)) {                                    \
			int __pdraw_errno__err = (_err);                       \
			ADEC_LOG_ERRNO("", (__pdraw_errno__err));              \
			return -(__pdraw_errno__err);                          \
		}                                                              \
	} while (0)
#define ADEC_LOG_ERRNO_RETURN_VAL_IF(_cond, _err, _val)                        \
	do {                                                                   \
		if (ULOG_UNLIKELY(_cond)) {                                    \
			ADEC_LOG_ERRNO("", (_err));                            \
			/* codecheck_ignore[RETURN_PARENTHESES] */             \
			return (_val);                                         \
		}                                                              \
	} while (0)


struct adec_ops {
	int (*get_supported_input_formats)(const struct adef_format **formats);

	int (*create)(struct adec_decoder *base);

	int (*flush)(struct adec_decoder *base, int discard);

	int (*stop)(struct adec_decoder *base);

	int (*destroy)(struct adec_decoder *base);

	int (*set_aac_asc)(struct adec_decoder *base,
			   const uint8_t *asc,
			   size_t asc_size,
			   enum adef_aac_data_format data_format);

	struct mbuf_pool *(*get_input_buffer_pool)(struct adec_decoder *base);

	struct mbuf_audio_frame_queue *(*get_input_buffer_queue)(
		struct adec_decoder *base);
};


struct audio_info {
	/* TODO */
};


struct adec_decoder {
	/* Reserved */
	struct adec_decoder *base;
	void *derived;
	const struct adec_ops *ops;
	struct pomp_loop *loop;
	struct adec_cbs cbs;
	void *userdata;
	struct adec_config config;
	int configured;
	struct audio_info audio_info;

	int dec_id;
	char *dec_name;

	union {
		/* TODO */
	} reader;
	atomic_uint_least64_t last_timestamp;

	struct {
		/* Frames that have passed the input filter */
		unsigned int in;
		/* Frames that have been pushed to the decoder */
		unsigned int pushed;
		/* Frames that have been pulled from the decoder */
		unsigned int pulled;
		/* Frames that have been output (frame_output) */
		unsigned int out;
	} counters;
};


ADEC_INTERNAL_API void
adec_call_frame_output_cb(struct adec_decoder *base,
			  int status,
			  struct mbuf_audio_frame *frame);


ADEC_INTERNAL_API void adec_call_flush_cb(struct adec_decoder *base);


ADEC_INTERNAL_API void adec_call_stop_cb(struct adec_decoder *base);


/**
 * Default filter for the input frame queue.
 * This function is intended to be used as a standalone input filter.
 * It will call adec_default_input_filter_internal(), and then
 * adec_default_input_filter_internal_confirm_frame() if the former returned
 * true.
 *
 * @param frame: The frame to filter.
 * @param userdata: The adec_decoder structure.
 *
 * @return true if the frame passes the checks, false otherwise
 */
ADEC_API bool adec_default_input_filter(struct mbuf_audio_frame *frame,
					void *userdata);

/**
 * Default filter for the input frame queue.
 * This filter does the following checks:
 * - frame is in a supported format
 * - frame timestamp is strictly monotonic
 * This version is intended to be used by custom filters, to avoid calls to
 * mbuf_audio_frame_get_frame_info() or get_supported_input_formats().
 *
 * @warning This function does NOT check input validity. Arguments must not be
 * NULL, except for supported_formats if nb_supported_formats is zero.
 *
 * @param decoder: The base video decoder.
 * @param frame: The frame to filter.
 * @param frame_info: The associated adef_frame.
 * @param supported_formats: The formats supported by the implementation.
 * @param nb_supported_formats: The size of the supported_formats array.
 *
 * @return true if the frame passes the checks, false otherwise
 */
ADEC_INTERNAL_API bool
adec_default_input_filter_internal(struct adec_decoder *decoder,
				   struct mbuf_audio_frame *frame,
				   struct adef_frame *frame_info,
				   const struct adef_format *supported_formats,
				   unsigned int nb_supported_formats);

/**
 * Filter update function.
 * This function should be called at the end of a custom filter. It registers
 * that the frame was accepted. This function saves the frame timestamp for
 * monotonic checks, and sets the ADEC_ANCILLARY_KEY_INPUT_TIME ancillary data
 * on the frame.
 *
 * @param decoder: The base video decoder.
 * @param frame: The accepted frame.
 * @param frame_info: The associated adef_frame.
 */
ADEC_INTERNAL_API void
adec_default_input_filter_internal_confirm_frame(struct adec_decoder *decoder,
						 struct mbuf_audio_frame *frame,
						 struct adef_frame *frame_info);


ADEC_INTERNAL_API struct adec_config_impl *
adec_config_get_specific(struct adec_config *config,
			 enum adec_decoder_implem implem);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* !_ADEC_INTERNAL_H_ */
