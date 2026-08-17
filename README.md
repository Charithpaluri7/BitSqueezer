***

```markdown
# Huffman File Compressor

A fast, command-line file compression utility built in C++ using the Huffman Coding algorithm. It operates on raw binary data, allowing it to safely compress and decompress any file type—from plain text to images and executables.

## ✨ Features

- **Binary-Safe I/O:** Handles any file format (`.txt`, `.jpg`, `.pdf`, `.exe`) without corrupting data.
- **Bit-Level Packing:** Squeezes data into exact bits rather than whole bytes for maximum compression.
- **Smart Padding:** Automatically tracks and discards leftover padding bits to guarantee perfect restoration.
- **Robust Validation:** Checks file headers, verifies frequency tables, and catches corrupted files safely without crashing.
- **Efficient Memory:** Uses `shared_ptr` for automatic memory management and `priority_queue` for fast tree building.

## 🛠️ How to Compile

You need a C++17 compatible compiler (like GCC/g++). Run the following command in your terminal:

```bash
g++ main.cpp -o huffman
```

*(On Windows, this will create `huffman.exe`)*

## 🚀 How to Use

The utility takes exactly three arguments: `mode`, `input file`, and `output file`.

### 1. Compress a File
```bash
./huffman compress input.txt compressed.huf
```

### 2. Decompress a File
```bash
./huffman decompress compressed.huf restored.txt
```

## 📊 Example Output

When you run a command, the utility prints clear statistics about the process:

```text
--------------------------------------------------
 Compression Statistics
--------------------------------------------------
  Input file        : input.txt
  Output file       : compressed.huf
  Original size     : 10600006 bytes
  Compressed size   : 3814568 bytes
  Space saved       : 6785438 bytes
  Reduction         : 64.01 %
  Distinct symbols  : 5 / 256
  Pad bits          : 2
--------------------------------------------------
```

## ⚠️ Note on Already-Compressed Files
If you try to compress a file that is already compressed (like a `.zip`, `.mp3`, or `.jpg`), the output `.huf` file might be slightly larger than the original. This is expected! These formats already remove redundant data, so Huffman coding cannot shrink them further, and the tool simply adds its small header overhead.
