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
# 2. The Windows unittest.exe is built with MinGW gcc and dynamically
#    linked against libduckdb.dll plus the MinGW runtime DLLs
#    (libgcc_s_seh-1.dll, libstdc++-6.dll, libwinpthread-1.dll). Windows'
#    Secure DLL Search Mode ignores PATH for dependent DLLs and only
#    resolves them from the .exe's own directory and CWD, so the loader
#    silently failed (exit 127, no output) without these DLLs staged.
#    Copy all four next to unittest.exe before invoking it.
ifeq ($(OS),Windows_NT)
# Stage libduckdb.dll (from build/<type>/src) and the MinGW runtime DLLs
# (from C:/mingw64/bin) next to unittest.exe; the leading '-' makes make
# tolerate a missing source (in case the build is reconfigured later).
define STAGE_WIN_DLLS
	-cp -f build/$(1)/src/libduckdb.dll build/$(1)/test/
	-cp -f C:/mingw64/bin/libgcc_s_seh-1.dll build/$(1)/test/
	-cp -f C:/mingw64/bin/libstdc++-6.dll build/$(1)/test/
	-cp -f C:/mingw64/bin/libwinpthread-1.dll build/$(1)/test/
endef

test_release_internal:
	$(call STAGE_WIN_DLLS,release)
	./build/release/test/unittest.exe "test/*"
test_debug_internal:
	$(call STAGE_WIN_DLLS,debug)
	./build/debug/test/unittest.exe "test/*"
test_reldebug_internal:
	$(call STAGE_WIN_DLLS,reldebug)
	./build/reldebug/test/unittest.exe "test/*"
else
test_release_internal:
	./build/release/test/unittest "test/*"
test_debug_internal:
	./build/debug/test/unittest "test/*"
test_reldebug_internal:
	./build/reldebug/test/unittest "test/*"
endif
