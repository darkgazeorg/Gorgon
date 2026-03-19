#include <Gorgon/Utils/Console.h>
#include <Gorgon/Graphics/Color.h>
#include <iostream>
#include <string>

using Gorgon::Utils::Console;

int main()
{
    // --- 1) Acquire a console ---
    // The Console class is a lightweight wrapper around a backend (stdin/stdout).
    // `StdConsole()` returns a shared instance that writes to `std::cout`.
    auto console = Gorgon::Utils::StdConsole();

    if (!console) {
        // If the backend could not be created, the console object evaluates to false.
        std::cerr << "Console backend unavailable\n";
        return 1;
    }

    // --- 2) Query supported features ---
    // ColorSupportLevel reports what kind of coloring can be used.
    // - None: no color support (plain text only)
    // - Safelist: a limited set of safe colors.
    // - RGB: arbitrary 24-bit colors are supported.
    auto colorSupport = console.ColorSupport();

    // All console features (color, styling, cursor movement) are provided by the backend.
    // We can still use simple output even if colors are unavailable.

    // --- 3) Get the current terminal size ---
    // This example uses the console size to draw a border and center the message.
    const auto size = console.GetSize();
    const int width  = size.Width;
    const int height = size.Height;

    // Print supported modes so user can see what this environment supports.
    std::cout << "Terminal size: " << width << "x" << height << "\n";
    std::cout << "Color support: ";
    switch (colorSupport) {
        case Console::None:     std::cout << "None"; break;
        case Console::Safelist: std::cout << "Safelist"; break;
        case Console::RGB:      std::cout << "RGB"; break;
    }
    std::cout << "\n";
    std::cout << "Styles supported: " << (console.IsStylesSupported() ? "Yes" : "No") << "\n";
    std::cout << "UTF-8 supported: " << (console.IsUTF8() ? "Yes" : "No") << "\n";

    std::cout << "\nPress <Enter> to draw the demo (will clear the screen)...";
    std::cin.get();

    // Choose border characters depending on UTF-8 support.
    const bool utf8 = console.IsUTF8();
    const std::string corner_tl = utf8 ? u8"┌" : "+";
    const std::string corner_tr = utf8 ? u8"┐" : "+";
    const std::string corner_bl = utf8 ? u8"└" : "+";
    const std::string corner_br = utf8 ? u8"┘" : "+";
    const std::string horizontal = utf8 ? u8"─" : "-";
    const std::string vertical = utf8 ? u8"│" : "|";

    // Clear the screen and move cursor to top-left.
    console.ClearScreen();
    console.GotoXY(0, 0);

    // --- 4) Draw a simple border using cursor movement ---
    // Note: if the terminal is too small, this may wrap. Use a terminal with at
    // least 40x10 to see the full effect.
    console.SetColor(Console::Color::Green);

    // Top and bottom edges
    for (int x = 1; x < width - 1; ++x) {
        console.GotoXY(x, 0);
        console.OutStream() << horizontal;
        console.GotoXY(x, height - 1);
        console.OutStream() << horizontal;
    }

    // Left and right edges
    for (int y = 1; y < height - 1; ++y) {
        console.GotoXY(0, y);
        console.OutStream() << vertical;
        console.GotoXY(width - 1, y);
        console.OutStream() << vertical;
    }

    // Corners
    console.GotoXY(0, 0);
    console.OutStream() << corner_tl;
    console.GotoXY(width - 1, 0);
    console.OutStream() << corner_tr;
    console.GotoXY(0, height - 1);
    console.OutStream() << corner_bl;
    console.GotoXY(width - 1, height - 1);
    console.OutStream() << corner_br;

    // --- 5) Draw styled text in the center ---
    const std::string title = "Console subsystem demo";
    const int titleX = (width - static_cast<int>(title.size())) / 2;
    const int titleY = height / 2 - 2;

    console.GotoXY(titleX, titleY);
    console.SetColor(Console::Color::Yellow);
    console.SetBold(true);
    console.OutStream() << title;

    // Underlined instructions
    const std::string instructions = "Press <Enter> to exit.";
    const int instX = (width - static_cast<int>(instructions.size())) / 2;
    const int instY = titleY + 2;

    console.GotoXY(instX, instY);
    console.SetColor(Console::Color::Cyan);
    console.SetUnderline(true);
    console.OutStream() << instructions;

    // Reset underline so the rest of the console isn't underlined.
    console.SetUnderline(false);

    // --- 6) Showcase RGB support (if available) ---
    console.GotoXY(2, height - 3);
    if (colorSupport == Console::RGB) {
        // RGB colors are only supported when the backend can map 24-bit values.
        console.SetColor(Gorgon::Graphics::RGBA{255, 160, 0, 255});
        console.OutStream() << "RGB colors supported!";
    }
    else if (colorSupport == Console::Safelist) {
        console.SetColor(Console::Color::Magenta);
        console.OutStream() << "Safelist colors supported (limited palette).";
    }
    else {
        console.SetColor(Console::Color::White);
        console.OutStream() << "No color support detected.";
    }

    // Demonstrate negative/inverted colors if supported
    if (console.IsStylesSupported()) {
        console.GotoXY(2, height - 2);
        console.SetNegative(true);
        console.OutStream() << "Negative/inverted style enabled.";
        console.SetNegative(false);
    }

    // --- 7) Wait for user input before exiting ---
    // The console is still in its styled state until Reset() is called.
    console.GotoXY(2, height - 1);
    console.SetColor(Console::Color::White);
    console.SetBold(false);
    console.OutStream() << "Waiting for Enter...";

    std::cin.get();

    // --- 8) Reset styles and clear screen before exit ---
    console.Reset();
    console.ClearScreen();
    console.GotoXY(0, 0);

    return 0;
}
