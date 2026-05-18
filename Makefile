PROJ_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

# Configuration of extension
EXT_NAME=astro
EXT_CONFIG=${PROJ_DIR}extension_config.cmake

# Include the Makefile from extension-ci-tools
include extension-ci-tools/makefiles/duckdb_extension.Makefile

# --- Override test recipes ---
# Two upstream issues, both surfacing on the GitHub Actions windows-latest
# image after a runner update; the v1.4.1 (2026-04-18) build worked, later
# builds did not:
#
# 1. extension-ci-tools/makefiles/duckdb_extension.Makefile (pin: ec20f45)
#    builds the test command from quoted variables
#      TEST_PATH="/test/unittest"
#      TESTS_BASE_DIRECTORY = "test/"
#    which expand to `./build/release/"/test/unittest" ""test/"*"`. The
#    explicit form below is more robust on every platform.
#
# 2. The Windows unittest.exe is dynamically linked against libduckdb.dll
#    (build/release/src/libduckdb.dll), but Windows only searches the
#    .exe's own directory and CWD. Without libduckdb.dll on the loader's
#    search path, unittest.exe silently fails to start (exit 127, no
#    output). Prepend build/release/src to PATH for the test run.
ifeq ($(OS),Windows_NT)
    UNITTEST_BIN := test/unittest.exe
    TEST_PREFIX := PATH="$$(pwd)/build/release/src:$$PATH"
else
    UNITTEST_BIN := test/unittest
    TEST_PREFIX :=
endif

test_release_internal:
	$(TEST_PREFIX) ./build/release/$(UNITTEST_BIN) "test/*"
test_debug_internal:
	$(TEST_PREFIX) ./build/debug/$(UNITTEST_BIN) "test/*"
test_reldebug_internal:
	$(TEST_PREFIX) ./build/reldebug/$(UNITTEST_BIN) "test/*"
