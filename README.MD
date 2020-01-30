Subject is [here](https://www-soc.lip6.fr/trac/sesi-peri/wiki/IOC20_T01).

Project is cross-compiled for a Raspberry PI 1 using gcc for the bcm2708 architecture. Code is divided in two parts, a simple "hello world" program to test cross-compilation, and a series of programs aiming to control LEDs and detect button presses (all components are connected on the GPIO).

To compile each project, just use make all, the programs have different CLI args:

- blink0 takes as first arg the blinking frequency (defaults to 1Hz)
- blink0_pt takes as first arg the blinking frequency of the LED (defaults to 1Hz)
- blink01_pt takes as first arg the blinking frequency of the first LED, as second arg the bliking frequency of the second led (defaults to 1Hz and 2Hz respectively)
- blink01_bp_pt takes as first arg the blinking frequency of the blinking LED (defaults to 1Hz) and as second argument the polling interval for the button status and global variable value (defaults to 20ms).
- read_bp takes as first argument the polling interval for the button status (defaults to 100ms)


Project is copied on the PI using SCP.
