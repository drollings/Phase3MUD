/***************************************************************************\
[*]    __     __  ___                ___  | LexiMUD is a C++ MUD codebase [*]
[*]   / /  ___\ \/ (_) /\/\  /\ /\  /   \ |        for Sci-Fi MUDs        [*]
[*]  / /  / _ \\  /| |/    \/ / \ \/ /\ / | Copyright(C) 1997-1999        [*]
[*] / /__|  __//  \| / /\/\ \ \_/ / /_//  | All rights reserved           [*]
[*] \____/\___/_/\_\_\/    \/\___/___,'   | Chris Jacobson (FearItself)   [*]
[*]      A Creation of the AvP Team!      | fear@technologist.com         [*]
[-]---------------------------------------+-------------------------------[-]
[*] File : files.c++                                                      [*]
[*] Usage: File handling code                                             [*]
\***************************************************************************/


#include "types.h"

#include <sys/stat.h>

#include "utils.h"
#include "buffer.h"
#include "internal.defs.h"
#include "character.defs.h"
#include "files.h"
#include "strings.h"


// External variables
extern FILE * logfile;

char fread_letter(FILE *fp) {
	char c;
	do {
		c = getc(fp);
	} while (isspace(c));
	return c;
}


/* read and allocate space for a '~'-terminated string from a given file */
char *fread_string(FILE * fl, char *error, char *filename) {
	char	*buf = get_buffer(MAX_STRING_LENGTH),
			*tmp = get_buffer(MAX_STRING_LENGTH),
			*rslt;
	char *point;
	int done = 0;
    UInt32 length = 0, templength = 0;

	do {
		if (!fgets(tmp, 512, fl)) {
			log("SYSERR: fread_string: format error at or near %s", error);
			tar_restore_file(filename);
			exit(1);
		}
/* If there is a '~', end the string; else put an "\r\n" over the '\n'. */
		if ((point = strchr(tmp, '~')) != NULL) {
			*point = '\0';
			done = 1;
		}
//			else
		else if ((point = strchr(tmp, '\n')))
//			point = tmp + strlen(tmp) - 1;
		{
			*(point++) = '\r';
			*(point++) = '\n';
			*point = '\0';
		}

		templength = strlen(tmp);

		if (length + templength >= MAX_STRING_LENGTH) {
			log("SYSERR: fread_string: string too large (db.c)");
			log(error);
			tar_restore_file(filename);
			exit(1);
		} else {
			strcat(buf + length, tmp);
			length += templength;
		}
	} while (!done);

/* allocate space for the new string and copy it */
	if (strlen(buf) > 0) {
		CREATE(rslt, char, length + 1);
		strcpy(rslt, buf);
	} else
		rslt = NULL;

	release_buffer(buf);
	release_buffer(tmp);
	return rslt;
}




/*
* Read a number from a file.
*/
int fread_number(FILE *fp) {
	SInt32	number;
	bool	sign;
	char	c;

	do {
		c = getc(fp);
	} while (isspace(c));

	number = 0;

	sign = false;
	if (c == '+') {
		c = getc(fp);
	} else if (c == '-') {
		sign	= true;
		c		= getc(fp);
	}


	if (!isdigit(c)) {
		log("Fread_number: bad format.");
		exit(1);
	}

	while (isdigit(c)) {
		number	= number * 10 + c - '0';
		c		= getc(fp);
	}

	if (sign)
		number = 0 - number;

	if (c == '|')
		number += fread_number(fp);
	else if (c != ' ')
		ungetc(c, fp);

	return number;
}

/*
* Read to end of line (for comments).
*/
void fread_to_eol(FILE *fp) {
	char c;

	do {
		c = getc(fp);
	} while (c != '\n' && c != '\r');

	do {
		c = getc(fp);
	} while (c == '\n' || c == '\r');

	ungetc(c, fp);
	return;
}


/*
* Read one word (into static buffer).
*/
char *fread_word(FILE *fp) {
	static char word[MAX_INPUT_LENGTH];
	char *pword;
	char cEnd;

	do {
		cEnd = getc(fp);
	} while (isspace(cEnd));

	if (cEnd == '\'' || cEnd == '"') {
		pword   = word;
	} else {
		word[0] = cEnd;
		pword   = word+1;
		cEnd    = ' ';
	}

	for (; pword < word + MAX_INPUT_LENGTH; pword++) {
		*pword = getc(fp);
		if (cEnd == ' ' ? isspace(*pword) || *pword == '~' : *pword == cEnd) {
			if (cEnd == ' ' || cEnd == '~')
				ungetc(*pword, fp);
			*pword = '\0';
			return word;
		}
	}

	log("SYSERR: Fread_word: word too long.");
	exit(1);
	return NULL;
}


SInt32 fread_block(char *block, FILE *fp, char *error, char *filename) {
	int		c = '\0';
	SInt32	number = 0;
	SInt32	i;
	char *	point = block;
	
	c = getc(fp);
	while (c != EOF && c != '#')	c = getc(fp);
	if (c == EOF)					return -1;
/*	while (c != -1 && !isdigit(c))	c = getc(fp);
	if (c == -1 || !isdigit(c))		return -1;
	while (isdigit(c)) {
		number = number * 10 + c - '0';
		c = getc(fp);
	}
*/
	number = fread_number(fp);
	
	while ((c != EOF) && (c != '{'))	c = getc(fp);
	
	c = getc(fp);
	for (i = 1; (c != EOF) && i; c = getc(fp)) {
		if (c == '{')		i++;
		else if (c == '}')	i--;
		else if (c == '"') {
			if (point)	*(point++) = c;
			for (c = getc(fp); (c != EOF) && (c != '"'); c = getc(fp)) {
				if (point)	*(point++) = c;
				if (c == '\\') {
					c = getc(fp);
					if (c == EOF)	break;
					if (point)	*(point++) = c;
				}
			}
		}
		if (c == EOF)	break;
		if (i && point)	*(point++) = c;
	}
	
//	while ((c != -1) && i) {
//		*(point++) = c = getc(fp);
//	}	
	
	if (point)	*point = '\0';
	
	return (c != EOF) ? number : -1;
}


SInt32 fread_keyblock(char *block, FILE *fp, char *error, char *filename) {
	int		c = '\0';
	SInt32	number = 0;
	SInt32	i;
	char *	point = block;
	
//	c = getc(fp);
//	while (c != EOF && c != '#')	c = getc(fp);
//	if (c == EOF)					return -1;
/*	while (c != -1 && !isdigit(c))	c = getc(fp);
	if (c == -1 || !isdigit(c))		return -1;
	while (isdigit(c)) {
		number = number * 10 + c - '0';
		c = getc(fp);
	}
*/
	while ((c != EOF) && (c != '{'))	c = getc(fp);
	if (c == EOF)						return -1;
	
	c = getc(fp);
	for (i = 1; (c != EOF) && i; c = getc(fp)) {
		if (c == '{')		i++;
		else if (c == '}')	i--;
		else if (c == '"') {
			if (point)	*(point++) = c;
			for (c = getc(fp); (c != EOF) && (c != '"'); c = getc(fp)) {
				if (point)	*(point++) = c;
				if (c == '\\') {
					c = getc(fp);
					if (c == EOF)	break;
					if (point)	*(point++) = c;
				}
			}
		}
		if (c == EOF)	break;
		if (i && point)	*(point++) = c;
	}
	
	if (point)	*point = '\0';
	
	return (c != EOF) ? number : -1;
}


#if 0
SInt32 fstream_block(char *block, ifstream &file, char *error, char *filename) {
	char	c = '\0';
	SInt32	number = 0;
	SInt32	i;
	char *	point = block;
	
	file.ignore(32768, '#');
	
//	number = fread_number(file);
	file >> number;
	
	file.ignore(32768, '{');
	
	for (file.get(c), i = 1; file.good() && i; file.get(c)) {
		if (c == '{')		i++;
		else if (c == '}')	i--;
		else if (c == '"') {
			if (point)	*(point++) = c;
			for (file.get(c); file.good() && (c != '"'); file.get(c)) {
				if (point)	*(point++) = c;
				if (c == '\\') {
					file.get(c);
					if (!file.good())	break;
					if (point)			*(point++) = c;
				}
			}
		}
		if (!file.good())	break;
		if (i && point)		*(point++) = c;
	}
	
	if (point)	*point = '\0';
	
	return (file.good()) ? number : -1;
}
#endif


UInt32 asciiflag_conv(char *flag) {
	UInt32 flags = 0;
	bool is_number = true;
	char *p;

	for (p = flag; *p; p++) {
		if (islower(*p))
			flags |= 1 << (*p - 'a');
		else if (isupper(*p))
			flags |= 1 << (26 + (*p - 'A'));

		if (!isdigit(*p))
			is_number = false;
	}

	if (is_number)
		flags = atol(flag);

	return flags;
}


void tar_restore_file(char *filename) 
{
#ifdef GOT_RID_OF_IT
#ifndef macintosh
	char	*archive = get_buffer(120),
			*mangled = get_buffer(70),
			*buf = get_buffer(MAX_INPUT_LENGTH);
	FILE *file;
	FILE *tmp;
	int i = 0;
	bool wrote = false;

	mudlogf(NRM, 0, TRUE, "CORRUPT: %s", filename);

	// Get the initial mangled filename
	sprintf(mangled, "%s.mangled%i", filename, i);


	// Increment i to get a new ???.zon.mangled"i" file name

	for (i = 0; (file = fopen(mangled, "r")); i++) {
		fclose(file);
		sprintf(mangled, "%s.mangled%i", filename, i);
	}

	mudlogf(NRM, 0, TRUE,  "CORRUPT: Swapping to %s", mangled);

	rename(filename, mangled);
	
	for (i = 0; !wrote && (i <= 3); i++) {
		switch (i) {
			case 0:
			case 1:
			case 2:
				sprintf(archive, "./backup/backup_incr_%d.tar.gz", i + 1);
				break;
			case 3:
				sprintf(archive, "./backup/backup_full.tar.gz");
				break;
		}
		sprintf(buf, "pushd ..; tar -zxvf %s %s%s; popd", archive, "lib/", filename);
		mudlogf(CMP, 0, TRUE, buf);
		if (!(tmp = popen(buf, "w")))	mudlog("CORRUPT: Error on port open!", CMP, 0, TRUE);
		else							pclose(tmp);
		if ((tmp = fopen(filename, "r"))) {
			wrote = true;
			fclose(tmp);
		}
	}
	release_buffer(buf);
	release_buffer(archive);
	release_buffer(mangled);
#endif
#else
  FILE *file;

  if ((file = fopen("mangled", "a")) == NULL) {
    log ("Can't open troublefiles for writing names of corrupt files!  Yikes!");
    exit (1);
  }

  fprintf(file, "%s\n", filename);
  fclose(file);
#endif
}


void fprintstring(FILE *fp, const char *fmt, const char *txt);
void fprintbits(FILE *fp, const char *fmt, UInt32 bits, const char *bitnames[]);


void fprintbits(FILE *fp, const char *fmt, UInt32 bits, const char *bitnames[]) {
	UInt32	nr;
	char *	result = get_buffer(MAX_INPUT_LENGTH);
	bool	bitAlreadySet = false;
	
	*result = '\0';
	
	for (nr = 0; bits; bits >>= 1) {
		if (IS_SET(bits, 1)) {
			if (*bitnames[nr] != '\n') {
				if (bitAlreadySet)	strcat(result, " ");
				strcat(result, bitnames[nr]);
			}
			bitAlreadySet = true;
		}
		if (*bitnames[nr] != '\n')	nr++;
		else						break;
	}
    
    fprintf(fp, fmt, result);
	
	release_buffer(result);
}


void fprintstring(FILE *fp, const char *fmt, const char *txt) {
	char *	result = get_buffer(MAX_STRING_LENGTH * 4);
	char *	resultPoint = result;
	const char *point;

	*(resultPoint++) = '"';
	for (point = txt; *point; point++) {
		switch (*point) {
			case '"':
				*(resultPoint++) = '\\';
				*(resultPoint++) = '"';
				break;
			case '\\':
				*(resultPoint++) = '\\';
				*(resultPoint++) = '\\';
				break;
			case '\n':
				*(resultPoint++) = '\\';
				*(resultPoint++) = 'n';
				if (*(point+1))
					*(resultPoint++) = '\n';
				break;
			case '\r':
				break;
			default:
				*(resultPoint++) = *point;
		}
	}
	*(resultPoint++) = '"';
	*(resultPoint++) = '\0';
	
	fprintf(fp, fmt, result);
	
	release_buffer(result);
};


#if defined(macintosh)
class File {
public:
					File(const char *name);
					~File(void);

	void			Open(void);
	void			Close(void);
	const char *	GetName(void)		{ return filename; }
	
	bool			eof(void) const		{ return (ptr >= end); }
	SInt32			size(void) const	{ return (end - cache); }
	
	char			GetChar(void);
	
protected:
	char *			filename;
	const char *	cache;
	struct stat		stats;

	const char *	ptr;
	const char *	end;
	SInt32			line;
private:
};


char File::GetChar(void) {
	if (eof())
		return '\0';
	if (*ptr == '\n')
		line++;
	return *ptr++;
}





File::File(const char *name) : cache(NULL), ptr(NULL), end(NULL), line(0) {
	filename = str_dup(name);
	Open();
}


File::~File(void) {
	Close();
	if (filename)	free(filename);
}


void File::Open(void) {
	SInt32		fd, num, total = 0;
	char *		tcache;

	if ((fd = ::open(filename, O_RDONLY /* | O_BINARY */ )) < 0)
		return;
	
	if (fstat(fd, &stats) < 0)
		return;
	
	tcache = new char[stats.st_size + 1];
	while (total < stats.st_size) {
		if ((num = ::read(fd, (tcache + total), stats.st_size - total)) == -1) {
			perror("File::Open:read");
			exit(0);
		} else if ((total += num) == stats.st_size)
			break;
		
		continue;
	}
	*(tcache + stats.st_size) = 0;

	::close(fd);	//	Stored in RAM, lets kill the file
	cache = tcache;
	ptr = tcache;
	end = tcache + stats.st_size;
}


void File::Close(void) {
	if (!cache)
		return;
	
	delete [] cache;
	
	cache = NULL;
	ptr = NULL;
	end = NULL;
}

#endif
