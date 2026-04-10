CC ?= gcc
BUILD_DIR := build
BIN := $(BUILD_DIR)/herofand
TEST_BIN := $(BUILD_DIR)/test_config
VERSION := $(shell cat VERSION)
PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
UNITDIR ?= /etc/systemd/system

CSTD := -std=c17
WARN := -Wall -Wextra -Werror -Wpedantic -Wshadow -Wconversion -Wstrict-prototypes \
	-Wmissing-prototypes -Wformat=2 -Wundef -Wwrite-strings
OPT ?= -O2
CPPFLAGS := -Iinclude -D_POSIX_C_SOURCE=200809L -DHEROFAND_VERSION=\"$(VERSION)\"
CFLAGS ?= $(CSTD) $(WARN) $(OPT)
LDFLAGS ?=

SRC := \
	src/controller.c \
	src/config.c \
	src/main.c \
	src/policy.c \
	src/sysfs.c

OBJ := $(patsubst src/%.c,$(BUILD_DIR)/src/%.o,$(SRC))

TEST_SRC := \
	tests/test_main.c \
	tests/test_policy.c \
	tests/test_config.c \
	src/config.c \
	src/policy.c

TEST_OBJ := $(patsubst %.c,$(BUILD_DIR)/%.o,$(TEST_SRC))

.PHONY: all clean test format-check debug release print-version install install-bin install-unit

all: $(BIN)

debug: OPT := -O0 -g3
debug: clean all

release: OPT := -O2
release: clean all

$(BIN): $(OBJ)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(OBJ) $(LDFLAGS) -o $@

$(TEST_BIN): $(TEST_OBJ)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(TEST_OBJ) $(LDFLAGS) -o $@

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

test: $(TEST_BIN)
	./$(TEST_BIN)

print-version:
	@printf '%s\n' '$(VERSION)'

format-check:
	python3 scripts/format_check.py include src tests

clean:
	rm -rf $(BUILD_DIR)

install: install-bin install-unit

install-bin: $(BIN)
	install -d "$(DESTDIR)$(BINDIR)"
	install -m 0755 "$(BIN)" "$(DESTDIR)$(BINDIR)/herofand"

install-unit:
	install -d "$(DESTDIR)$(UNITDIR)"
	install -m 0644 systemd/herofand.service "$(DESTDIR)$(UNITDIR)/herofand.service"
