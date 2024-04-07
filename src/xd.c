#include <ctype.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h> 
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>

#ifndef ONEBIN
	#define __xd__(...) main(__VA_ARGS__) 
#endif

#define __XD_USAGE__											\
	"Usage: %s [-D|-O][-L text][-K text][-R when][-bd][-c cols][-g size][-o file][-pu] [file...]\n"	\
	"       %s -i [-c columns][-n name][-o file][-u] [file...]\n"					\
	"       %s -?\n"										\
	"Displays a hexdump of the specified file(s) or standard input.\n"				\
	"This utility is not a part of POSIX® specification.\n"						\
	"Special options:\n"										\
	"\t-?: Displays this message and then exits.\n"							\
	"General options:\n"										\
	"\t-D: Disables the printing of offsets.\n"							\
	"\t-L text: Specifies the dump separator. A space is added before it.\n"			\
	"\t-K text: Specifies the offset separator. A space is added after it.\n"			\
	"\t-O: Enables the printing of offsets (default).\n"						\
	"\t-R when: Specifies when to color output. When may be: never, always or auto.\n"		\
	"\t-b: Performs a binary dump (implies -c6 and -g1).\n"						\
	"\t-c cols: Specifies how many columns to use on dump (defaults to 16).\n"			\
	"\t-d: Counts the offset in decimal, not hexadecimal.\n"					\
	"\t-g size: Specifies a group size for the dumped bytes (defaults to 2).\n"			\
	"\t-i: Outputs C headers (implies -c12).\n"							\
	"\t-n name: Specifies a name for the variable with the option -i.\n"				\
	"\t-o file: Specifies a output file instead of standard output.\n"				\
	"\t-p: Displays in plain dump format (implies -D, -c30 and -g0).\n"				\
	"\t-u: Use upper case hex letters.\n"								\
	"Manual entry shall be avaliable with: `man 1 xd`.\n"						\
	"Copyright (©) @Arthur de Souza Manske, 2024. All rights reserved.\n"

#define HEXDFL_FORCE_COLUMNS	(0x01)
#define HEXDFL_FORCE_WORDSIZE	(0x02)
#define HEXDFL_COLORED		(0x04)
#define HEXDFL_PLAIN		(0x08)
#define HEXDFL_BINDUMP		(0x10)
#define HEXDFL_DECCOUNT		(0x20)
#define HEXDFL_C		(0x40)
#define HEXDFL_UPPER		(0x80)
#define HEXDFL_NOCOLOR		(0x100)
#define HEXDFL_AUTONAME		(0x200)
#define HEXDFL_NOCOUNT		(0x400)
#define HEXDFL_MASK_COLOR	(HEXDFL_COLORED	| HEXDFL_NOCOLOR)
#define HEXPFL_BYTE		(0x800)

struct hexdumping {
	uint16_t hex_flags;
	int	 hex_columns, hex_wordsize;
	char	*hex_filename, *hex_dump_separator, *hex_off_separator;
};

int hexprint(FILE *stream, unsigned char ch, uint16_t flags)
{
	int ret = 0;

	if (flags & HEXDFL_COLORED) {
		if (!ch) {
			fputs("\033[1;97m", stream);
		} else if (ch < 32) {
			fputs("\033[1;33m", stream);
		} else if (ch > 127) {
			fputs("\033[1;31m", stream);
		} else {
			fputs("\033[1;32m", stream);
		}
	}

	if (flags & HEXPFL_BYTE && flags & HEXDFL_BINDUMP) {
		for (int8_t i = 7; i >= 0; --i, ret++)
			fputc(ch & (1 << i) ? '1' : '0', stream);
	} else if (flags & HEXPFL_BYTE && flags & HEXDFL_UPPER) {
		fprintf(stream, "%02X", ch);
		ret += 2;
	} else if (flags & HEXPFL_BYTE) {
		fprintf(stream, "%02x", ch);
		ret += 2;
	} else {
		if (ch < 32 || ch > 127) {
			fputc('.', stream);
		} else {
			fputc(ch, stream); 
		}

		ret++;
	}

	if (flags & HEXDFL_COLORED) fputs("\033[0m", stream);
	return ret;
}

int chexdump(FILE *restrict stream, int source, unsigned char *restrict buf, size_t bufsiz, struct hexdumping *restrict hexfl)
{	
	ssize_t readbytes;
	ssize_t j = 0, i = 0;

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

	while ((readbytes = read(source, buf, bufsiz)) > 0) {
		for (i = 0; i < readbytes; ++i, ++j) {
			if (j != 0 && (j % hexfl->hex_columns) == 0) {
				fputc('\n', stream);
				if (hexfl->hex_filename) fputc('\t', stream);
			} else if (j != 0) {
				fputs(", ", stream);
			}
			
			if (hexfl->hex_flags & HEXDFL_UPPER) {
				fprintf(stream, "0x%02hhX", buf[i]);
			} else {
				fprintf(stream, "0x%02hhx", buf[i]);
			}
		}
	}
	
	if (readbytes < 0) return -1;
	
	if (hexfl->hex_filename) fprintf(stream, "\n};\nunsigned int %s_len = %zu;", hexfl->hex_filename, j); 
	if (hexfl->hex_filename || (j % hexfl->hex_columns) != 0) fputc('\n', stream);

	return 0;
}

int hexdump(FILE *restrict stream, int source, unsigned char *restrict buf, size_t bufsiz, struct hexdumping *restrict hexfl)
{
	ssize_t readbytes;
	ssize_t j = lseek(source, 0, SEEK_CUR);
	ssize_t i = 0, k = 0;
	
	int dumplen = 0, digits = 2;
	int used_digits = 0, used_spaces = 0;
	
	if (source == STDIN_FILENO) j = 0;
	if (hexfl->hex_flags & HEXDFL_BINDUMP) digits = 8;
	if (!(hexfl->hex_flags & HEXDFL_FORCE_COLUMNS)) {
		if (hexfl->hex_flags & HEXDFL_C) {
			hexfl->hex_columns = 12;
		} else if (hexfl->hex_flags & HEXDFL_BINDUMP) {
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
		} else if (hexfl->hex_flags & HEXDFL_BINDUMP) {
			hexfl->hex_wordsize = 1;
		} else {
			hexfl->hex_wordsize = 2;
		}
	}

	if (hexfl->hex_flags & HEXDFL_C) return chexdump(stream, source, buf, bufsiz, hexfl);

	dumplen = (hexfl->hex_columns * digits) + (hexfl->hex_columns / hexfl->hex_wordsize) + 1;

	while ((readbytes = read(source, buf, bufsiz)) > 0) {
		i = 0;
		while (i < readbytes) {
			if (!(hexfl->hex_flags & HEXDFL_NOCOUNT)) {
				if (hexfl->hex_flags & HEXDFL_DECCOUNT) {
					fprintf(stream, "%010zd", j);
				} else if (hexfl->hex_flags & HEXDFL_UPPER) {
					fprintf(stream, "%010zX", j);
				} else {
					fprintf(stream, "%010zx", j);
				}

				fputs(hexfl->hex_off_separator, stream); 
			}
			
			k = used_digits = used_spaces = 0;
			while (k < hexfl->hex_columns && i < readbytes) {
				if (!(hexfl->hex_flags & HEXDFL_NOCOUNT && k == 0))  {
					used_spaces++;
					fputc(' ', stream);
				}

				for (ssize_t m = 0; m < hexfl->hex_wordsize && k < hexfl->hex_columns && i < readbytes; ++m, ++k, ++j, ++i) 
					used_digits += hexprint(stream, buf[i], hexfl->hex_flags | HEXPFL_BYTE);
			}
	
			if (!(hexfl->hex_flags & HEXDFL_PLAIN)) {
				fprintf(stream, "%*s", (dumplen - used_digits - used_spaces), "");
				fputs(hexfl->hex_dump_separator, stream);
				for (k = i - k; k < i; ++k)
					hexprint(stream, buf[k], hexfl->hex_flags);
			}

			fputc('\n', stream);
		}
	}

	if (readbytes < 0) return -1;
	return 0;
}

int __xd__(int argc, char **argv)
{
	int ch, fd;

	int exitcode = EXIT_SUCCESS;

	struct hexdumping *hexfl = & (struct hexdumping) {.hex_dump_separator = " ", .hex_off_separator = ":"};

	size_t bufsiz = (sysconf(_SC_PAGE_SIZE) * 4);
	size_t outbufsiz = bufsiz;
	
	unsigned char *buf = malloc(bufsiz);
	char *outbuf = malloc(outbufsiz);

	if (!buf) {
		buf = (unsigned char[32]) {0}; 
		bufsiz = 32;
	} 

	if (!outbuf) {
		outbuf = (char [32]) {0};
		outbufsiz = 32;
	}

	setvbuf(stdout, outbuf, _IOFBF, outbufsiz); 

	opterr = 0;
	while ((ch = getopt(argc, argv, ":DK:L:OR:bc:dig:n:o:prs:u")) != -1) {
		switch (ch) {
		case '?':
			if (optopt == '?') {
				dprintf(STDOUT_FILENO, __XD_USAGE__, argv[0], argv[0], argv[0], argv[0]);
				return EXIT_SUCCESS;
			}
			
			dprintf(STDERR_FILENO, "%s: Illegal option: %c.\n", argv[0], optopt);
			dprintf(STDERR_FILENO, __XD_USAGE__, argv[0], argv[0], argv[0], argv[0]);
			return EXIT_FAILURE;
		case ':':
			dprintf(STDERR_FILENO, "%s: Missing option argument: %c.\n", argv[0], optopt);
			dprintf(STDERR_FILENO, __XD_USAGE__, argv[0], argv[0], argv[0], argv[0]);
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
		case 'b': hexfl->hex_flags |= HEXDFL_BINDUMP; break;
		case 'c':
			hexfl->hex_columns = (uint32_t) strtoull(optarg, NULL, 0);
			hexfl->hex_flags |= HEXDFL_FORCE_COLUMNS;
			if (hexfl->hex_columns == 0 && *optarg != '0') {
				dprintf(STDERR_FILENO, "%s: Illegal option argument, expected integer number.\n", argv[0]);
				dprintf(STDERR_FILENO, __XD_USAGE__, argv[0], argv[0], argv[0], argv[0]);
				return EXIT_FAILURE;
			}

			if (hexfl->hex_columns == 0) hexfl->hex_columns = INT_MAX;	

			break;
		case 'd': hexfl->hex_flags |= HEXDFL_DECCOUNT; break;
		case 'i': hexfl->hex_flags |= HEXDFL_C; hexfl->hex_flags &= ~HEXDFL_PLAIN; break;
		case 'g':
			hexfl->hex_wordsize = (uint32_t) strtoull(optarg, NULL, 0);
			hexfl->hex_flags |= HEXDFL_FORCE_WORDSIZE;
			if (hexfl->hex_wordsize == 0 && *optarg != '0') {
				dprintf(STDERR_FILENO, "%s: Illegal option argument, expected integer number.\n", argv[0]);
				dprintf(STDERR_FILENO, __XD_USAGE__, argv[0], argv[0], argv[0], argv[0]);
				return EXIT_FAILURE;
			}

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
			if ((fd = open(optarg, O_RDWR | O_CREAT, S_IWUSR | S_IRUSR)) < 0) {
				dprintf(STDERR_FILENO, "%s: '%s': Unable to output open file: %s (%d).\n", argv[0], optarg, strerror(errno), errno);
				return EXIT_FAILURE;
			}

			dup2(fd, STDOUT_FILENO);
			break;
		case 'p':
			hexfl->hex_flags &= ~(HEXDFL_C | HEXDFL_MASK_COLOR);
			hexfl->hex_flags |= (HEXDFL_NOCOUNT | HEXDFL_PLAIN | HEXDFL_NOCOLOR);
			hexfl->hex_off_separator = "";
			break;
		case 'u': hexfl->hex_flags |= HEXDFL_UPPER; break;
		}
	}
	
	if (!(hexfl->hex_flags & HEXDFL_MASK_COLOR) && isatty(STDOUT_FILENO) && !getenv("NO_COLOR")) 
		hexfl->hex_flags |= HEXDFL_COLORED;

	if (!argv[optind]) argv[optind] = "-";
	
	do {
		if (argv[optind] && argv[optind][0] == '-' && !argv[optind][1]) {
			fd = STDIN_FILENO;
		} else if ((fd = open(argv[optind], O_RDONLY)) < 0) {
			dprintf(STDERR_FILENO, "%s: '%s': Unable to open file: %s (%d).\n", argv[0], argv[optind], strerror(errno), errno);
			exitcode = EXIT_FAILURE; 
			continue; 
		}

		if (!(hexfl->hex_flags & HEXDFL_AUTONAME)) hexfl->hex_filename = argv[optind];
		if (hexdump(stdout, fd, buf, bufsiz, hexfl) < 0) {
			dprintf(STDERR_FILENO, "%s: '%s': Unable to hexdump: %s (%d).\n", argv[0], argv[optind], strerror(errno), errno);
			exitcode = EXIT_FAILURE;
		}

		if (fd) close(fd); 
	} while ((++optind) < argc);

	return exitcode;
}
