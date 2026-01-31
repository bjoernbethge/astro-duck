# Suggested Commands

## Serena Setup (for symbolic analysis)
```bash
# 1. Run build (generates compile_commands.json)
make release

# 2. Link compile_commands.json to root
ln -s build/release/compile_commands.json .

# 3. Restart Claude Code (reloads Serena MCP)
```


## Build Commands
```bash
make release              # Release build (outputs to build/release/)
make debug                # Debug build
make clean                # Clean build artifacts
```

## Test Commands
```bash
python test_astro.py      # Run Python integration test suite
```

## Build Output Location
- Extension binary: `build/release/extension/astro/astro.duckdb_extension`
- DuckDB binary: `build/release/duckdb`

## Loading Extension for Testing
```sql
-- Must use -unsigned flag and absolute path
LOAD '/absolute/path/to/build/release/extension/astro/astro.duckdb_extension';
```

## Git Commands (Windows)
```bash
git status
git add <file>
git commit -m "message"
git push
git checkout -b feature/name
```

## File System (Windows)
```bash
dir                       # List directory (or use ls in Git Bash)
type <file>               # Display file contents (or cat)
```

## CI Trigger
- Push to any branch triggers CI (except docs-only changes)
- Version tags (v*) trigger deployment workflow
