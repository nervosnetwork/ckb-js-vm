# Unit tests in TypeScript

## How to Run

```
pnpm test
```

## Structure Overview

The test suite is organized within the `src` folder, containing multiple sub-folders. Each sub-folder represents a distinct
unit test suite with the following components:

### File Types

- **TypeScript Source Files** (`.ts` or `.js`)

  - Contains the on-chain script implementation
  - Core logic to be tested

- **Test Files** (`.test.ts`)

  - Jest test files executed on native machine
  - Contains test cases and assertions

- **Build Configuration** (`build.cjs`)
  - Handles compilation of JavaScript to QuickJS bytecode
  - Manages bundling with esbuild
  - Handles file system packaging
  - Auto-executes during import (no manual execution needed)

- **File System Assets** (`data/*`, `data2/*`, ...)
  - Contains files for packing into Simple File System
  - Supports multiple file types:
    - Data files
    - JavaScript source code
    - Compiled bytecode
  - Used for testing file system operations and script execution
