mkdir -pv build step1
glib-compile-resources --generate --target=step1/resources.c midi_pipes.gresource.xml
gcc -o build/midi_pipes src/main.c src/inc/midi.c step1/resources.c \
  -lasound -Wall `pkg-config --cflags --libs gtk+-3.0` -export-dynamic
