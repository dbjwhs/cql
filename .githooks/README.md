# CQL Git Hooks

This directory contains custom git hooks for the CQL project to enforce code quality and consistency.

## Available Hooks

### pre-commit
**Purpose:** Enforces EOF newlines on all committed text files

**What it does:**
- Checks all staged files for proper EOF newlines
- Blocks commit if any files are missing EOF newlines
- Provides helpful instructions for fixing issues
- Supports the POSIX standard requiring newlines at end of text files

**Supported file types:**
- C/C++ source files (`.cpp`, `.hpp`, `.c`, `.h`, etc.)
- Python files (`.py`, `.pyx`, `.pyi`)
- Documentation (`.md`, `.rst`, `.txt`)
- Configuration files (`.json`, `.yaml`, `.yml`, etc.)
- Build files (`CMakeLists.txt`, `Makefile`)
- And many more text file types

## Installation

### Automatic Installation (Recommended)
```bash
# From project root
./scripts/install_git_hooks.sh
```

### Manual Installation
```bash
# From project root
cp .githooks/pre-commit .git/hooks/pre-commit
chmod +x .git/hooks/pre-commit
```

## Usage

Once installed, the hooks run automatically:

- **pre-commit**: Runs every time you `git commit`
- No additional action needed - just commit normally
- If issues are found, the commit will be blocked with helpful instructions

## Fixing EOF Newline Issues

If the pre-commit hook reports missing EOF newlines:

```bash
# Option 1: Fix all files automatically
python3 scripts/check_eof_newline.py --fix

# Option 2: Fix with backup files
python3 scripts/check_eof_newline.py --fix --backup

# Option 3: Fix only staged files
python3 scripts/check_eof_newline.py --fix --filter-from-file <temp_file>

# Then re-stage and commit
git add -u
git commit
```

## Bypassing Hooks (Not Recommended)

In rare cases where you need to bypass hooks:
```bash
git commit --no-verify
```

**⚠️ Warning:** Only use `--no-verify` in exceptional circumstances. The hooks are designed to maintain code quality and POSIX compliance.

## Why EOF Newlines Matter

- **POSIX Standard:** Text files should end with a newline character
- **Tool Compatibility:** Many Unix tools expect files to end with newlines
- **Git Behavior:** Git shows "No newline at end of file" warnings
- **Consistency:** Ensures uniform formatting across the codebase

## Troubleshooting

### Hook Not Running
- Verify installation: `ls -la .git/hooks/pre-commit`
- Check permissions: `chmod +x .git/hooks/pre-commit`
- Ensure Python 3 is available: `python3 --version`

### Script Not Found Error
- Verify the EOF checker exists: `ls -la scripts/check_eof_newline.py`
- Run from project root directory

### Permission Denied
- Make hooks executable: `chmod +x .git/hooks/*`
- Check file ownership and permissions

## Development

To add new hooks:
1. Create the hook script in `.githooks/`
2. Make it executable: `chmod +x .githooks/new-hook`
3. Update this README
4. Run `./scripts/install_git_hooks.sh` to install
5. Test the hook thoroughly

## Integration with CI/CD

The EOF newline checker can also be used in CI/CD pipelines:

```yaml
# Example GitHub Actions step
- name: Check EOF newlines
  run: python3 scripts/check_eof_newline.py --check
```

This ensures the same standards are enforced both locally and in automated builds.
