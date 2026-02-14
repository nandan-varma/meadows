# Meadows CLI Tools

A collection of Linux command-line utilities written in the Meadows programming language, demonstrating the use of the Meadows standard library.

## Building

```bash
cd examples/cli
make
```

## Available Commands

### ls - List directory contents
```bash
./ls.out              # List current directory
./ls.out -a           # Show all files (including hidden)
./ls.out /path/to/dir # List specific directory
```

### cat - Concatenate and display files
```bash
./cat.out file.txt           # Display file contents
./cat.out file1.txt file2.txt # Display multiple files
```

### echo - Display a line of text
```bash
./echo.out Hello World       # Print arguments
./echo.out -n Hello          # Print without newline (limited)
```

### head - Output the first part of files
```bash
./head.out file.txt          # First 10 lines
./head.out -n 5 file.txt     # First 5 lines
```

### tail - Output the last part of files
```bash
./tail.out file.txt          # Last 10 lines
./tail.out -n 5 file.txt     # Last 5 lines
```

### wc - Print newline, word, and byte counts
```bash
./wc.out file.txt            # Lines, words, bytes
./wc.out -l file.txt         # Line count only
./wc.out -w file.txt         # Word count only
./wc.out -c file.txt         # Byte count only
```

## Architecture

```
cli/
├── Makefile              # Build script
├── README.md            # This file
├── commands/
│   ├── ls.ms           # List directory
│   ├── cat.ms          # Concatenate files
│   ├── echo.ms         # Print arguments
│   ├── head.ms         # First N lines
│   ├── tail.ms         # Last N lines
│   └── wc.ms           # Word count
└── src/                # Shared modules
    └── (uses stdlib modules)
```

## Standard Library Modules Used

- `std.dir` - Directory operations (opendir, readdir, closedir)
- `std.io` - File I/O (fopen, fread, fwrite, fgets, etc.)
- `std.string` - String operations (strlen, strcmp, etc.)
- `std.args` - Command-line argument parsing
- `std.os` - Operating system functions (exit, getenv)

## Adding New Commands

1. Create a new `.ms` file in `commands/`
2. Import required stdlib modules
3. Implement `main()` function
4. Add to Makefile
5. Test with `make test`

## Notes

- All commands follow Unix exit code conventions (0 = success, non-zero = error)
- Error messages follow standard Unix format
- Some advanced features may be limited compared to GNU coreutils
