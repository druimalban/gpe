#include <stdio.h>
#include <stdlib.h>
#include <err.h>

#include <vga.h>
#include <vgagl.h>

void
svgalib_init(int mode)
{
	gl_setcontextvgavirtual(mode);
	vga_init();
	vga_setmode(mode);
	gl_setcontextvga(mode);
}

void
svgalib_done(void)
{
	vga_setmode(TEXT);
}

static void
decode_sprite(char *out, char *in)
{
	int i, c, x, y;
	static int palette[] = {0, 4, 3, 2};
	static int trans[] = {0, 12, 10, 14, 9, 13, 11, 15};

	x = y = 0;
	for (i = 0; i < 128; i++) {
		c = ((in[i] & 8) / 8) + ((in[i] & 128) / 64);
		*(out + (x+0)*2 + y * 32) = trans[palette[c]];
		*(out + (x+0)*2+1 + y * 32) = trans[palette[c]];
		c = ((in[i] & 4) / 4) + ((in[i] & 64) / 32);
		*(out + (x+1)*2 + y * 32) = trans[palette[c]];
		*(out + (x+1)*2+1 + y * 32) = trans[palette[c]];
		c = ((in[i] & 2) / 2) + ((in[i] & 32) / 16);
		*(out + (x+2)*2 + y * 32) = trans[palette[c]];
		*(out + (x+2)*2+1 + y * 32) = trans[palette[c]];
		c = ((in[i] & 1) / 1) + ((in[i] & 16) / 8);
		*(out + (x+3)*2 + y * 32) = trans[palette[c]];
		*(out + (x+3)*2+1 + y * 32) = trans[palette[c]];
		y++;
		if ((y / 8) * 8 == y) {
			x += 4;
			y = ((y / 8 - 1) * 8);
			if (x == 16) {
				x = 0;
				y += 8;
			}
		}
	}
}

int
main(int argc, char **argv)
{
	char buf[512], *sprites[48];
	int sx, sy, i;
	FILE *f;

	if (argc != 2)
		errx(1, "usage: showsprites [filename]");

	if ((f = fopen(argv[1], "r")) == NULL)
		err(1, "%s", argv[1]);

	fseek(f, 3616, SEEK_SET);

	for (i = 0; i < 48; i++) {
		fread(buf, 128, 1, f);
		sprites[i] = (char *)malloc(32 * 32);
		decode_sprite(sprites[i], buf);
	}

	fclose(f);

	svgalib_init(G320x200x256);

	for (sy = 0; sy < 6; sy++)
	for (sx = 0; sx < 8 && sx + sy * 8 < 48; sx++)
		gl_putbox(sx * 32, sy * 32, 32, 32, sprites[sx + sy * 8]);

	vga_getch();
	svgalib_done();

	return 0;
}
