mkdir -pv build
gcc -o build/midi_pipes src/main.c src/inc/midi.c -lasound -Wall `pkg-config --cflags --libs gtk+-3.0` -export-dynamic
