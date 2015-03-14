# PS2KeyPolled #
This project is C code for **Arduino** (or potentially other micro; the code is rather general) for interfacing to a PC-style (PS2 or AT) serial keyboard.  It has features:

1. Can use any two IO pins for polled clock and data signals; does not require interrupts.
2. Keeps the keyboard "flow controlled" when not being read, so that keystrokes will be buffered.
3. Supports writing to the keyboard as well as reading, for setting LEDs and etc.
4. Compresses extended keycodes into a single response.
5. Returns information about key release events as well as keypress events.
6. Includes code to translate key events to ascii.
 
