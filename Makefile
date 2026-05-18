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
test_release_internal:
	@echo "--- stage Windows DLLs (debug) ---"
	@echo "src DLLs:" && ls -la build/release/src/*.dll 2>&1 || true
	-cp -f build/release/src/libduckdb.dll build/release/test/
	-cp -f C:/mingw64/bin/libgcc_s_seh-1.dll build/release/test/
	-cp -f C:/mingw64/bin/libstdc++-6.dll build/release/test/
	-cp -f C:/mingw64/bin/libwinpthread-1.dll build/release/test/
	@echo "test/ after staging:" && ls -la build/release/test/ 2>&1 || true
	./build/release/test/unittest.exe "test/*"
test_debug_internal:
	-cp -f build/debug/src/libduckdb.dll build/debug/test/
	-cp -f C:/mingw64/bin/libgcc_s_seh-1.dll build/debug/test/
	-cp -f C:/mingw64/bin/libstdc++-6.dll build/debug/test/
	-cp -f C:/mingw64/bin/libwinpthread-1.dll build/debug/test/
	./build/debug/test/unittest.exe "test/*"
test_reldebug_internal:
	-cp -f build/reldebug/src/libduckdb.dll build/reldebug/test/
	-cp -f C:/mingw64/bin/libgcc_s_seh-1.dll build/reldebug/test/
	-cp -f C:/mingw64/bin/libstdc++-6.dll build/reldebug/test/
	-cp -f C:/mingw64/bin/libwinpthread-1.dll build/reldebug/test/
	./build/reldebug/test/unittest.exe "test/*"
else
test_release_internal:
	./build/release/test/unittest "test/*"
test_debug_internal:
	./build/debug/test/unittest "test/*"
test_reldebug_internal:
	./build/reldebug/test/unittest "test/*"
endif
