
LOCAL_PATH := $(call my-dir)

# API library. This is the library that most programs should use.
include $(CLEAR_VARS)
LOCAL_MODULE := libaudio-decode
LOCAL_CATEGORY_PATH := libs
LOCAL_DESCRIPTION := Audio decoding library
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include
LOCAL_CFLAGS := -DADEC_API_EXPORTS -fvisibility=hidden -std=gnu99 -D_GNU_SOURCE
LOCAL_SRC_FILES := \
	src/adec.c
LOCAL_LIBRARIES := \
	libaudio-decode-core \
	libaudio-defs \
	libpomp \
	libulog
LOCAL_CONFIG_FILES := config.in
$(call load-config)
LOCAL_CONDITIONAL_LIBRARIES := \
	CONFIG_ADEC_FDK_AAC:libaudio-decode-fdk-aac
LOCAL_EXPORT_LDLIBS := -laudio-decode-core

ifeq ("$(TARGET_OS)","windows")
  LOCAL_LDLIBS += -lws2_32
endif

include $(BUILD_LIBRARY)

include $(CLEAR_VARS)

# Core library, common code for all implementations and structures definitions.
# Used by implementations.
LOCAL_MODULE := libaudio-decode-core
LOCAL_CATEGORY_PATH := libs
LOCAL_DESCRIPTION := Audio decoding library: core files
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/core/include
LOCAL_CFLAGS := -DADEC_API_EXPORTS -fvisibility=hidden -std=gnu99 -D_GNU_SOURCE
LOCAL_SRC_FILES := \
	core/src/adec_enums.c \
	core/src/adec_format.c
LOCAL_LIBRARIES := \
	libaudio-defs \
	libfutils \
	libmedia-buffers \
	libmedia-buffers-memory \
	libulog


ifeq ("$(TARGET_OS)","windows")
  LOCAL_LDLIBS += -lws2_32
endif

include $(BUILD_LIBRARY)

include $(CLEAR_VARS)

# FDK AAC implementation. can be enabled in the product configuration
LOCAL_MODULE := libaudio-decode-fdk-aac
LOCAL_CATEGORY_PATH := libs
LOCAL_DESCRIPTION := Video decoding library: FDK AAC implementation
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/fdk-aac/include
LOCAL_CFLAGS := -DADEC_API_EXPORTS -fvisibility=hidden -std=gnu99 -D_GNU_SOURCE
LOCAL_SRC_FILES := \
	fdk-aac/src/adec_fdk_aac.c
LOCAL_LIBRARIES := \
	fdk-aac \
	libaudio-decode-core \
	libaudio-defs \
	libfutils \
	libmedia-buffers \
	libmedia-buffers-memory \
	libmedia-buffers-memory-generic \
	libpomp \
	libulog

ifeq ("$(TARGET_OS)","windows")
  LOCAL_LDLIBS += -lws2_32
endif

include $(BUILD_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := adec
LOCAL_DESCRIPTION := Audio decoding program
LOCAL_CATEGORY_PATH := multimedia
LOCAL_SRC_FILES := tools/adec.c
LOCAL_LIBRARIES := \
	libaac \
	libaudio-decode \
	libaudio-defs \
	libaudio-raw \
	libfutils \
	libmedia-buffers \
	libmedia-buffers-memory \
	libmedia-buffers-memory-generic \
	libpomp \
	libulog

ifeq ("$(TARGET_OS)","windows")
  LOCAL_LDLIBS += -lws2_32
endif

include $(BUILD_EXECUTABLE)
