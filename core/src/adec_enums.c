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
ULOG_DECLARE_TAG(ULOG_TAG);


const char *adec_decoder_implem_str(enum adec_decoder_implem implem)
{
	switch (implem) {
	case ADEC_DECODER_IMPLEM_FDK_AAC:
		return "FDK_AAC";
	default:
		return "UNKNOWN";
	}
}


struct adec_config_impl *
adec_config_get_specific(struct adec_config *config,
			 enum adec_decoder_implem implem)
{
	/* Check if specific config is present */
	if (!config->implem_cfg)
		return NULL;

	/* Check if implementation is the right one */
	if (config->implem != implem) {
		ULOGI("specific config found, but implementation is %s "
		      "instead of %s. ignoring specific config",
		      adec_decoder_implem_str(config->implem),
		      adec_decoder_implem_str(implem));
		return NULL;
	}

	/* Check if specific config implementation matches the base one */
	if (config->implem_cfg->implem != config->implem) {
		ULOGW("specific config implem (%s) does not match"
		      " base config implem (%s). ignoring specific config",
		      adec_decoder_implem_str(config->implem_cfg->implem),
		      adec_decoder_implem_str(config->implem));
		return NULL;
	}

	/* All tests passed, return specific config */
	return config->implem_cfg;
}
