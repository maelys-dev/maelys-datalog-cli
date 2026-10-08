CC ?= cc
CMAKE ?= cmake
BUILD_DIR ?= build
PROFILE ?= SMALL
PREFIX ?= /usr/local
SANITIZE_FLAGS ?=

ifeq ($(filter $(PROFILE),SMALL LARGE),)
$(error PROFILE must be SMALL or LARGE)
endif

DEPS = $(MAELYS_DEPENDENCIES_DIR)
ENGINE_DIR = $(DEPS)/maelys-datalog
CLI_DIR = $(DEPS)/maelys-cli
JSON_DIR = $(DEPS)/maelys-json
SPEC_DIR = $(DEPS)/agent-cli-spec
SDK_PREFIX = $(abspath $(BUILD_DIR))/sdk
ENGINE_CMAKE_BUILD = $(abspath $(BUILD_DIR))/engine-cmake
ENGINE_LIB = $(SDK_PREFIX)/lib/libmaelys_datalog.a
CLI_LIB = $(abspath $(BUILD_DIR))/deps/maelys-cli/lib/libmaelys_cli.a
JSON_LIB = $(abspath $(BUILD_DIR))/deps/maelys-json/lib/libmaelys-json.a
BIN = $(BUILD_DIR)/bin/maelys-datalog
SCHEMAS = $(wildcard cli/schemas/*.json)
SCHEMA_SYMBOLS = $(foreach schema,$(SCHEMAS),datalog_$(basename $(notdir $(schema)))_schema=$(schema))
SOURCES = cli/main.c cli/domain.c cli/facts.c
OBJECTS = $(SOURCES:cli/%.c=$(BUILD_DIR)/obj/%.o)
PROFILE_CFLAGS = $(if $(filter LARGE,$(PROFILE)),-DMAELYS_DATALOG_PROFILE_LARGE,)
ENGINE_LARGE = $(if $(filter LARGE,$(PROFILE)),ON,OFF)
CLI_CFLAGS = -std=c11 -Wall -Wextra -Werror -g $(SANITIZE_FLAGS) \
    -I$(SDK_PREFIX)/include -I$(CLI_DIR)/include -I$(JSON_DIR)/include \
    -I$(BUILD_DIR)/generated $(PROFILE_CFLAGS) \
    -DDATALOG_CLI_VERSION='"$(shell cat VERSION)"'

.PHONY: all check check-dependencies check-engine-contract check-cli-contract \
    check-json-contract check-spec-contract cli-test conformance-check \
    installed-sdk-check asan-cli install clean
all: $(BIN)

check-dependencies:
	@test -n "$(MAELYS_DEPENDENCIES_DIR)" || { \
		echo "MAELYS_DEPENDENCIES_DIR is unset; run 'sh scripts/checkout-dependencies.sh DIR' and export the line it prints" >&2; exit 1; }

define check_pin
	@test "$$(git -C "$(1)" rev-parse HEAD)" = "$$(sed -n 2p dependencies/$(2).pin)"
	@test "$$(git -C "$(1)" rev-parse "$$(sed -n 1p dependencies/$(2).pin)^{}")" = "$$(sed -n 2p dependencies/$(2).pin)"
	@git -C "$(1)" diff --quiet HEAD --
	@git -C "$(1)" diff --cached --quiet HEAD --
	@test -z "$$(git -C "$(1)" ls-files --others --exclude-standard)"
endef

check-engine-contract: check-dependencies
	$(call check_pin,$(ENGINE_DIR),maelys-datalog)
	@test "$$(cat "$(ENGINE_DIR)/VERSION")" = \
		"$$(sed -n '1s/^v//p' dependencies/maelys-datalog.pin)"
	@test -f "$(ENGINE_DIR)/include/maelys/datalog.h"

check-cli-contract: check-dependencies
	$(call check_pin,$(CLI_DIR),maelys-cli)
	@grep -Fq '#define MAELYS_CLI_ABI 1' "$(CLI_DIR)/include/maelys/cli/version.h"
	@grep -Fq '#define MAELYS_CLI_CONTRACT "agent-cli/v2"' "$(CLI_DIR)/include/maelys/cli/version.h"

check-json-contract: check-dependencies
	$(call check_pin,$(JSON_DIR),maelys-json)
	@grep -Fq '#define MAELYS_JSON_ABI_VERSION 3u' "$(JSON_DIR)/include/maelys/json.h"
	@cmp dependencies/maelys-json.pin "$(CLI_DIR)/dependencies/maelys-json.pin"

check-spec-contract: check-dependencies
	$(call check_pin,$(SPEC_DIR),agent-cli-spec)
	@cmp dependencies/agent-cli-spec.pin "$(CLI_DIR)/dependencies/agent-cli-spec.pin"

$(ENGINE_LIB): dependencies/maelys-datalog.pin | check-engine-contract
	@mkdir -p $(BUILD_DIR)
	$(CMAKE) -S "$(ENGINE_DIR)" -B "$(ENGINE_CMAKE_BUILD)" \
		-DCMAKE_INSTALL_PREFIX="$(SDK_PREFIX)" \
		-DCMAKE_C_FLAGS="$(SANITIZE_FLAGS)" \
		-DMAELYS_DATALOG_PROFILE_LARGE=$(ENGINE_LARGE)
	$(CMAKE) --build "$(ENGINE_CMAKE_BUILD)" --target maelys_datalog --parallel 4
	$(CMAKE) --install "$(ENGINE_CMAKE_BUILD)" --component sdk-static
	$(CMAKE) --install "$(ENGINE_CMAKE_BUILD)" --component sdk

$(CLI_LIB): dependencies/maelys-cli.pin | check-cli-contract
	$(MAKE) -C "$(CLI_DIR)" CPPFLAGS= BUILD=$(abspath $(BUILD_DIR))/deps/maelys-cli \
		CFLAGS='-O1 -g $(SANITIZE_FLAGS)' LDFLAGS='$(SANITIZE_FLAGS)' $@

$(JSON_LIB): dependencies/maelys-json.pin | check-json-contract
	$(MAKE) -C "$(JSON_DIR)" CPPFLAGS= BUILD=$(abspath $(BUILD_DIR))/deps/maelys-json \
		CFLAGS='-O1 -g $(SANITIZE_FLAGS)' LDFLAGS='$(SANITIZE_FLAGS)' $@

$(BUILD_DIR)/generated/datalog_schemas.c: $(SCHEMAS) | $(CLI_LIB)
	@mkdir -p $(dir $@)
	$(CLI_DIR)/tools/maelys-cli-embed $(SCHEMA_SYMBOLS) > $@

$(BUILD_DIR)/generated/datalog_schemas.h: $(SCHEMAS) | $(CLI_LIB)
	@mkdir -p $(dir $@)
	$(CLI_DIR)/tools/maelys-cli-embed --header $(SCHEMA_SYMBOLS) > $@

$(BUILD_DIR)/obj/%.o: cli/%.c cli/reader.h $(BUILD_DIR)/generated/datalog_schemas.h | $(ENGINE_LIB) $(CLI_LIB) $(JSON_LIB)
	@mkdir -p $(dir $@)
	$(CC) $(CLI_CFLAGS) -c $< -o $@

$(BUILD_DIR)/obj/main.o: VERSION

$(BUILD_DIR)/obj/datalog_schemas.o: $(BUILD_DIR)/generated/datalog_schemas.c
	@mkdir -p $(dir $@)
	$(CC) $(CLI_CFLAGS) -c $< -o $@

$(BIN): $(OBJECTS) $(BUILD_DIR)/obj/datalog_schemas.o $(ENGINE_LIB) $(CLI_LIB) $(JSON_LIB)
	@mkdir -p $(dir $@)
	$(CC) $(SANITIZE_FLAGS) $^ -o $@

cli-test: $(BIN) check-spec-contract
	python3 cli/tests/test_cli.py $(abspath $(BIN)) "$(SPEC_DIR)"

conformance-check: $(BIN) check-spec-contract
	python3 "$(SPEC_DIR)/conformance/run.py" $(abspath $(BIN))

$(BUILD_DIR)/bin/sdk-policy-lifetime: cli/tests/sdk_policy_lifetime.c | $(ENGINE_LIB)
	@mkdir -p $(dir $@)
	$(CC) -std=c11 -D_POSIX_C_SOURCE=200112L -Wall -Wextra -Werror \
		$(PROFILE_CFLAGS) $(SANITIZE_FLAGS) -I$(SDK_PREFIX)/include $< $(ENGINE_LIB) -o $@

$(BUILD_DIR)/bin/sdk-reservations: cli/tests/sdk_reservations.c cli/tests/sdk_allocation_guard.h | $(ENGINE_LIB)
	@mkdir -p $(dir $@)
	$(CC) -std=c11 -Wall -Wextra -Werror $(PROFILE_CFLAGS) $(SANITIZE_FLAGS) \
		-I$(SDK_PREFIX)/include $< $(ENGINE_LIB) -o $@

.PHONY: sdk-allocation-check
sdk-allocation-check: check-engine-contract
	$(CMAKE) -S "$(ENGINE_DIR)" -B "$(abspath $(BUILD_DIR))/guard-engine" \
		-DBUILD_TESTING=OFF -DMAELYS_DATALOG_PROFILE_LARGE=$(ENGINE_LARGE) \
		-DCMAKE_INSTALL_PREFIX="$(abspath $(BUILD_DIR))/guard-sdk" \
		-DCMAKE_C_FLAGS="-include $(abspath cli/tests/sdk_allocation_guard.h) $(SANITIZE_FLAGS)"
	$(CMAKE) --build "$(abspath $(BUILD_DIR))/guard-engine" --target maelys_datalog --parallel 4
	$(CMAKE) --install "$(abspath $(BUILD_DIR))/guard-engine" --component sdk-static
	$(CMAKE) --install "$(abspath $(BUILD_DIR))/guard-engine" --component sdk
	@mkdir -p $(BUILD_DIR)/bin
	$(CC) -std=c11 -Wall -Wextra -Werror $(PROFILE_CFLAGS) $(SANITIZE_FLAGS) \
		-DCLI_SDK_ALLOC_GUARDED -I$(abspath $(BUILD_DIR))/guard-sdk/include \
		cli/tests/sdk_reservations.c $(abspath $(BUILD_DIR))/guard-sdk/lib/libmaelys_datalog.a \
		-o $(BUILD_DIR)/bin/sdk-reservations-guarded
	$(BUILD_DIR)/bin/sdk-reservations-guarded

installed-sdk-check: $(BIN) $(BUILD_DIR)/bin/sdk-policy-lifetime $(BUILD_DIR)/bin/sdk-reservations
	$(BUILD_DIR)/bin/sdk-policy-lifetime
	$(BUILD_DIR)/bin/sdk-reservations
	@test -f "$(SDK_PREFIX)/include/maelys/datalog.h"
	@test -f "$(SDK_PREFIX)/lib/libmaelys_datalog.a"
	@$(BIN) check --domain cli/tests/fixtures/rbac.domain.json \
		cli/tests/fixtures/rbac.dl --format json --compact >/dev/null

check: check-engine-contract check-cli-contract check-json-contract \
    check-spec-contract cli-test conformance-check installed-sdk-check

asan-cli:
	$(MAKE) BUILD_DIR=build/asan-$(PROFILE) PROFILE=$(PROFILE) \
		SANITIZE_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer -O1' cli-test

install: $(BIN)
	install -d "$(DESTDIR)$(PREFIX)/bin"
	install -m 755 "$(BIN)" "$(DESTDIR)$(PREFIX)/bin/maelys-datalog"
	install -d "$(DESTDIR)$(PREFIX)/share/doc/maelys-datalog"
	install -m 644 LICENSE CHANGELOG.md docs/cli.md docs/cli-contract.json \
		docs/specifications/maelys-datalog-domain-v1.md \
		"$(DESTDIR)$(PREFIX)/share/doc/maelys-datalog/"

clean:
	rm -rf "$(BUILD_DIR)"
