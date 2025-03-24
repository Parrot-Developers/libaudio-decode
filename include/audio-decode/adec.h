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

#ifndef _ADEC_H_
#define _ADEC_H_

#include <stdint.h>
#include <unistd.h>

#include <audio-decode/adec_core.h>
#include <audio-defs/adefs.h>
#include <libpomp.h>

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
 * Get the supported input buffer data formats for the given
 * decoder implementation.
 * Each implementation supports at least one input format,
 * and optionally more. All input buffers need to be in one of
 * the supported formats, otherwise they will be discarded.
 * The returned formats array is a static array whose size is the return value
 * of this function. If this function returns an error (negative errno value),
 * then the value of *formats is undefined.
 * @param implem: decoder implementation
 * @param formats: pointer to the supported formats list (output)
 * @return the size of the formats array, or a negative errno on error.
 */
ADEC_API int
adec_get_supported_input_formats(enum adec_decoder_implem implem,
				 const struct adef_format **formats);


/**
 * Get the implementation that will be chosen in case ADEC_DECODER_IMPLEM_AUTO
 * is used.
 * @return the decoder implementation, or ADEC_DECODER_IMPLEM_AUTO in case of
 * error
 */
ADEC_API enum adec_decoder_implem adec_get_auto_implem(void);


/**
 * Get an implementation for a given coded format
 * @param format: coded format to support
 * @return the decoder implementation, or ADEC_DECODER_IMPLEM_AUTO in case of
 * error
 */
ADEC_API enum adec_decoder_implem
adec_get_auto_implem_by_coded_format(struct adef_format *format);

/**
 * Create a decoder instance.
 * The configuration and callbacks structures must be filled.
 * The instance handle is returned through the ret_obj parameter.
 * When no longer needed, the instance must be freed using the
 * adec_destroy() function.
 * @param loop: event loop to use
 * @param config: decoder configuration
 * @param cbs: decoder callback functions
 * @param userdata: callback functions user data (optional, can be null)
 * @param ret_obj: decoder instance handle (output)
 * @return 0 on success, negative errno value in case of error
 */
ADEC_API int adec_new(struct pomp_loop *loop,
		      const struct adec_config *config,
		      const struct adec_cbs *cbs,
		      void *userdata,
		      struct adec_decoder **ret_obj);


/**
 * Flush the decoder.
 * This function flushes all queues and optionally discards all buffers
 * retained by the decoder. If the buffers are not discarded the frame
 * output callback is called for each frame when the decoding is complete.
 * The function is asynchronous and returns immediately. When flushing is
 * complete the flush callback function is called if defined. After flushing
 * the decoder new input buffers can still be queued but should start with a
 * synchronization frame (e.g. IDR frame or start of refresh).
 * @param self: decoder instance handle
 * @param discard: if null, all pending buffers are output, otherwise they
 *        are discarded
 * @return 0 on success, negative errno value in case of error
 */
ADEC_API int adec_flush(struct adec_decoder *self, int discard);


/**
 * Stop the decoder.
 * This function stops any running threads. The function is asynchronous and
 * returns immediately. When stopping is complete the stop callback function
 * is called if defined. After stopping the decoder no new input buffers
 * can be queued and the decoder instance must be freed using the
 * adec_destroy() function.
 * @param self: decoder instance handle
 * @return 0 on success, negative errno value in case of error
 */
ADEC_API int adec_stop(struct adec_decoder *self);


/**
 * Free a decoder instance.
 * This function frees all resources associated with a decoder instance.
 * @note this function blocks until all internal threads (if any) can be
 * joined; therefore the application should call adec_stop() and wait for
 * the stop callback function to be called before calling adec_destroy().
 * @param self: decoder instance handle
 * @return 0 on success, negative errno value in case of error
 */
ADEC_API int adec_destroy(struct adec_decoder *self);


/**
 * Set the AAC audio specific config for decoding.
 * This function must be called prior to decoding (i.e. pushing buffer into
 * the input queue) with the AAC Audio Specific Config (ASC). The ASC data will
 * be copied internally if necessary. The ownership of the ASC buffer stays with
 * the caller. It is the caller's responsibility to ensure that the instance is
 * configured to decode an AAC stream.
 * @param self decoder instance handle
 * @param[in] asc: pointer to the ASC data
 * @param[in] asc_size: ASC size
 * @param[in] format: AAC data format
 * @return 0 on success, negative errno value in case of error
 */
ADEC_API int adec_set_aac_asc(struct adec_decoder *self,
			      const uint8_t *asc,
			      size_t asc_size,
			      enum adef_aac_data_format format);


/**
 * Get the input buffer pool.
 * The input buffer pool is defined only for implementations that require
 * using input memories from the decoder's own pool. This function must
 * be called prior to decoding and if the returned value is not NULL the
 * input buffer pool should be used to get input memories. If the input
 * memories provided are not originating from the pool, they will be copied
 * resulting in a loss of performance.
 * @param self: decoder instance handle
 * @return a pointer on the input memory pool on success, NULL in case of
 * error or if no pool is used
 */
ADEC_API struct mbuf_pool *
adec_get_input_buffer_pool(struct adec_decoder *self);


/**
 * Get the input frame queue.
 * This function must be called prior to decoding and the input
 * frame queue must be used to push input frames for decoding.
 * @param self: decoder instance handle
 * @return a pointer on the input frame queue on success, NULL in case of error
 */
ADEC_API struct mbuf_audio_frame_queue *
adec_get_input_buffer_queue(struct adec_decoder *self);


/**
 * Get the decoder implementation used.
 * @param self: decoder instance handle
 * @return the decoder implementation used, or ADEC_DECODER_IMPLEM_AUTO
 * in case of error
 */
ADEC_API enum adec_decoder_implem
adec_get_used_implem(struct adec_decoder *self);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* !_ADEC_H_ */
