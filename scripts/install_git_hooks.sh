#!/bin/bash
# MIT License
# Copyright (c) 2025 dbjwhs

# Script to install custom git hooks from .githooks/ directory
# This ensures all developers use the same pre-commit checks

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Get script directory and project root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
GITHOOKS_DIR="$PROJECT_ROOT/.githooks"
GIT_HOOKS_DIR="$PROJECT_ROOT/.git/hooks"

echo -e "${BLUE}🔧 Installing CQL git hooks...${NC}"

# Check if we're in a git repository
if [ ! -d "$PROJECT_ROOT/.git" ]; then
    echo -e "${RED}❌ Error: Not in a git repository${NC}"
    echo -e "${YELLOW}💡 Run this script from the project root directory${NC}"
    exit 1
fi

# Check if .githooks directory exists
if [ ! -d "$GITHOOKS_DIR" ]; then
    echo -e "${RED}❌ Error: .githooks directory not found at $GITHOOKS_DIR${NC}"
    exit 1
fi

# Create .git/hooks directory if it doesn't exist
mkdir -p "$GIT_HOOKS_DIR"

# Install each hook
INSTALLED_COUNT=0
for hook_file in "$GITHOOKS_DIR"/*; do
    if [ -f "$hook_file" ]; then
        hook_name=$(basename "$hook_file")
        target_file="$GIT_HOOKS_DIR/$hook_name"
        
        # Check if hook already exists
        if [ -f "$target_file" ]; then
            echo -e "${YELLOW}⚠️  Hook $hook_name already exists, backing up...${NC}"
            mv "$target_file" "$target_file.backup.$(date +%s)"
        fi
        
        # Copy and make executable
        cp "$hook_file" "$target_file"
        chmod +x "$target_file"
        
        echo -e "${GREEN}✅ Installed $hook_name hook${NC}"
        ((INSTALLED_COUNT++))
    fi
done

if [ $INSTALLED_COUNT -eq 0 ]; then
    echo -e "${YELLOW}⚠️  No hooks found in $GITHOOKS_DIR${NC}"
    exit 0
fi

echo
echo -e "${GREEN}🎉 Successfully installed $INSTALLED_COUNT git hook(s)${NC}"
echo
echo -e "${BLUE}ℹ️  Installed hooks:${NC}"
for hook_file in "$GIT_HOOKS_DIR"/*; do
    if [ -f "$hook_file" ] && [ -x "$hook_file" ]; then
        hook_name=$(basename "$hook_file")
        case "$hook_name" in
            "pre-commit")
                echo -e "${BLUE}   📝 $hook_name - Enforces EOF newlines on committed files${NC}"
                ;;
            *)
                echo -e "${BLUE}   🔧 $hook_name${NC}"
                ;;
        esac
    fi
done

echo
echo -e "${YELLOW}💡 Next steps:${NC}"
echo -e "${YELLOW}   • These hooks will run automatically on git operations${NC}"
echo -e "${YELLOW}   • To test the pre-commit hook: git commit (it will check your staged files)${NC}"
echo -e "${YELLOW}   • To bypass hooks temporarily: git commit --no-verify${NC}"
echo
echo -e "${GREEN}✨ Git hooks are now active for this repository!${NC}"
