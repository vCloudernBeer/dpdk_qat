#   BSD LICENSE
#
#   Copyright(c) 2010-2013 Intel Corporation. All rights reserved.
#   All rights reserved.
#
#   Redistribution and use in source and binary forms, with or without
#   modification, are permitted provided that the following conditions
#   are met:
#
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in
#       the documentation and/or other materials provided with the
#       distribution.
#     * Neither the name of Intel Corporation nor the names of its
#       contributors may be used to endorse or promote products derived
#       from this software without specific prior written permission.
#
#   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
#   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

ifeq ($(RTE_SDK),)
$(error "Please define RTE_SDK environment variable")
endif

ifeq ($(ICP_ROOT),)
$(error "Please define ICP_ROOT environment variable")
endif

# Default target, can be overriden by command line or environment
RTE_TARGET ?= x86_64-native-linuxapp-gcc

include $(RTE_SDK)/mk/rte.vars.mk

ifneq ($(CONFIG_RTE_EXEC_ENV),"linuxapp")
$(error This application can only operate in a linuxapp environment, \
please change the definition of the RTE_TARGET environment variable)
endif

LBITS := $(shell uname -p)
ifeq ($(CROSS_COMPILE),)
    ifneq ($(CONFIG_RTE_ARCH),"x86_64")
        ifneq ($(LBITS),i686)
        $(error The RTE_TARGET chosen is not compatible with this environment \
        (x86_64), for this application. Please change the definition of the \
        RTE_TARGET environment variable, or run the application on a i686 OS)
        endif
    endif
endif

# binary name
APP = dpdk_qat

# all source are stored in SRCS-y
SRCS-y := main.c crypto.c

CFLAGS += -O3
CFLAGS += $(WERROR_FLAGS)
CFLAGS += -I$(ICP_ROOT)/quickassist/include \
		-I$(ICP_ROOT)/quickassist/include/lac \
		-I$(ICP_ROOT)/quickassist/lookaside/access_layer/include

# From CRF 1.2 driver, library was renamed to libicp_qa_al.a
ifneq ($(wildcard $(ICP_ROOT)/build/icp_qa_al.a),)
ICP_LIBRARY_PATH = $(ICP_ROOT)/build/icp_qa_al.a
else
ICP_LIBRARY_PATH = $(ICP_ROOT)/build/libicp_qa_al.a
endif

LDLIBS += -L$(ICP_ROOT)/build
LDLIBS += $(ICP_LIBRARY_PATH) \
                -lz \
                -losal \
                -ladf_proxy \
                -lcrypto

# workaround for a gcc bug with noreturn attribute
# http://gcc.gnu.org/bugzilla/show_bug.cgi?id=12603
ifeq ($(CONFIG_RTE_TOOLCHAIN_GCC),y)
CFLAGS_main.o += -Wno-return-type
endif

include $(RTE_SDK)/mk/rte.extapp.mk
