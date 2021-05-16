/*************************************************************************
*   File: files.h                    Part of Aliens vs Predator: The MUD *
*  Usage: Header for file handling code                                  *
*************************************************************************/

char *	fread_string(FILE *fl, char *error, char *filename);
char	fread_letter(FILE *fp);
int		fread_number(FILE *fp);
void	fread_to_eol(FILE *fp);
char *	fread_word(FILE *fp);

SInt32 fread_block(char *block, FILE *fp, char *error, char *filename);
SInt32 fread_keyblock(char *block, FILE *fp, char *error, char *filename);

extern void tar_restore_file(char *filename);

UInt32 asciiflag_conv(char *flag);

