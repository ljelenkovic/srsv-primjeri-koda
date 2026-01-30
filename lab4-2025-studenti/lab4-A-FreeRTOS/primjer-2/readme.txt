This is a FreeRTOS demo that encodes ASCII text to Morse code and decodes it back.

It is organized into the following files:
- main.c - entry point, hardware init, FreeRTOS hooks
- main_lab.c - creates tasks, queue and mutex
- src/encoder.c, inc/encoder.h - encodes ASCII to Morse code
- src/decoder.c, inc/decoder.h - decodes Morse code to ASCII
- src/monitor.c, inc/monitor.h - prints system statistics
- inc/shared.h - shared data types and globals

Building and running:
- cd build/gcc && make
- qemu-system-arm -machine mps2-an385 -cpu cortex-m3 -kernel output/RTOSDemo.out -nographic -serial stdio

Environment prepared based on this video: https://www.youtube.com/watch?v=l2GmlDN_SPo