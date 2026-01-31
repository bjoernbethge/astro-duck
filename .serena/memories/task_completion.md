# Task Completion Checklist

## After Code Changes

### 1. Build
```bash
make clean && make release
```

### 2. Test
```bash
python test_astro.py
```

### 3. For New Functions
1. Declare in `src/include/astro.hpp` (if public interface)
2. Implement in `src/astro.cpp`
3. Register in `LoadInternal()` using `CreateScalarFunction`
4. Add tests to `test_astro.py`
5. Update `README.md` function table

### 4. Before Commit
- Ensure build succeeds
- Ensure all tests pass
- Update documentation if adding features

## CI Notes
- CI runs automatically on push
- Skips build for documentation-only changes (*.md, docs/**, LICENSE, .gitignore, *.txt)
- Deploy workflow triggers on version tags (v*)
