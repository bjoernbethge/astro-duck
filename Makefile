# Makefile für Astro Extension
.PHONY: all clean debug release test set_duckdb_version

# Build-Konfiguration
BUILD_TYPE ?= release
BUILD_DIR = build/astro_$(BUILD_TYPE)
EXTENSION_NAME = astro
SOURCE_FILE = src/astro.cpp

# DuckDB Version Configuration (Fix für Metadaten-Mismatch)
DUCKDB_VERSION ?= v1.3.0
DUCKDB_GIT_VERSION ?= v1.3.0
TARGET_DUCKDB_VERSION ?= v1.3.0
EXTENSION_VERSION ?= v1.0.0

# Parallelisierung (18 von 22 Kernen)
CORES ?= 18
MAKEFLAGS += -j$(CORES)

# Build-Optimierungen
export CMAKE_BUILD_PARALLEL_LEVEL=$(CORES)
export NINJA_STATUS="[%f/%t] "

# Compiler-Cache
export CCACHE_DIR=/tmp/ccache
export CCACHE_MAXSIZE=5G
export CC=ccache gcc
export CXX=ccache g++

# Linker-Optimierung (automatische Erkennung)
CUSTOM_LINKER := $(shell \
	if command -v mold >/dev/null 2>&1; then \
		echo "-fuse-ld=mold"; \
	elif command -v lld >/dev/null 2>&1; then \
		echo "-fuse-ld=lld"; \
	else \
		echo ""; \
	fi)

# Compiler-Optimierungen
export CMAKE_CXX_FLAGS=-O2 -DNDEBUG -pipe
export CMAKE_C_FLAGS=-O2 -DNDEBUG -pipe

# Export DuckDB Version für Build-System
export DUCKDB_VERSION
export DUCKDB_GIT_VERSION
export TARGET_DUCKDB_VERSION
export EXTENSION_VERSION

all: $(BUILD_TYPE)

# Setup ccache
setup-ccache:
	@ccache -M $(CCACHE_MAXSIZE) >/dev/null 2>&1 || true
	@ccache -z >/dev/null 2>&1 || true

# Release Build
release: setup-ccache
	@echo "🚀 Building Astro Extension (Release)..."
	@echo "   - Kerne: $(CORES)"
	@echo "   - DuckDB Version: $(DUCKDB_VERSION)"
	@echo "   - Extension Version: $(EXTENSION_VERSION)"
	@echo "   - ccache: $(shell ccache -s 2>/dev/null | grep 'cache size' || echo 'Initialisiert')"
	@if [ -n "$(CUSTOM_LINKER)" ]; then echo "   - Linker: $(CUSTOM_LINKER)"; fi
	@mkdir -p $(BUILD_DIR)
	@time cmake \
		-DEXTENSION_STATIC_BUILD=1 \
		-DDUCKDB_EXTENSION_CONFIGS="$(PWD)/extension_config.cmake" \
		-DCMAKE_BUILD_TYPE=Release \
		-DCUSTOM_LINKER="$(CUSTOM_LINKER)" \
		-DDUCKDB_VERSION="$(DUCKDB_VERSION)" \
		-DDUCKDB_GIT_VERSION="$(DUCKDB_GIT_VERSION)" \
		-DTARGET_DUCKDB_VERSION="$(TARGET_DUCKDB_VERSION)" \
		-DEXTENSION_VERSION="$(EXTENSION_VERSION)" \
		-S "./duckdb/" \
		-B "$(BUILD_DIR)"
	@echo "🔨 Kompilierung..."
	@time $(MAKE) -C "$(BUILD_DIR)" astro_loadable_extension
	@echo "✅ Astro Extension built successfully!"
	@ls -la "$(BUILD_DIR)/extension/astro/"
	@echo "📊 ccache Statistiken:"
	@ccache -s 2>/dev/null | grep -E "(cache hit|cache miss)" || echo "   Keine Statistiken verfügbar"
	@$(MAKE) optimize-extension

# Post-Build Optimierung mit gzip-Komprimierung (UPX funktioniert nicht mit DuckDB Extensions)
optimize-extension:
	@echo ""
	@echo "🗜️  Post-Build Optimierung..."
	@if [ -f "$(BUILD_DIR)/extension/astro/astro.duckdb_extension" ]; then \
		EXT_FILE="$(BUILD_DIR)/extension/astro/astro.duckdb_extension"; \
		EXT_DIR="$(BUILD_DIR)/extension/astro"; \
		ORIGINAL_SIZE=$$(stat -c%s "$$EXT_FILE"); \
		echo "   Original: $$(numfmt --to=iec $$ORIGINAL_SIZE)"; \
		\
		echo "   🗜️  Erstelle gzip-komprimierte Version..."; \
		if gzip -9 -c "$$EXT_FILE" > "$$EXT_DIR/astro.duckdb_extension.gz"; then \
			COMPRESSED_SIZE=$$(stat -c%s "$$EXT_DIR/astro.duckdb_extension.gz"); \
			RATIO=$$(echo "scale=1; $$COMPRESSED_SIZE * 100 / $$ORIGINAL_SIZE" | bc -l); \
			echo "   ✅ Gzip: $$(numfmt --to=iec $$COMPRESSED_SIZE) ($$RATIO%)"; \
			echo "   💾 Ersparnis: $$(numfmt --to=iec $$(($$ORIGINAL_SIZE - $$COMPRESSED_SIZE)))"; \
		else \
			echo "   ❌ Gzip-Komprimierung fehlgeschlagen"; \
		fi; \
		\
		echo "   📋 Verfügbare Dateien:"; \
		ls -lh "$$EXT_DIR"/*.duckdb_extension* | awk '{print "      " $$9 " (" $$5 ")"}'; \
	else \
		echo "   ❌ Extension nicht gefunden: $(BUILD_DIR)/extension/astro/astro.duckdb_extension"; \
		echo "   💡 Führe zuerst 'make release' aus"; \
	fi

# Debug Build
debug: setup-ccache
	@echo "🚀 Building Astro Extension (Debug)..."
	@mkdir -p $(BUILD_DIR)
	@cmake \
		-DEXTENSION_STATIC_BUILD=1 \
		-DDUCKDB_EXTENSION_CONFIGS="$(PWD)/extension_config.cmake" \
		-DCMAKE_BUILD_TYPE=Debug \
		-DCUSTOM_LINKER="$(CUSTOM_LINKER)" \
		-DDUCKDB_VERSION="$(DUCKDB_VERSION)" \
		-DDUCKDB_GIT_VERSION="$(DUCKDB_GIT_VERSION)" \
		-DTARGET_DUCKDB_VERSION="$(TARGET_DUCKDB_VERSION)" \
		-DEXTENSION_VERSION="$(EXTENSION_VERSION)" \
		-S "./duckdb/" \
		-B "$(BUILD_DIR)"
	@$(MAKE) -C "$(BUILD_DIR)" astro_loadable_extension
	@echo "✅ Astro Extension (Debug) built successfully!"

# Test
test: release
	@echo "🧪 Testing Astro Extension..."
	@if [ -f "$(BUILD_DIR)/extension/astro/astro.duckdb_extension" ]; then \
		echo "Extension file found: $(BUILD_DIR)/extension/astro/astro.duckdb_extension"; \
		echo "⚠️  DuckDB Binary nicht verfügbar für automatischen Test"; \
		echo "✅ Extension erfolgreich gebaut und bereit zum Laden"; \
	else \
		echo "❌ Extension file not found!"; \
		exit 1; \
	fi

# Clean
clean:
	@echo "🧹 Cleaning Astro build..."
	@rm -rf build/astro_*
	@echo "✅ Astro build cleaned!"

# Hilfe
help:
	@echo "Astro Extension Makefile"
	@echo "======================="
	@echo "Targets:"
	@echo "  release  - Build release version (default)"
	@echo "  debug    - Build debug version"
	@echo "  test     - Build and test extension"
	@echo "  clean    - Clean build files"
	@echo "  help     - Show this help"
	@echo ""
	@echo "Usage:"
	@echo "  make -f Makefile.astro"
	@echo "  make -f Makefile.astro test"
	@echo ""
	@echo "DuckDB Version: $(DUCKDB_VERSION)"

# Set DuckDB Version (für CI/CD)
set_duckdb_version:
	@echo "🔧 Setting DuckDB version to $(DUCKDB_GIT_VERSION)..."
	@cd duckdb && git fetch --tags
	@cd duckdb && git checkout $(DUCKDB_GIT_VERSION)
	@echo "✅ DuckDB version set to $(DUCKDB_GIT_VERSION)"
	@cd duckdb && git log --oneline -1 