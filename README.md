# PDF Zip Processor

A C program that processes ZIP files containing PDFs and generates metadata text files for each PDF. This tool extracts all PDFs from a ZIP file and creates accompanying .txt files containing metadata such as file size and modification date.

## Features

- Extracts PDF files from ZIP archives
- Creates a separate directory for extracted files
- Generates metadata files for each PDF containing:
  - Original filename
  - File size in bytes
  - File modification date
- Windows-compatible (built with MinGW-w64)
- Handles both forward and backward slashes in paths
- Includes error handling for file operations

## Prerequisites

- MinGW-w64 compiler
- libzip library

## Installation

1. Install MSYS2 if you haven't already:
   - Download from: https://www.msys2.org/
   - Follow the installation instructions on their website

2. Open MSYS2 MinGW64 terminal and install required packages:
```bash
pacman -S mingw-w64-x86_64-gcc
pacman -S mingw-w64-x86_64-libzip
```

3. Clone this repository:
```bash
git clone https://github.com/badcircle/zpt.git
cd zpt
```

4. Compile the program:
```bash
gcc -o zpt.exe zpt.c -lzip -I/mingw64/include -L/mingw64/lib
```

5. Make sure libzip DLL is in your PATH or copy it to the program directory:
```bash
cp /mingw64/bin/libzip*.dll .
```

## Usage

```bash
zpt.exe <path_to_zip_file>
```

Example:
```bash
zpt.exe documents.zip
```

This will:
1. Create a directory named 'documents' (same as zip file name without extension)
2. Extract all PDFs from the ZIP file into this directory
3. Create a .txt file for each PDF containing its metadata

The metadata text files will have the same name as their corresponding PDFs but with a .txt extension. Each text file contains a single line with pipe-separated values in this format:
```
filename.pdf|filesize_in_bytes|modification_date
```

## Error Handling

The program includes error checking for:
- Invalid command-line arguments
- ZIP file opening failures
- Memory allocation errors
- File reading/writing errors
- Invalid file paths

Error messages are printed to stderr with specific details about the failure.

## Development

### Project Structure

```
zpt/
├── program.c          # Main source code
├── README.md         # This file
└── .gitignore       # Git ignore file
```

### Building from Source

For development, you might want to add debugging symbols:
```bash
gcc -g -o zpt.exe zpt.c -lzip -I/mingw64/include -L/mingw64/lib
```

## Contributing

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add some amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Acknowledgments

- libzip library for ZIP file handling
- MinGW-w64 project for Windows compilation support
