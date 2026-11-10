# QNX VitalPulse RT — Cross-Platform Makefile (QNX SDP 8.0 qcc + Host POSIX clang/gcc)
#
# Usage on QNX SDP 8.0 (after sourcing qnxsdp-env.sh):
#   make qnx QNX_ARCH=aarch64le
#
# Usage on macOS / Linux Host for Real-Time Benchmark & Studio Integration:
#   make host

QNX_ARCH ?= aarch64le
QCC      := qcc
QCFLAGS  := -Vgcc_nto$(QNX_ARCH) -Wall -Wextra -O2 -I./include
HOST_CC  := cc
HCFLAGS  := -Wall -Wextra -O2 -pthread -I./include

BUILD_DIR := ./bin

.PHONY: all host qnx clean test

all: host

host: $(BUILD_DIR)/vitalpulse_icu_benchmark

$(BUILD_DIR)/vitalpulse_icu_benchmark: src/vitalpulse_icu_benchmark.c include/vitalpulse_ipc.h
	@mkdir -p $(BUILD_DIR)
	$(HOST_CC) $(HCFLAGS) src/vitalpulse_icu_benchmark.c -o $(BUILD_DIR)/vitalpulse_icu_benchmark

qnx:
	@mkdir -p $(BUILD_DIR)
	$(QCC) $(QCFLAGS) src/vitalpulse_resmgr.c src/vitalpulse_ham_guardian.c -lha -o $(BUILD_DIR)/vitalpulse_resmgr
	$(QCC) $(QCFLAGS) src/vitalpulse_icu_benchmark.c -o $(BUILD_DIR)/vitalpulse_icu_benchmark

test: host
	$(BUILD_DIR)/vitalpulse_icu_benchmark vtach_priority_inversion

clean:
	rm -rf $(BUILD_DIR)
