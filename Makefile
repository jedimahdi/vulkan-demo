MAKEFLAGS += --no-print-directory

CC := gcc
TIDY := clang-tidy
VALGRIND := valgrind
CALLGRIND := valgrind --tool=callgrind

SRC_DIR := src
BUILD_DIR := build
THIRD_BUILD_DIR := $(BUILD_DIR)/thirdparty
TARGET := main

BUILD ?= DEBUG

INCLUDES := \
	-I./thirdparty/stb

BASE_CFLAGS := -std=c11 -Wall -Wextra -Wno-unused -MMD -MP $(INCLUDES)
BASE_LFLAGS := -lglfw -lm -lvulkan
THIRD_CFLAGS := -std=c11 -O2 $(INCLUDES)

ifeq ($(BUILD),DEBUG)
    OPT_CFLAGS := -g -O0
    OPT_LFLAGS :=
else ifeq ($(BUILD),RELEASE)
    OPT_CFLAGS := -O2
    OPT_LFLAGS :=
else ifeq ($(BUILD),PROFILE)
    OPT_CFLAGS := -pg -g -O2
    OPT_LFLAGS := -pg
else ifeq ($(BUILD),PERF)
    OPT_CFLAGS := -g -O2
    OPT_LFLAGS :=
else ifeq ($(BUILD),SANITIZE)
    OPT_CFLAGS := -g -O1 -fsanitize=address,undefined
    OPT_LFLAGS := -fsanitize=address,undefined
else
    $(error Unknown build type '$(BUILD)'. Use one of: DEBUG RELEASE PROFILE PERF SANITIZE)
endif

CFLAGS := $(BASE_CFLAGS) $(OPT_CFLAGS)
LFLAGS := $(BASE_LFLAGS) $(OPT_LFLAGS)

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))
DEPS := $(OBJS:.o=.d)

THIRD_IMPLS := $(THIRD_BUILD_DIR)/stb_image_impl.c
THIRD_OBJS := $(THIRD_IMPLS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS) $(THIRD_OBJS)
	$(CC) $^ $(LFLAGS) -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(THIRD_BUILD_DIR)/%.o: $(THIRD_BUILD_DIR)/%.c
	$(CC) $(THIRD_CFLAGS) -c $< -o $@

$(THIRD_BUILD_DIR)/stb_image_impl.c: thirdparty/stb/stb_image.h | $(THIRD_BUILD_DIR)
	@echo "#define STB_IMAGE_IMPLEMENTATION 1" > $@
	@echo "#include <stb_image.h>" >> $@

-include $(DEPS)

$(BUILD_DIR) $(THIRD_BUILD_DIR):
	@mkdir -p $@

clean-objs:
	rm -rf $(OBJS) $(DEPS)

clean:
	rm -rf $(BUILD_DIR) $(TARGET) gmon.out profile.txt callgrind.out.* perf.data perf.data.old

tidy:
	@for f in $(SRCS); do $(TIDY) $$f -- $(CFLAGS) || exit 1; done

valgrind: clean-objs
	$(MAKE) all BUILD=DEBUG
	$(VALGRIND) --leak-check=full ./$(TARGET)

callgrind: clean-objs
	$(MAKE) all BUILD=PERF
	$(CALLGRIND) ./$(TARGET)
	@echo "🔍 Use 'callgrind_annotate callgrind.out.*' to view results"

gprof: clean-objs
	$(MAKE) all BUILD=PROFILE
	@echo "Run the binary to generate gmon.out, then:"
	@echo "gprof $(TARGET) gmon.out > profile.txt && less profile.txt"

perf: clean-objs
	$(MAKE) all BUILD=PERF
	@echo "Run with: perf record ./$(TARGET) && perf report"

sanitize: clean-objs
	$(MAKE) all BUILD=SANITIZE
	ASAN_OPTIONS=detect_leaks=0 ./$(TARGET)

compile_commands.json: clean
	bear -- make all

.PHONY: all clean clean-objs tidy valgrind callgrind gprof perf sanitize
