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

#define ULOG_TAG adec_core
#include "adec_core_priv.h"
#include <futils/timetools.h>


void adec_call_frame_output_cb(struct adec_decoder *base,
			       int status,
			       struct mbuf_audio_frame *frame)
{
	if (!base->cbs.frame_output)
		return;

	base->cbs.frame_output(base, status, frame, base->userdata);
	if (status == 0 && frame != NULL)
		base->counters.out++;
}


void adec_call_flush_cb(struct adec_decoder *base)
{
	/* Reset last_timestamp */
	atomic_store(&base->last_timestamp, UINT64_MAX);

	/* Call the application callback */
	if (!base->cbs.flush)
		return;

	base->cbs.flush(base, base->userdata);
}


void adec_call_stop_cb(struct adec_decoder *base)
{
	if (!base->cbs.stop)
		return;

	base->cbs.stop(base, base->userdata);
}


bool adec_default_input_filter(struct mbuf_audio_frame *frame, void *userdata)
{
	int ret;
	bool accept;
	struct adec_decoder *decoder = userdata;
	const struct adef_format *supported_formats;
	struct adef_frame frame_info;

	if (!frame || !decoder)
		return false;

	ret = mbuf_audio_frame_get_frame_info(frame, &frame_info);
	if (ret != 0)
		return false;

	ret = decoder->ops->get_supported_input_formats(&supported_formats);
	if (ret < 0)
		return false;
	accept = adec_default_input_filter_internal(
		decoder, frame, &frame_info, supported_formats, ret);
	if (accept)
		adec_default_input_filter_internal_confirm_frame(
			decoder, frame, &frame_info);
	return accept;
}


bool adec_default_input_filter_internal(
	struct adec_decoder *decoder,
	struct mbuf_audio_frame *frame,
	struct adef_frame *frame_info,
	const struct adef_format *supported_formats,
	unsigned int nb_supported_formats)
{
	uint64_t last_timestamp;
	if (!adef_format_intersect(&frame_info->format,
				   supported_formats,
				   nb_supported_formats)) {
		ULOG_ERRNO(
			"unsupported format:"
			" " ADEF_FORMAT_TO_STR_FMT,
			EPROTO,
			ADEF_FORMAT_TO_STR_ARG(&frame_info->format));
		return false;
	}

	last_timestamp = atomic_load(&decoder->last_timestamp);

	if (frame_info->info.timestamp <= last_timestamp &&
	    last_timestamp != UINT64_MAX) {
		ULOG_ERRNO("non-strictly-monotonic timestamp (%" PRIu64
			   " <= %" PRIu64 ")",
			   EPROTO,
			   frame_info->info.timestamp,
			   last_timestamp);
		return false;
	}

	return true;
}


void adec_default_input_filter_internal_confirm_frame(
	struct adec_decoder *decoder,
	struct mbuf_audio_frame *frame,
	struct adef_frame *frame_info)
{
	int err;
	uint64_t ts_us;
	struct timespec cur_ts = {0, 0};

	/* Save frame timestamp to last_timestamp */
	uint_least64_t last_timestamp = frame_info->info.timestamp;
	atomic_store(&decoder->last_timestamp, last_timestamp);
	decoder->counters.in++;

	/* Set the input time ancillary data to the frame */
	time_get_monotonic(&cur_ts);
	time_timespec_to_us(&cur_ts, &ts_us);
	err = mbuf_audio_frame_add_ancillary_buffer(
		frame, ADEC_ANCILLARY_KEY_INPUT_TIME, &ts_us, sizeof(ts_us));
	if (err < 0)
		ULOG_ERRNO("mbuf_audio_frame_add_ancillary_buffer", -err);
}
