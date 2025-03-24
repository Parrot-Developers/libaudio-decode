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

#define ULOG_TAG adec
#include "adec_priv.h"
ULOG_DECLARE_TAG(adec);


static atomic_int s_instance_counter;
static pthread_once_t instance_counter_is_init = PTHREAD_ONCE_INIT;
static void initialize_instance_counter(void)
{
	atomic_init(&s_instance_counter, 0);
}


static const struct adec_ops *implem_ops(enum adec_decoder_implem implem)
{
	switch (implem) {
#ifdef BUILD_LIBAUDIO_DECODE_FDK_AAC
	case ADEC_DECODER_IMPLEM_FDK_AAC:
		return &adec_fdk_aac_ops;
#endif
	default:
		return NULL;
	}
}


static int adec_get_implem(enum adec_decoder_implem *implem)
{
	ULOG_ERRNO_RETURN_ERR_IF(implem == NULL, EINVAL);

#ifdef BUILD_LIBAUDIO_DECODE_FDK_AAC
	if ((*implem == ADEC_DECODER_IMPLEM_AUTO) ||
	    (*implem == ADEC_DECODER_IMPLEM_FDK_AAC)) {
		*implem = ADEC_DECODER_IMPLEM_FDK_AAC;
		return 0;
	}
#endif /* BUILD_LIBAUDIO_DECODE_FDK_AAC */

	return -ENOSYS;
}


int adec_get_supported_input_formats(enum adec_decoder_implem implem,
				     const struct adef_format **formats)
{
	int ret;
	ULOG_ERRNO_RETURN_ERR_IF(!formats, EINVAL);

	ret = adec_get_implem(&implem);
	if (ret < 0) {
		ULOG_ERRNO("adec_get_implem", -ret);
		return ret;
	}

	return implem_ops(implem)->get_supported_input_formats(formats);
}


enum adec_decoder_implem adec_get_auto_implem(void)
{
	int ret;
	enum adec_decoder_implem implem = ADEC_DECODER_IMPLEM_AUTO;

	ret = adec_get_implem(&implem);
	ULOG_ERRNO_RETURN_VAL_IF(ret < 0, -ret, ADEC_DECODER_IMPLEM_AUTO);

	return implem;
}


enum adec_decoder_implem
adec_get_auto_implem_by_coded_format(struct adef_format *format)
{
	int res = 0, count;
	const struct adef_format *supported_input_formats;

	ULOG_ERRNO_RETURN_VAL_IF(
		format == NULL, EINVAL, ADEC_DECODER_IMPLEM_AUTO);

	for (enum adec_decoder_implem implem = ADEC_DECODER_IMPLEM_AUTO + 1;
	     implem < ADEC_DECODER_IMPLEM_MAX;
	     implem++) {

		res = adec_get_implem(&implem);
		if (res < 0)
			continue;

		count = implem_ops(implem)->get_supported_input_formats(
			&supported_input_formats);
		if (count < 0)
			continue;

		if (!adef_format_intersect(
			    format, supported_input_formats, count))
			continue;

		return implem;
	}

	return ADEC_DECODER_IMPLEM_AUTO;
}


int adec_new(struct pomp_loop *loop,
	     const struct adec_config *config,
	     const struct adec_cbs *cbs,
	     void *userdata,
	     struct adec_decoder **ret_obj)
{
	int ret;
	struct adec_decoder *self = NULL;

	(void)pthread_once(&instance_counter_is_init,
			   initialize_instance_counter);

	ADEC_LOG_ERRNO_RETURN_ERR_IF(loop == NULL, EINVAL);
	ADEC_LOG_ERRNO_RETURN_ERR_IF(config == NULL, EINVAL);
	ADEC_LOG_ERRNO_RETURN_ERR_IF(cbs == NULL, EINVAL);
	ADEC_LOG_ERRNO_RETURN_ERR_IF(cbs->frame_output == NULL, EINVAL);
	ADEC_LOG_ERRNO_RETURN_ERR_IF(ret_obj == NULL, EINVAL);

	self = calloc(1, sizeof(*self));
	if (self == NULL)
		return -ENOMEM;

	self->base = self; /* For logging */
	self->loop = loop;
	self->cbs = *cbs;
	self->userdata = userdata;
	self->config = *config;
	self->config.name = xstrdup(config->name);
	atomic_init(&self->last_timestamp, UINT64_MAX);
	self->dec_id = (atomic_fetch_add(&s_instance_counter, 1) + 1);

	if (self->config.name != NULL)
		ret = asprintf(&self->dec_name, "%s", self->config.name);
	else
		ret = asprintf(&self->dec_name, "%02d", self->dec_id);
	if (ret < 0) {
		ret = -ENOMEM;
		ADEC_LOG_ERRNO("asprintf", -ret);
		goto error;
	}

	ret = adec_get_implem(&self->config.implem);
	if (ret < 0)
		goto error;

	self->ops = implem_ops(self->config.implem);

	ret = self->ops->create(self);
	if (ret < 0)
		goto error;

	*ret_obj = self;
	return 0;

error:
	adec_destroy(self);
	*ret_obj = NULL;
	return ret;
}


int adec_flush(struct adec_decoder *self, int discard)
{
	ADEC_LOG_ERRNO_RETURN_ERR_IF(self == NULL, EINVAL);

	return self->ops->flush(self, discard);
}


int adec_stop(struct adec_decoder *self)
{
	ADEC_LOG_ERRNO_RETURN_ERR_IF(self == NULL, EINVAL);

	return self->ops->stop(self);
}


int adec_destroy(struct adec_decoder *self)
{
	int ret = 0;

	if (self == NULL)
		return 0;

	ret = self->ops->destroy(self);

	ADEC_LOGI("adec instance stats: [%u [%u %u] %u]",
		  self->counters.in,
		  self->counters.pushed,
		  self->counters.pulled,
		  self->counters.out);

	if (ret == 0) {
		xfree((void **)&self->dec_name);
		xfree((void **)&self->config.name);
		free(self);
	}

	return ret;
}


int adec_set_aac_asc(struct adec_decoder *self,
		     const uint8_t *asc,
		     size_t asc_size,
		     enum adef_aac_data_format data_format)
{
	int ret;

	ADEC_LOG_ERRNO_RETURN_ERR_IF(self == NULL, EINVAL);
	ADEC_LOG_ERRNO_RETURN_ERR_IF(self->configured, EALREADY);

	if (self->ops->set_aac_asc)
		ret = self->ops->set_aac_asc(self, asc, asc_size, data_format);
	else
		ret = -ENOSYS;

	self->configured = (ret == 0) ? 1 : 0;

	return ret;
}


struct mbuf_pool *adec_get_input_buffer_pool(struct adec_decoder *self)
{
	ADEC_LOG_ERRNO_RETURN_VAL_IF(self == NULL, EINVAL, NULL);

	return self->ops->get_input_buffer_pool(self);
}


struct mbuf_audio_frame_queue *
adec_get_input_buffer_queue(struct adec_decoder *self)
{
	ADEC_LOG_ERRNO_RETURN_VAL_IF(self == NULL, EINVAL, NULL);

	return self->ops->get_input_buffer_queue(self);
}


enum adec_decoder_implem adec_get_used_implem(struct adec_decoder *self)
{
	ADEC_LOG_ERRNO_RETURN_VAL_IF(
		self == NULL, EINVAL, ADEC_DECODER_IMPLEM_AUTO);

	return self->config.implem;
}
