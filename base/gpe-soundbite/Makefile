CFLAGS=-Os -fomit-frame-pointer `gtk-config --cflags`

gpe-soundbite: main.o gsm-codec.o oss.o
	$(CC) -o $@ $^ -lgsm
