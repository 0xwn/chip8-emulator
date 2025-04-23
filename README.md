# CXX-CHIP8: A Simple CHIP-8 Emulator in C++

This is an emulator for the CHIP-8 interpreter system, written in C++ as a learning project focused on emulator development. It utilizes the SDL2 library to handle graphics, input, and window management.

## Features

*   **Complete Opcode Emulation:** Implements all 35 standard CHIP-8 opcodes.
*   **Graphical Output:** Renders the 64x32 monochrome CHIP-8 display in an SDL2 window.
*   **Resizable Window:** The display window can be resized in real-time, and the CHIP-8 graphical content will dynamically scale to fill it.
*   **Keyboard Input:** Maps modern keyboard keys to the 16-key hexadecimal CHIP-8 keypad.
*   **Timers:** Implements the Delay Timer and Sound Timer, correctly decrementing at 60Hz. (Note: Audio *output* - the "beep" - is not yet implemented).
*   **ROM Loading:** Loads CHIP-8 ROM files (usually `.ch8`) specified via command line.
*   **CPU Speed Control:** Includes basic control for the CHIP-8 CPU cycle execution speed (adjustable via a constant in the code).

## Dependencies

To build and run this emulator, you will need:

1.  **C++ Compiler:** A compiler supporting C++11 or later (e.g., GCC, Clang, MSVC).
2.  **SDL2 Library:** The Simple DirectMedia Layer library version 2 (SDL2). (Include)
