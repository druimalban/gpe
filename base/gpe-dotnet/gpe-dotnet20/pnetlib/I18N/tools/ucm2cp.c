/*
 * ucm2cp.c - Convert IBM ".ucm" files into code page handling classes.
 *
 * Copyright (c) 2002  Southern Storm Software, Pty Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


/*

Usage: ucm2cp [options] file

	--region name			I18N region name
	--page num				Code page number
	--wpage num				Windows code page number (optional)
	--name str				Human-readable encoding name
	--webname str			Web name of the encoding
	--headername str		Header name of the encoding (optional)
	--bodyname str			Body name of the encoding (optional)
	--no-browser-display	Set browser display value to false (optional)
	--no-browser-save		Set browser save value to false (optional)
	--no-mailnews-display	Set mail/news display value to false (optional)
	--no-mailnews-save		Set mail/news save value to false (optional)

*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*
 * Option values.
 */
static char *region = 0;
static int codePage = 0;
static int windowsCodePage = 0;
static char *name = 0;
static char *webName = 0;
static char *headerName = 0;
static char *bodyName = 0;
static int isBrowserDisplay = 1;
static int isBrowserSave = 1;
static int isMailNewsDisplay = 1;
static int isMailNewsSave = 1;
static const char *filename = 0;

/*
 * Forward declarations.
 */
static void usage(char *progname);
static void loadCharMaps(FILE *file);
static void printHeader(void);
static void printFooter(void);
static void printByteToChar(void);
static void printCharToByte(void);

int main(int argc, char *argv[])
{
	char *progname = argv[0];
	FILE *file;
	int len;

	/* Process the command-line options */
	while(argc > 1 && argv[1][0] == '-')
	{
		if(!strcmp(argv[1], "--page") && argc > 2)
		{
			codePage = atoi(argv[2]);
			++argv;
			--argc;
		}
		else if(!strcmp(argv[1], "--wpage") && argc > 2)
		{
			windowsCodePage = atoi(argv[2]);
			++argv;
			--argc;
		}
		else if(!strcmp(argv[1], "--region") && argc > 2)
		{
			region = argv[2];
			++argv;
			--argc;
		}
		else if(!strcmp(argv[1], "--name") && argc > 2)
		{
			name = argv[2];
			++argv;
			--argc;
		}
		else if(!strcmp(argv[1], "--webname") && argc > 2)
		{
			webName = argv[2];
			++argv;
			--argc;
		}
		else if(!strcmp(argv[1], "--headername") && argc > 2)
		{
			headerName = argv[2];
			++argv;
			--argc;
		}
		else if(!strcmp(argv[1], "--bodyname") && argc > 2)
		{
			bodyName = argv[2];
			++argv;
			--argc;
		}
		else if(!strcmp(argv[1], "--no-browser-display"))
		{
			isBrowserDisplay = 0;
		}
		else if(!strcmp(argv[1], "--no-browser-save"))
		{
			isBrowserSave = 0;
		}
		else if(!strcmp(argv[1], "--no-mailnews-display"))
		{
			isMailNewsDisplay = 0;
		}
		else if(!strcmp(argv[1], "--no-mailnews-save"))
		{
			isMailNewsSave = 0;
		}
		++argv;
		--argc;
	}

	/* Make sure that we have sufficient options */
	if(!region || !codePage || !name || !webName || argc != 2)
	{
		usage(progname);
		return 1;
	}

	/* Set defaults for unspecified options */
	if(!headerName)
	{
		headerName = webName;
	}
	if(!bodyName)
	{
		bodyName = webName;
	}
	if(!windowsCodePage)
	{
		windowsCodePage = codePage;
	}

	/* Open the UCM file */
	file = fopen(argv[1], "r");
	if(!file)
	{
		perror(argv[1]);
		return 1;
	}
	filename = argv[1];
	len = strlen(filename);
	while(len > 0 && filename[len - 1] != '/' && filename[len - 1] != '\\')
	{
		--len;
	}
	filename += len;

	/* Load the character maps from the input file */
	loadCharMaps(file);

	/* Print the output header */
	printHeader();

	/* Print the byte->char conversion table */
	printByteToChar();

	/* Output the char->byte conversion methods */
	printCharToByte();

	/* Print the output footer */
	printFooter();

	/* Clean up and exit */
	fclose(file);
	return 0;
}

static void usage(char *progname)
{
	fprintf(stderr, "Usage: %s [options] file\n\n", progname);
	fprintf(stderr, "    --region name         I18N region name\n");
	fprintf(stderr, "    --page num            Code page number\n");
	fprintf(stderr, "    --wpage num           Windows code page number (optional)\n");
	fprintf(stderr, "    --name str            Human-readable encoding name\n");
	fprintf(stderr, "    --webname str         Web name of the encoding\n");
	fprintf(stderr, "    --headername str      Header name of the encoding (optional)\n");
	fprintf(stderr, "    --bodyname str        Body name of the encoding (optional)\n");
	fprintf(stderr, "    --no-browser-display  Set browser display value to false (optional)\n");
	fprintf(stderr, "    --no-browser-save     Set browser save value to false (optional)\n");
	fprintf(stderr, "    --no-mailnews-display Set mail/news display value to false (optional)\n");
	fprintf(stderr, "    --no-mailnews-save    Set mail/news save value to false (optional)\n");
}

/*
 * Map bytes to characters.  The level value is used to determine
 * which char mapping is the most likely if there is more than one.
 */
static unsigned byteToChar[256];
static int      byteToCharLevel[256];

/*
 * Map characters to bytes.
 */
static int charToByte[65536];

/*
 * Parse a hexadecimal value.  Returns the length
 * of the value that was parsed.
 */
static int parseHex(const char *buf, unsigned long *value)
{
	int len = 0;
	char ch;
	*value = 0;
	while((ch = buf[len]) != '\0')
	{
		if(ch >= '0' && ch <= '9')
		{
			*value = *value * 16 + (unsigned long)(ch - '0');
		}
		else if(ch >= 'A' && ch <= 'F')
		{
			*value = *value * 16 + (unsigned long)(ch - 'A' + 10);
		}
		else if(ch >= 'a' && ch <= 'f')
		{
			*value = *value * 16 + (unsigned long)(ch - 'a' + 10);
		}
		else
		{
			break;
		}
		++len;
	}
	return len;
}

/*
 * Load the character mapping information from a UCM file.
 */
static void loadCharMaps(FILE *file)
{
	unsigned long posn;
	unsigned long byteValue;
	int level;
	char buffer[BUFSIZ];
	const char *buf;
	int macStyle;
	int macStyleInited = 0;

	/* Initialize the mapping tables */
	for(posn = 0; posn < 256; ++posn)
	{
		byteToChar[posn] = (unsigned)'?';
		byteToCharLevel[posn] = 100;
	}
	for(posn = 0; posn < 65536; ++posn)
	{
		charToByte[posn] = -1;
	}

	/* Read the contents of the file */
	while(fgets(buffer, BUFSIZ, file))
	{
		/* Lines of interest begin with "<U" (IBM style) or "0x" (Mac style) */
		macStyle = 0;
		if(buffer[0] == '0' && buffer[1] == 'x')
		{
			macStyle = 1;
			if(!macStyleInited)
			{
				for(posn = 0; posn < 256; ++posn)
				{
					byteToChar[posn] = (unsigned)posn;
				}
			}
		}
		else if(buffer[0] != '<' || buffer[1] != 'U')
		{
			continue;
		}
		macStyleInited = 1;

		/* Parse the fields on the line */
		if(!macStyle)
		{
			buf = buffer + 2;
			buf += parseHex(buf, &posn);
			if(posn >= 65536)
			{
				continue;
			}
			while(*buf != '\0' && *buf != '\\')
			{
				++buf;
			}
			if(*buf != '\\' || buf[1] != 'x')
			{
				continue;
			}
			buf += 2;
			buf += parseHex(buf, &byteValue);
			if(byteValue >= 256)
			{
				continue;
			}
			while(*buf != '\0' && *buf != '|')
			{
				++buf;
			}
			if(*buf != '|')
			{
				continue;
			}
			level = (int)(buf[1] - '0');
		}
		else
		{
			buf = buffer + 2;
			buf += parseHex(buf, &byteValue);
			if(byteValue >= 0x0100)
			{
				continue;
			}
			while(*buf != '\0' && *buf != '0')
			{
				++buf;
			}
			if(*buf != '0' || buf[1] != 'x')
			{
				continue;
			}
			buf += 2;
			buf += parseHex(buf, &posn);
			if(posn >= 65535)
			{
				continue;
			}
			level = 1;
		}

		/* Update the byte->char mapping table */
		if(level < byteToCharLevel[byteValue])
		{
			byteToCharLevel[byteValue] = level;
			byteToChar[byteValue] = (unsigned)posn;
		}

		/* Update the char->byte mapping table */
		charToByte[posn] = (int)byteValue;
	}
}

#define	COPYRIGHT_MSG \
" *\n" \
" * Copyright (c) 2002  Southern Storm Software, Pty Ltd\n" \
" *\n" \
" * This program is free software; you can redistribute it and/or modify\n" \
" * it under the terms of the GNU General Public License as published by\n" \
" * the Free Software Foundation; either version 2 of the License, or\n" \
" * (at your option) any later version.\n" \
" *\n" \
" * This program is distributed in the hope that it will be useful,\n" \
" * but WITHOUT ANY WARRANTY; without even the implied warranty of\n" \
" * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n" \
" * GNU General Public License for more details.\n" \
" *\n" \
" * You should have received a copy of the GNU General Public License\n" \
" * along with this program; if not, write to the Free Software\n" \
" * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA\n" \
" */\n\n"

/*
 * Print the header for the current code page definition.
 */
static void printHeader(void)
{
	printf("/*\n * CP%d.cs - %s code page.\n", codePage, name);
	fputs(COPYRIGHT_MSG, stdout);
	printf("// Generated from \"%s\".\n\n", filename);
	printf("namespace I18N.%s\n{\n\n", region);
	printf("using System;\n");
	printf("using I18N.Common;\n\n");
	printf("public class CP%d : ByteEncoding\n{\n", codePage);
	printf("\tpublic CP%d()\n", codePage);
	printf("\t\t: base(%d, ToChars, \"%s\",\n", codePage, name);
	printf("\t\t       \"%s\", \"%s\", \"%s\",\n",
	       bodyName, headerName, webName);
	printf("\t\t       %s, %s, %s, %s, %d)\n",
		   (isBrowserDisplay ? "true" : "false"),
		   (isBrowserSave ? "true" : "false"),
		   (isMailNewsDisplay ? "true" : "false"),
		   (isMailNewsSave ? "true" : "false"),
		   windowsCodePage);
	printf("\t{}\n\n");
}

/*
 * Print an encoding name, adjusted to look like a type name.
 */
static void printEncodingName(const char *name)
{
	while(*name != '\0')
	{
		if(*name >= 'A' && *name <= 'Z')
		{
			putc(*name - 'A' + 'a', stdout);
		}
		else if(*name == '-')
		{
			putc('_', stdout);
		}
		else
		{
			putc(*name, stdout);
		}
		++name;
	}
}

/*
 * Print the footer for the current code page definition.
 */
static void printFooter(void)
{
	printf("}; // class CP%d\n\n", codePage);
	printf("public class ENC");
	printEncodingName(webName);
	printf(" : CP%d\n{\n", codePage);
	printf("\tpublic ENC");
	printEncodingName(webName);
	printf("() : base() {}\n\n");
	printf("}; // class ENC");
	printEncodingName(webName);
	printf("\n\n}; // namespace I18N.%s\n", region);
}

/*
 * Print the byte->char conversion table.
 */
static void printByteToChar(void)
{
	int posn;
	printf("\tprivate static readonly char[] ToChars = {");
	for(posn = 0; posn < 256; ++posn)
	{
		if((posn % 6) == 0)
		{
			printf("\n\t\t");
		}
		printf("'\\u%04X', ", byteToChar[posn]);
	}
	printf("\n\t};\n\n");
}

/*
 * Print a "switch" statement that converts "ch" from
 * a character value into a byte value.
 */
static void printConvertSwitch(void)
{
	unsigned long directLimit;
	unsigned long posn;
	unsigned long posn2;
	unsigned long rangeSize;
	int haveDirect;
	int haveFullWidth;

	/* Find the limit of direct byte mappings */
	directLimit = 0;
	while(directLimit < 256 && charToByte[directLimit] == (int)directLimit)
	{
		++directLimit;
	}

	/* Determine if we have the full-width Latin1 mappings, which
	   we can optimise in the default case of the switch */
	haveFullWidth = 1;
	for(posn = 0xFF01; posn <= 0xFF5E; ++posn)
	{
		if((charToByte[posn] - 0x21) != (int)(posn - 0xFF01))
		{
			haveFullWidth = 0;
		}
	}

	/* Print the switch header.  The "if" is an optimisation
	   to ignore the common case of direct ASCII mappings */
	printf("\t\t\tif(ch >= %lu) switch(ch)\n", directLimit);
	printf("\t\t\t{\n");

	/* Handle all direct byte mappings above the direct limit */
	haveDirect = 0;
	for(posn = directLimit; posn < 256; ++posn)
	{
		if(charToByte[posn] == (int)posn)
		{
			haveDirect = 1;
			printf("\t\t\t\tcase 0x%04lX:\n", posn);
		}
	}
	if(haveDirect)
	{
		printf("\t\t\t\t\tbreak;\n");
	}

	/* Handle the indirect mappings */
	for(posn = 0; posn < 65536; ++posn)
	{
		if(haveFullWidth && posn >= 0xFF01 && posn <= 0xFF5E)
		{
			/* Handle full-width Latin1 conversions later */
			continue;
		}
		if(charToByte[posn] != (int)posn &&
		   charToByte[posn] != -1)
		{
			/* See if we have a run of 4 or more characters that
			   can be mapped algorithmically to some other range */
			rangeSize = 1;
			for(posn2 = posn + 1; posn2 < 65536; ++posn2)
			{
				if(charToByte[posn2] == (int)posn2 ||
				   charToByte[posn2] == -1)
				{
					break;
				}
				if((charToByte[posn2] - charToByte[posn]) !=
				   (int)(posn2 - posn))
				{
					break;
				}
				++rangeSize;
			}
			if(rangeSize >= 4)
			{
				/* Output a range mapping for the characters */
				for(posn2 = posn; posn2 < (posn + rangeSize); ++posn2)
				{
					printf("\t\t\t\tcase 0x%04lX:\n", posn2);
				}
				posn += rangeSize - 1;
				if(((long)posn) >= (long)(charToByte[posn]))
				{
					printf("\t\t\t\t\tch -= 0x%04lX;\n",
						   (long)(posn - charToByte[posn]));
				}
				else
				{
					printf("\t\t\t\t\tch += 0x%04lX;\n",
						   (long)(charToByte[posn] - posn));
				}
				printf("\t\t\t\t\tbreak;\n");
			}
			else
			{
				/* Use a simple non-algorithmic mapping */
				printf("\t\t\t\tcase 0x%04lX: ch = 0x%02X; break;\n",
					   posn, (unsigned)(charToByte[posn]));
			}
		}
	}

	/* Print the switch footer */
	if(!haveFullWidth)
	{
		printf("\t\t\t\tdefault: ch = 0x3F; break;\n");
	}
	else
	{
		printf("\t\t\t\tdefault:\n");
		printf("\t\t\t\t{\n");
		printf("\t\t\t\t\tif(ch >= 0xFF01 && ch <= 0xFF5E)\n");
		printf("\t\t\t\t\t\tch -= 0xFEE0;\n");
		printf("\t\t\t\t\telse\n");
		printf("\t\t\t\t\t\tch = 0x3F;\n");
		printf("\t\t\t\t}\n");
		printf("\t\t\t\tbreak;\n");
	}
	printf("\t\t\t}\n");
}

/*
 * Print the char->byte conversion methods.
 */
static void printCharToByte(void)
{
	/* Print the conversion method for character buffers */
	printf("\tprotected override void ToBytes(char[] chars, int charIndex, int charCount,\n");
	printf("\t                                byte[] bytes, int byteIndex)\n");
	printf("\t{\n");
	printf("\t\tint ch;\n");
	printf("\t\twhile(charCount > 0)\n");
	printf("\t\t{\n");
	printf("\t\t\tch = (int)(chars[charIndex++]);\n");
	printConvertSwitch();
	printf("\t\t\tbytes[byteIndex++] = (byte)ch;\n");
	printf("\t\t\t--charCount;\n");
	printf("\t\t}\n");
	printf("\t}\n\n");

	/* Print the conversion method for string buffers */
	printf("\tprotected override void ToBytes(String s, int charIndex, int charCount,\n");
	printf("\t                                byte[] bytes, int byteIndex)\n");
	printf("\t{\n");
	printf("\t\tint ch;\n");
	printf("\t\twhile(charCount > 0)\n");
	printf("\t\t{\n");
	printf("\t\t\tch = (int)(s[charIndex++]);\n");
	printConvertSwitch();
	printf("\t\t\tbytes[byteIndex++] = (byte)ch;\n");
	printf("\t\t\t--charCount;\n");
	printf("\t\t}\n");
	printf("\t}\n\n");
}
