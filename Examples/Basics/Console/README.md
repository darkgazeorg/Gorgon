# Console Example

This example demonstrates usage of the `Gorgon::Utils::Console` subsystem.

## Supported features shown in this example

- **Color output** (safelist + RGB when available)
- **Text styling** (bold, underline, negative/inverted)
- **Cursor movement** (`GotoXY`) for drawing anywhere on the screen
- **Terminal sizing** (`GetSize`, `GetWidth`, `GetHeight`) to layout content
- **Screen clearing** (`ClearScreen`) and style reset (`Reset`)
- **Writing to console output stream** (`OutStream()`)

## How to run

Build this example using the existing CMake setup (project is added to `Examples/CMakeLists.txt`).

From the repository root:

```sh
cmake --build build --target ConsoleExample
./Examples/Basics/Console/Bin/ConsoleExample
```

On Windows, run the generated executable from the `Bin` folder.
