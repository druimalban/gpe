#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>

#define PASSWORD_OFFSET		0
#define TIME_LIMIT_OFFSET	64
#define EDITOR_CODES_OFFSET	80
#define TRANSPORTER_OFFSET	96
#define PALETTE_OFFSET		224
#define MAP_OFFSET		256
#define SPRITE_OFFSET		3616

static int opt_password;
static int opt_timelimit;
static int opt_codes;
static int opt_transporter;
static int opt_palette;
static int opt_map;
static int opt_sprite;

struct rep_s {
	unsigned char password[8][8];
	unsigned char timelimit[8][2];
	unsigned char codes[16];
	unsigned char transporter[8][16];
	unsigned char palette[8][4];
	unsigned char map[8][420];
	unsigned char sprite[48][128];
};

static void
xfread(void *buf, size_t size, size_t n, FILE *stream, char *filename)
{
	if (fread(buf, size, n, stream) != n)
		errx(1, "%s:unexpected end of file", filename);
}

static void
xfwrite(void *buf, size_t size, size_t n, FILE *stream, char *filename)
{
	if (fwrite(buf, size, n, stream) != n)
		errx(1, "%s:unexpected error while writing", filename);
}

static struct rep_s *
load_rep(char *filename)
{
	FILE *fin;
	struct rep_s *rptr;

	if ((rptr = (struct rep_s *)malloc(sizeof(struct rep_s))) == NULL)
		err(1, NULL);

	if ((fin = fopen(filename, "r")) == NULL)
		err(1, "%s", filename);

	xfread(rptr->password, 8, 8, fin, filename);
	xfread(rptr->timelimit, 2, 8, fin, filename);
	xfread(rptr->codes, 16, 1, fin, filename);
	xfread(rptr->transporter, 16, 8, fin, filename);
	xfread(rptr->palette, 4, 8, fin, filename);
	xfread(rptr->map, 420, 8, fin, filename);
	xfread(rptr->sprite, 128, 48, fin, filename);

	fclose(fin);

	return rptr;
}

static void
process_files(int num, char *file1, char *file2)
{
	FILE *fout;
	struct rep_s *rptr;
	int copyall = 0;

	rptr = load_rep(file1);
	if ((fout = fopen(file2, "r+")) == NULL)
		err(1, "%s", file2);

	if (!opt_password && !opt_timelimit && !opt_codes &&
	    !opt_transporter && !opt_palette && !opt_map && !opt_sprite)
	    copyall = 1;

	if (opt_password || copyall) {
		if (num > 0)
			fseek(fout, 8 * num, SEEK_SET);
		xfwrite(rptr->password[num], 8, 1, fout, file2);
	}
	if (opt_timelimit || copyall) {
		fseek(fout, TIME_LIMIT_OFFSET + 2 * num, SEEK_SET);
		xfwrite(rptr->timelimit[num], 2, 1, fout, file2);
	}
	if (opt_codes || copyall) {
		fseek(fout, EDITOR_CODES_OFFSET, SEEK_SET);
		xfwrite(rptr->codes, 16, 1, fout, file2);
	}
	if (opt_transporter || copyall) {
		fseek(fout, TRANSPORTER_OFFSET + 16 * num, SEEK_SET);
		xfwrite(rptr->timelimit[num], 16, 1, fout, file2);
	}
	if (opt_palette || copyall) {
		fseek(fout, PALETTE_OFFSET + 4 * num, SEEK_SET);
		xfwrite(rptr->palette[num], 4, 1, fout, file2);
	}
	if (opt_map || copyall) {
		fseek(fout, TIME_LIMIT_OFFSET + 420 * num, SEEK_SET);
		xfwrite(rptr->map[num], 420, 1, fout, file2);
	}
	if (opt_sprite || copyall) {
		fseek(fout, SPRITE_OFFSET + 128 * num, SEEK_SET);
		xfwrite(rptr->sprite[num], 128, 1, fout, file2);
	}

	fclose(fout);
	free(rptr);
}

/*
 * Output the program syntax then exit.
 */
static void
usage(void)
{
	fprintf(stderr, "usage: repcopy [-p] [-i] [-e] [-t] [-a] [-m] [-s] [num] [file1] [file2]\n");
	exit(1);
}

int
main(int argc, char **argv)
{
	int i, c;

	while ((c = getopt(argc, argv, "pietams")) != -1)
		switch (c) {
		case 'p':
			opt_password = 1;
			break;
		case 'i':
			opt_timelimit = 1;
			break;
		case 'e':
			opt_codes = 1;
			break;
		case 't':
			opt_transporter = 1;
			break;
		case 'a':
			opt_palette = 1;
			break;
		case 'm':
			opt_map = 1;
			break;
		case 's':
			opt_sprite = 1;
			break;
		case '?':
		default:
			usage();
			/* NOTREACHED */
		}
	argc -= optind;
	argv += optind;

	if (argc != 3)
		usage();

	if (!isdigit(*argv[0]) || (i = atoi(argv[0])) < 0)
		errx(1, "invalid object number");

	process_files(i, argv[1], argv[2]);

	return 0;
}
