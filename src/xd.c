#include <ctype.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h> 
#include <string.h>
#include <strings.h>
#include <stdbool.h>
#include <stdarg.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>

#define __XD_USAGE__													\
	"Usage: %s [-D|-O][-L text][-K text][-R when][-b][-d|-o][-c cols][-g size][-p][-t type][-u] [file...]\n"	\
	"       %s -i [-c columns][-n name][-u] [file...]\n"								\
	"       %s -?\n"												\
	"Displays a hexdump of the specified file(s) or standard input.\n"						\
	"This utility is not a part of POSIX® specification.\n"								\
	"Special options:\n"												\
	"\t-?: Displays this message and then exits.\n"									\
	"General options:\n"												\
	"\t-D: Disables the printing of offsets.\n"									\
	"\t-L text: Specifies the dump separator.\n"									\
	"\t-K text: Specifies the offset separator.\n"									\
	"\t-O: Enables the printing of offsets (default).\n"								\
	"\t-R when: Specifies when to color output. When may be: never, always or auto.\n"				\
	"\t-b: Performs a binary dump (implies -c6 and -g1).\n"								\
	"\t-c cols: Specifies how many columns to use on dump (defaults to 16).\n"					\
	"\t-d: Counts the offset in decimal, not hexadecimal.\n"							\
	"\t-g size: Specifies a group size for the dumped bytes (defaults to 2).\n"					\
	"\t-i: Outputs C headers (implies -c12).\n"									\
	"\t-n name: Specifies a name for the variable with the option -i.\n"						\
	"\t-o: Counts the offset in octal, not in hexadecimal.\n"							\
	"\t-p: Displays in plain dump format (implies -D, -c30 and -g0).\n"						\
	"\t-t type: Performs a dump of specified type: b(binary), x or h (hexadecimal), d(decimal) or o(octal).\n"	\
	"\t-u: Use upper case hex letters.\n"										\
	"Manual entry shall be avaliable with: `man 1 xd`.\n"								\
	"Copyright (©) @Arthur de Souza Manske, 2024. All rights reserved.\n"

#define HEXDFL_FORCE_COLUMNS	(0x01)
#define HEXDFL_FORCE_WORDSIZE	(0x02)
#define HEXDFL_COLORED		(0x04)
#define HEXDFL_PLAIN		(0x08)
#define HEXDFL_BINARY_DUMP	(0x10)
#define HEXDFL_DECIMAL_COUNT		(0x20)
#define HEXDFL_C		(0x40)
#define HEXDFL_UPPER		(0x80)
#define HEXDFL_NOCOLOR		(0x100)
#define HEXDFL_AUTONAME		(0x200)
#define HEXDFL_NOCOUNT		(0x400)
#define HEXDFL_MASK_COLOR	(HEXDFL_COLORED	| HEXDFL_NOCOLOR)
#define HEXPFL_BYTE		(0x800)

#define HEXDFL_OCTAL_COUNT	(0x1000)
#define HEXDFL_OCTAL_DUMP	(0x2000)
#define HEXDFL_DECIMAL_DUMP	(0x4000)

struct hexdumping {
	uint16_t hex_flags;
	int	 hex_columns, hex_wordsize;
	char	*hex_filename, *hex_dump_separator, *hex_off_separator;
};

int fpeek(FILE *stream)
{
	int ch = fgetc(stream);
	ungetc(ch, stream);
	return ch;
}

int hexprint(FILE *stream, unsigned char ch, uint16_t flags)
{
	static char *format;
	int ret = 0;

	format = "%02hhx";

	if (flags & HEXDFL_COLORED) {
		uint8_t color = 32;	
		if (ch < 32) color = 33;
		if (ch > 127) color = 31;
		if (!ch) color = 97;

		fprintf(stream, "\033[1;%hhum", color);
	}

	if (flags & HEXPFL_BYTE && flags & HEXDFL_BINARY_DUMP) {
		for (int8_t i = 7; i >= 0; --i, ++ret)
			fputc(ch & (1 << i) ? '1' : '0', stream);

		return ret;
	}

	if (flags & HEXDFL_UPPER)	 format = "%02hhX";
	if (flags & HEXDFL_DECIMAL_DUMP) format = "%03hhu";
	if (flags & HEXDFL_OCTAL_DUMP)	 format = "%03hho";

	if (!(flags & HEXPFL_BYTE)) {
		if (ch < 32 || ch > 127) ch = '.';
		format = "%c";
	}

	ret += fprintf(stream, format, ch);
	if (flags & HEXDFL_COLORED) fputs("\033[0m", stream);

	return ret;
}

int chexdump(FILE *restrict stream, FILE *restrict source, struct hexdumping *restrict hexfl)
{	
	int ch;
	ssize_t i;
	uintmax_t totalbytes = 0;

	if (hexfl->hex_filename) {
		if (hexfl->hex_filename[0] == '-' && !hexfl->hex_filename[1]) {
			hexfl->hex_filename = "stdin";
		} else {
			for (i = 0; hexfl->hex_filename[i]; ++i) {
				if ((i == 0 && isdigit(hexfl->hex_filename[i])) || !isalnum(hexfl->hex_filename[i]))
					hexfl->hex_filename[i] = '_';
			}
		}

		fprintf(stream, "unsigned char %s[] = {\n\t", hexfl->hex_filename);
	}

	while (((ch = fgetc(source))) != EOF) {
		char *format = "0x%02hhx";
		if (hexfl->hex_flags & HEXDFL_UPPER) format = "0x%02hhX";
		
		if (totalbytes != 0 && totalbytes % hexfl->hex_columns == 0) {
			fputc('\n', stream);
			if (hexfl->hex_filename) fputc('\t', stream);
		} else if (totalbytes != 0) {
			fputs(", ", stream);
		}

		fprintf(stream, format, ch);
		totalbytes++;
	}

	if (ferror(stream) || ferror(source)) return -1;
	
	if (hexfl->hex_filename) fprintf(stream, "\n};\nunsigned int %s_len = %ju;", hexfl->hex_filename, totalbytes); 
	if (hexfl->hex_filename || (totalbytes % hexfl->hex_columns) != 0) fputc('\n', stream);

	return 0;
}

int hexdump(FILE *restrict stream, FILE *restrict source, struct hexdumping *restrict hexfl)
{
	int ch;
	uintmax_t totalbytes = 0;
	
	int dumplen = 0, digits = 2;

	if (hexfl->hex_flags & HEXDFL_BINARY_DUMP) digits = 8;
	if (hexfl->hex_flags & HEXDFL_DECIMAL_DUMP || hexfl->hex_flags & HEXDFL_OCTAL_DUMP) digits = 3;

	if (!(hexfl->hex_flags & HEXDFL_FORCE_COLUMNS)) {
		if (hexfl->hex_flags & HEXDFL_C) {
			hexfl->hex_columns = 12;
		} else if (hexfl->hex_flags & HEXDFL_BINARY_DUMP) {
			hexfl->hex_columns = 6;
		} else if (hexfl->hex_flags & HEXDFL_PLAIN) {
			hexfl->hex_columns = 30;
		} else {
			hexfl->hex_columns = 16;
		}
	}
	
	if (!(hexfl->hex_flags & HEXDFL_FORCE_WORDSIZE)) {
		if (hexfl->hex_flags & HEXDFL_PLAIN) {
			hexfl->hex_wordsize = INT_MAX;
		} else if (hexfl->hex_flags & HEXDFL_BINARY_DUMP) {
			hexfl->hex_wordsize = 1;
		} else {
			hexfl->hex_wordsize = 2;
		}
	}

	if (hexfl->hex_flags & HEXDFL_C) return chexdump(stream, source, hexfl);

	dumplen = (hexfl->hex_columns * digits) + (hexfl->hex_columns / hexfl->hex_wordsize);
	if (hexfl->hex_columns % hexfl->hex_wordsize == 0) dumplen -= 1;

	while (fpeek(source) != EOF) {
		fpos_t initpos;	
		int i = 0, used_spaces = 0, used_digits = 0;

		if (!(hexfl->hex_flags & HEXDFL_PLAIN)) fgetpos(source, &initpos);

		if (!(hexfl->hex_flags & HEXDFL_NOCOUNT)) {
			char *format = "%010jx%s";
			if (hexfl->hex_flags & HEXDFL_OCTAL_COUNT)   format = "%010jo%s";
			if (hexfl->hex_flags & HEXDFL_DECIMAL_COUNT) format = "%010ju%s";

			fprintf(stream, format, totalbytes, hexfl->hex_off_separator);
		}

		while (i < hexfl->hex_columns && ch != EOF) {
			if (i != 0) {
				fputc(' ', stream);
				used_spaces++;
			}

			for (ssize_t k = 0; k < hexfl->hex_wordsize && i < hexfl->hex_columns; ++i, ++k, ++totalbytes) {
				if ((ch = fgetc(source)) == EOF) break;
				used_digits += hexprint(stream, (unsigned char) ch, hexfl->hex_flags | HEXPFL_BYTE);
			}
		}
		
		if (!(hexfl->hex_flags & HEXDFL_PLAIN)) {
			fsetpos(source, &initpos);
			fprintf(stream, "%*s", dumplen - used_digits - used_spaces, "");

			fputs(hexfl->hex_dump_separator, stream);

			for (i = 0; i < hexfl->hex_columns; ++i) {
				if ((ch = fgetc(source)) == EOF) break;
				hexprint(stream, (unsigned char) ch, hexfl->hex_flags);
			}
		}
		
		fputc('\n', stream);
	}

	if (ferror(stream) || ferror(source)) return -1;
	return 0;
}

int main(int argc, char **argv)
{
	int ch;
	FILE *input;

	int exitcode = EXIT_SUCCESS;

	struct hexdumping *hexfl = & (struct hexdumping) {.hex_dump_separator = " ", .hex_off_separator = ": "};

	opterr = 0;
	while ((ch = getopt(argc, argv, ":DK:L:OR:bc:dig:n:oprt:u")) != -1) {
		switch (ch) {
		case '?':
			if (optopt == '?') {
				fprintf(stdout, __XD_USAGE__, argv[0], argv[0], argv[0]);
				return EXIT_SUCCESS;
			}
			
			fprintf(stderr, "%s: Illegal option: %c.\n", argv[0], optopt);
			fprintf(stderr, __XD_USAGE__, argv[0], argv[0], argv[0]);
			return EXIT_FAILURE;
		case ':':
			fprintf(stderr, "%s: Missing option argument: %c.\n", argv[0], optopt);
			fprintf(stderr, __XD_USAGE__, argv[0], argv[0], argv[0]);
			return EXIT_FAILURE;
		case 'K': hexfl->hex_off_separator = optarg; break; 
		case 'L': hexfl->hex_dump_separator = optarg; break;
		case 'D': hexfl->hex_flags |= HEXDFL_NOCOUNT; break;
		case 'O': hexfl->hex_flags &= ~HEXDFL_NOCOUNT; break;
		case 'R':
			if (strcasecmp("auto", optarg) == 0) {
				hexfl->hex_flags &= ~HEXDFL_MASK_COLOR;
			} else if (strcasecmp("always", optarg) == 0) {
				hexfl->hex_flags &= ~HEXDFL_MASK_COLOR;
				hexfl->hex_flags |= HEXDFL_COLORED;
			} else if (strcasecmp("never", optarg) == 0) {
				hexfl->hex_flags &= ~HEXDFL_MASK_COLOR;
				hexfl->hex_flags |= HEXDFL_NOCOLOR;
			}

			break;
		case 'b': hexfl->hex_flags |= HEXDFL_BINARY_DUMP; break;
		case 'c':
			hexfl->hex_columns = (uint32_t) strtoull(optarg, NULL, 0);
			hexfl->hex_flags |= HEXDFL_FORCE_COLUMNS;
			if (hexfl->hex_columns == 0 && *optarg != '0') {
				illegal_optarg:
				fprintf(stderr, "%s: '%s': Illegal option argument.\n", argv[0], optarg);
				fprintf(stderr, __XD_USAGE__, argv[0], argv[0], argv[0]);
				return EXIT_FAILURE;
			}

			if (hexfl->hex_columns == 0) hexfl->hex_columns = INT_MAX;	

			break;
		case 'd': hexfl->hex_flags |= HEXDFL_DECIMAL_COUNT; break;
		case 'i': hexfl->hex_flags |= HEXDFL_C; hexfl->hex_flags &= ~HEXDFL_PLAIN; break;
		case 'g':
			hexfl->hex_wordsize = (uint32_t) strtoull(optarg, NULL, 0);
			hexfl->hex_flags |= HEXDFL_FORCE_WORDSIZE;
			if (hexfl->hex_wordsize == 0 && *optarg != '0')
				goto illegal_optarg;

			if (hexfl->hex_wordsize == 0) hexfl->hex_wordsize = INT_MAX;	

			break;
		case 'n':
			if (strcmp(optarg, "NULL") == 0) {
				hexfl->hex_filename = NULL;
			} else {
				hexfl->hex_filename = optarg;
			}

			hexfl->hex_flags |= HEXDFL_AUTONAME;
			break;
		case 'o':
			hexfl->hex_flags &= ~(HEXDFL_DECIMAL_COUNT);
			hexfl->hex_flags |= HEXDFL_OCTAL_COUNT;
			break;
		case 'p':
			hexfl->hex_flags &= ~(HEXDFL_C | HEXDFL_MASK_COLOR);
			hexfl->hex_flags |= (HEXDFL_NOCOUNT | HEXDFL_PLAIN | HEXDFL_NOCOLOR);
			break;
		case 't':
			if (optarg[1]) goto illegal_optarg; 
			
			switch (optarg[0]) {
				case 'h': case 'x': hexfl->hex_flags &= ~(HEXDFL_DECIMAL_DUMP | HEXDFL_OCTAL_DUMP | HEXDFL_BINARY_DUMP); break;
				case 'b': hexfl->hex_flags &= ~(HEXDFL_DECIMAL_DUMP | HEXDFL_OCTAL_DUMP); hexfl->hex_flags |= HEXDFL_BINARY_DUMP; break;
				case 'o': hexfl->hex_flags &= ~(HEXDFL_BINARY_DUMP  | HEXDFL_DECIMAL_DUMP); hexfl->hex_flags |= HEXDFL_OCTAL_DUMP; break;
				case 'd': hexfl->hex_flags &= ~(HEXDFL_BINARY_DUMP  | HEXDFL_OCTAL_DUMP); hexfl->hex_flags |= HEXDFL_DECIMAL_DUMP; break;
				default: goto illegal_optarg;
			}

			break;
		case 'u': hexfl->hex_flags |= HEXDFL_UPPER; break;
		}
	}
	
	if (!(hexfl->hex_flags & HEXDFL_MASK_COLOR) && isatty(STDOUT_FILENO) && !getenv("NO_COLOR")) 
		hexfl->hex_flags |= HEXDFL_COLORED;

	if (!argv[optind]) argv[optind] = "-";
	
	do {
		if (argv[optind] && argv[optind][0] == '-' && !argv[optind][1]) {
			input = stdin;
		} else if (!(input = fopen(argv[optind], "r"))) {
			fprintf(stderr, "%s: '%s': Unable to open file: %s (%d).\n", argv[0], argv[optind], strerror(errno), errno);
			exitcode = EXIT_FAILURE; 
			continue; 
		}

		if (isatty(fileno(input))) { /* saves input to a temporary file */
			int ch;
			FILE *tmp = tmpfile();
			while ((ch = fgetc(input)) != EOF) fputc(ch, tmp);
			if (input != stdin) fclose(input);

			input = tmp;
			fseek(input, 0, SEEK_SET);
		}

		if (!(hexfl->hex_flags & HEXDFL_AUTONAME)) hexfl->hex_filename = argv[optind];
		if (hexdump(stdout, input, hexfl) < 0) {
			fprintf(stderr, "%s: '%s': Unable to hexdump: %s (%d).\n", argv[0], argv[optind], strerror(errno), errno);
			exitcode = EXIT_FAILURE;
		}

		if (input == stdin) fclose(input); 
	} while ((++optind) < argc);

	return exitcode;
}
