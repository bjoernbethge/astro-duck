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
#    (build/release/src/libduckdb.dll), but Windows' Secure DLL Search
#    Mode ignores PATH for dependent DLLs and only searches the .exe's
#    own directory and CWD. Copy libduckdb.dll next to unittest.exe
#    before invoking it (cp -u so we only touch it once).
ifeq ($(OS),Windows_NT)
    UNITTEST_BIN := test/unittest.exe
    define STAGE_TEST_DLLS
	@cp -u build/$(1)/src/libduckdb.dll build/$(1)/test/ 2>/dev/null || true
    endef
else
    UNITTEST_BIN := test/unittest
    define STAGE_TEST_DLLS
    endef
endif

test_release_internal:
	$(call STAGE_TEST_DLLS,release)
	./build/release/$(UNITTEST_BIN) "test/*"
test_debug_internal:
	$(call STAGE_TEST_DLLS,debug)
	./build/debug/$(UNITTEST_BIN) "test/*"
test_reldebug_internal:
	$(call STAGE_TEST_DLLS,reldebug)
	./build/reldebug/$(UNITTEST_BIN) "test/*"
