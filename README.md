# 2D Graphics Editor

A terminal-based 2D graphics editor written in C.
Shapes are drawn using `*` and background is filled with `_`.

## Features
- Draw Circle, Rectangle, Line and Triangle
- Add, Delete and Modify shapes
- 2D array canvas to store the picture
- Simple menu-driven interface

## Requirements
- MSYS2 MinGW64
- GCC

## How to compile
```bash
gcc graphics_editor.c -o graphics_editor.exe -lm
```

## How to run
```bash
./graphics_editor.exe
```

## How to use
- Choose **1** to Add a shape
- Choose **2** to Delete a shape
- Choose **3** to Modify a shape
- Choose **4** to List all shapes
- Choose **5** to Display the canvas
- Choose **6** to Clear the canvas
- Choose **0** to Exit

## Canvas size
- 20 rows x 50 columns
- Supports up to 50 objects