#############################################
#
#	USEVOLT uv_hal UNIT TESTS
#
# Host-side unit tests for the hardware independent parts of uv_hal.
#
# The suite is deliberately self contained: it supplies its own uv_hal_config.h
# (in config/) and its own stubs (in stubs/), so it can be built and run from any
# device repository, or from a bare uv_hal checkout, without depending on any
# particular application's configuration.
#
#	make			build and run every test
#	make build		build only
#	make run		run the already built binary
#	make san		build and run with AddressSanitizer + UBSanitizer
#	make clean		remove build artifacts
#
# A single test or group can be run by passing a substring filter:
#	make run T=hysteresis
#	./build/uv_hal_tests pid
#
##############################################


HALDIR := ..
BUILDDIR := build
BINARY := $(BUILDDIR)/uv_hal_tests

CC ?= gcc

INCLUDEDIRS := -I"config" \
				-I"." \
				-I"stubs" \
				-I"$(HALDIR)/inc" \
				-I"$(HALDIR)/inc/ui" \
				-I"$(HALDIR)/inc/output" \
				-I"$(HALDIR)/freertos/include" \
				-I"$(HALDIR)/freertos/portable/ThirdParty/GCC/Posix"

# The uv_hal modules under test. Only hardware independent modules belong here -
# see README.md for what is deliberately left out and why.
HAL_SOURCES := $(HALDIR)/src/uv_filters.c \
				$(HALDIR)/src/uv_pid.c \
				$(HALDIR)/src/uv_utilities.c \
				$(HALDIR)/src/uv_json.c \
				$(HALDIR)/src/uv_yaml.c \
				$(HALDIR)/src/canopen/canopen_sdo.c \
				$(HALDIR)/src/canopen/canopen_sdo_server.c \
				$(HALDIR)/src/canopen/canopen_sdo_client.c \
				$(HALDIR)/src/canopen/canopen_obj_dict.c

# Every test_*.c is picked up automatically, so adding a test file requires no
# makefile change. Test cases register themselves through the TEST() macro, so
# adding a test case requires no change anywhere else either.
TEST_SOURCES := uv_test.c $(wildcard test_*.c) $(wildcard stubs/*.c)

SOURCES := $(TEST_SOURCES) $(HAL_SOURCES)
OBJECTS := $(addprefix $(BUILDDIR)/,$(patsubst %.c,%.o,$(notdir $(SOURCES))))

# uv_memory.h insists on knowing the project and build name; the tests are not a
# firmware image, so they simply declare themselves.
CFLAGS := -std=gnu11 -g -O0 -DCONFIG_TARGET_LINUX=1 \
			-D__UV_PROJECT_NAME=uv_hal_tests \
			-D__UV_PROGRAM_VERSION=0 \
			-D__UV_APP_VERSION=\"tests\" $(INCLUDEDIRS)
# Warnings are kept non-fatal: the tests are not the place to enforce a warning
# policy on the HAL sources they compile.
CFLAGS += -Wall -Wno-unused-parameter -Wno-unused-function

# Appended by the caller (see the san target). Kept separate from CFLAGS so that
# a recursive make never has to re-expand CFLAGS through the shell.
EXTRA_CFLAGS ?=

LDFLAGS :=


.PHONY: all
all: run


.PHONY: build
build: $(BINARY)


# One explicit rule per source directory rather than a VPATH: VPATH is also
# searched for the *target*, so a stray build/ directory anywhere on the path
# would let make resolve an object to it and declare it up to date.
$(BUILDDIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -MMD -MP -c $< -o $@

$(BUILDDIR)/%.o: stubs/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -MMD -MP -c $< -o $@

$(BUILDDIR)/%.o: $(HALDIR)/src/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -MMD -MP -c $< -o $@

$(BUILDDIR)/%.o: $(HALDIR)/src/canopen/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -MMD -MP -c $< -o $@


$(BINARY): $(OBJECTS)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) $(OBJECTS) -o $@ $(LDFLAGS)


.PHONY: run
run: $(BINARY)
	@./$(BINARY) $(T)


# AddressSanitizer and UndefinedBehaviorSanitizer catch the class of defect that
# matters most in firmware - out of bounds accesses on fixed size buffers and
# signed overflow - which a plain assertion pass can miss entirely.
.PHONY: san
san:
	@$(MAKE) clean
	@$(MAKE) run EXTRA_CFLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" \
		LDFLAGS="-fsanitize=address,undefined"


.PHONY: clean
clean:
	@rm -rf $(BUILDDIR)


-include $(OBJECTS:.o=.d)
