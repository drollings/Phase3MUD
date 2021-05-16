/* ************************************************************************
*   File: editor.cc                                      Part of CircleMUD *
*  Usage: Run-time modification of game variables                         *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
**************************************************************************/



#include "structs.h"
#include "utils.h"
#include "editor.h"
#include "buffer.h"
#include "interpreter.h"
#include "scripts.h"
#include "characters.h"
#include "objects.h"
#include "handler.h"
#include "find.h"
#include "db.h"
#include "comm.h"
#include "mail.h"
#include "boards.h"
#include "olc.h"
#include "skills.h"

// Demands here being what they are, we'll allocate this.
static char editbuf[32 * 1024];

/*
 * Function declarations.
 */
void show_string(Descriptor *d, char *input);
int string_freq(const char *searchin, const char *searchfor);

void format_string(string &str, bool indent, StrLenInt maxlen, unsigned int rmargin = 78);
void delete_string(Descriptor *d, string &str, const char *arg);
void edit_string(Descriptor *d, string &str, const char *arg, StrLenInt maxlen);
void insert_string(Descriptor *d, string &str, const char *arg, StrLenInt maxlen);
void list_string(Descriptor *d, string &str, const char *arg);
void list_string_num(Descriptor *d, string &str, const char *arg);
void editor_replace(Descriptor *d, string &str, char *arg, bool replace_all, StrLenInt maxlen);
void line_range(string &str, const char *arg, int &low, int &high, bool required = false);


const char *next_page(const char *str);
int count_pages(const char *str);
void paginate_string(const char *str, Descriptor *d);
ACMD(do_skillset);
extern char *MENU;
extern char *PLAINMENU;


void TextEditor::SaveInternally(void)
{
	if (ptr) {
		if (*ptr)
			delete *ptr;
		*ptr = str_dup(str.c_str());
	}
}

	
void TextEditor::Menu(void)
{
			d->Write("Editor command formats: /<letter>\r\n\r\n"
					 "/a         -  aborts editor\r\n"
					 "/c         -  clears buffer\r\n"
					 "/d#        -  deletes a line #\r\n"
					 "/e# <text> -  changes the line at # with <text>\r\n"
					 "/f         -  formats text\r\n"
					 "/fi        -  indented formatting of text\r\n"
					 "/h         -  list text editor commands\r\n"
					 "/i# <text> -  inserts <text> before line #\r\n"
					 "/l         -  lists buffer\r\n"
					 "/n         -  lists buffer with line numbers\r\n"
					 "/r 'a' 'b' -  replace 1st occurance of text <a> in buffer with text <b>\r\n"
					 "/ra 'a' 'b'-  replace all occurances of text <a> within buffer with text <b>\r\n"
					 "              usage: /r[a] 'pattern' 'replacement'\r\n"
					 "/s         -  saves text\r\n");
}


void TextEditor::Parse(char *arg)
{
	int		i = 2, 
			j = 0;
	char	*tilde,
			*actions = get_buffer(MAX_INPUT_LENGTH);
	bool	terminator = false,
			extra = false;	// For /ra, /fi, etc.

	delete_doubledollar(arg);
	
	while ((tilde = strchr(arg, '~')))
		*tilde = ' ';
	
	if (*arg == '/') {
		while (arg[i] != '\0') {
			actions[j] = arg[i];              
			i++;
			j++;
						}             
		actions[j] = '\0';
		*arg = '\0';

		switch (arg[1]) {
		case 'a':
			terminator = true;
						break;
		case 'c':
			if (str.size()) {
				d->Write("Current buffer cleared.\r\n");
				save = 1;
			} else					d->Write("Current buffer empty.\r\n");
			str.resize(0);
						break;
		case 'd':
			if (str.size()) {
				delete_string(d, str, arg + 2);
				save = 1;
			} else					d->Write("Current buffer empty.\r\n");
			break;
		case 'e':
			if (str.size()) {
				edit_string(d, str, arg + 2, max_str);
				save = 1;
			} else					d->Write("Current buffer empty.\r\n");
			break;
		case 'f': 
			if (str.size()) {
				format_string(str, (arg[2] == 'i' ? true : false), max_str);
				d->Writef("Text formatted with%s indent.\r\n", (arg[2] == 'i' ? "" : "out")); 
				save = 1;
				 }     
			else					d->Write("Current buffer empty.\r\n");
			break;
		case 'i':
			if (str.size()) {
				insert_string(d, str, arg + 2, max_str);
				save = 1;
			} else					d->Write("Current buffer empty.\r\n");
			break;
		case 'h': 
			Menu();
			break;
		case 'l':
			if (str.size())		list_string(d, str, arg + 2);
			else					d->Write("Current buffer empty.\r\n");
			break;
		case 'n':
			if (str.size())		list_string_num(d, str, arg + 2);
			else					d->Write("Current buffer empty.\r\n");
			break;
		case 'r':   
			if (str.size()) {
				extra = (arg[2] == 'a');
				editor_replace(d, str, arg + 2, extra, max_str);
						}             
			else					d->Write("Current buffer empty.\r\n");
			save = 1;
			break;
		case 's':
			SaveInternally();
			terminator = true;
						break;
					default:
			d->Write("Invalid option.\r\n");
						break;
				}     
	} else {
		if ((str.size() + strlen(arg)) >= max_str - 3) {
			d->Write("String too long, limit reached on message.  Last line ignored.\r\n");
		} else {
			str += arg;
			str += "\r\n";
			save = 1;
		}
	}

	release_buffer(actions);

	if (!terminator)	
		return;

	Finish(All);
}


void TextEditor::Finish(FinishMode mode)
{
	if (d->character && (d->character->InRoom() != -1) && (prev_editor == NULL)) {
		act("$n stops writing.", TRUE, d->character, 0, 0, TO_ROOM);
		REMOVE_BIT(PLR_FLAGS(d->character), PLR_WRITING | PLR_MAILING);
	}
	Editor::Finish();
}

            
SStringEditor::SStringEditor(Descriptor *desc, SString **edstring, StrLenInt maxlen) :
				TextEditor(desc, NULL, maxlen), ptr(edstring)
{ 	if (edstring && *edstring) {
		str = (*edstring)->str;	
		desc->Write((*edstring)->str);
	}
};

void SStringEditor::SaveInternally(void)
{
	if (*ptr) {
		if ((*ptr)->count <= 1) {
			(*ptr)->Free();
			*ptr = NULL;

			if (str.size()) {
				*ptr = SString::Create(str.c_str());
			}
		} else {
			(*ptr)->Free();
			delete (*ptr)->str;
			(*ptr)->str = str_dup(str.c_str());
		}
	} else {
		*ptr = SString::Create(str.c_str());
	}
}


void FileEditor::SaveInternally(void)
{
	FILE *fl;

	TextEditor::SaveInternally();

	if (!(fl = fopen(filename, "w"))) {
		mudlogf(BRF, NULL, TRUE, "SYSERR: Can't write file '%s'.", filename);
				} else {
		if (str.size()) {
			char *buf1 = get_buffer(max_str);
			strcpy(buf1, str.c_str());
			strip_string(buf1);
			fputs(buf1, fl);
			release_buffer(buf1);
				}
		fclose(fl);
		mudlogf(NRM, d->character, TRUE, "OLC: %s saves '%s'.", d->character->RealName(), filename);
		d->Write("Saved.\r\n");
	}
}


void MailEditor::SaveInternally(void)
{
	FILE *fl;
	char *tmstr;
	time_t mytime;

	strcpy(buf, str.c_str());

		store_mail(mail_to, GET_IDNUM(d->character), buf);
	d->Write("Message sent!\r\n");
}


void BoardEditor::SaveInternally(void)
{
	TextEditor::SaveInternally();	

	Board::Save(board);
	d->Write("Message posted!\r\n");
}


CmdlistEditor::CmdlistEditor(Descriptor *desc, Cmdlist **edit, StrLenInt maxlen) :
	TextEditor(desc, NULL, maxlen), ptr(edit)
{
	CmdlistElement *cmd;

	if (ptr && *ptr) {
		for (cmd = (*ptr)->command; cmd; cmd = cmd->next) {
			if (cmd->cmd)	str += cmd->cmd;
			str += "\r\n";
		}
	} else {
		str = "* Empty script\r\n";
	}

	desc->Write(str.c_str());
};


void CmdlistEditor::SaveInternally(void)
{
	char 		*buf = get_buffer(MAX_STRING_LENGTH * 3),
				*s = buf;
	CmdlistElement *cmd;

	if (*ptr) {
		(*ptr)->Free();
	}

	if (str.size() <= 0) {
		strcpy(buf, "* Empty script\r\n");
	} else {
		strcpy(buf, str.c_str());
	}

	*ptr = new Cmdlist(strtok(s, "\n\r"));
	cmd = (*ptr)->command;
	while ((s = strtok(NULL, "\n\r"))) {
		cmd->next = new CmdlistElement(s);
		cmd = cmd->next;
	}
	release_buffer(buf);
}


void format_string(string &str, bool indent, StrLenInt maxlen, unsigned int rmargin = 78)
{
	unsigned int	total_chars;
	int				return_found = 0,
					found_paragraph = 0,
					found_char = 0;

	char			*flow,
					*start = NULL,
					temp;

	bool			cap_next = true, 
					cap_next_next = false;

	string			formatted;
	char 			*ptr_string = str_dup(str.c_str());

	flow = ptr_string;

	if (!flow) 		return;

	if (indent) {
		formatted = "   ";
		total_chars = 3;
	} else {
		total_chars = 0;
	} 

	while (*flow != '\0') {
		found_paragraph = 0;
		found_char = 0;

		while (!found_char) {
			switch (*flow) {
			case '\n':
				++found_paragraph;
				++flow;
			break;
			case '\r':
			case '\f':
			case '\t':
			case '\v':
			case ' ':
				++flow;
					break;
			default:
				found_char = true;
					break;
			}
		}
      
		if (found_paragraph >= 2) {
			found_paragraph = 0;
			formatted += "\r\n\r\n";
			total_chars = 0;
			if (indent) {
				formatted += "   ";
				total_chars += 3;
					}
				}
	 
	    if (*flow != '\0') {

			start = flow++;
			while ((*flow != '\0') &&
					(*flow != '\n') &&
					(*flow != '\r') &&
					(*flow != '\f') &&
					(*flow != '\t') &&
					(*flow != '\v') &&
					(*flow != ' ') &&
					(*flow != '.') &&
					(*flow != '?') &&
					(*flow != '!'))
				++flow;

			if (cap_next_next) {
				cap_next_next = false;
				cap_next = true;
			}

			/* this is so that if we stopped on a sentence .. we move off the sentence delim. */
			while ((*flow == '.') || (*flow == '!') || (*flow == '?')) {
				cap_next_next = true;
				++flow;
			}
	 
			temp = *flow;
			*flow = '\0';

			if ((total_chars + strlen(start) + 1) > rmargin) {
				formatted += "\r\n";
				total_chars = 0;
			}

			if (cap_next == false) {
				if (total_chars > 0) {
					formatted += " ";
					total_chars++;
					}
			} else {
				cap_next = false;
				*start = UPPER(*start);
			}

			total_chars += strlen(start);
			formatted += start;

			*flow = temp;
	    }

	    if (cap_next_next) {
			if ((total_chars + 3) > rmargin) {
				formatted += "\r\n";
				total_chars = 0;
			} else {
				formatted += "  ";
				total_chars += 2;
			}
		}
	}
	formatted += "\r\n";

	if (formatted.size() > maxlen)
		formatted.resize(maxlen - 1);

	str = formatted;
}


// This function will scan a string for a range of lines
void line_range(string &str, const char *arg, int &low, int &high, bool required = false)
{
	low = 1;
	high = 30000;

	if (*arg) {
		switch (sscanf(arg, " %d - %d ", &low, &high)) {
		case 1:		high = low;		break;
		}
	} else if (required) {
		low = -1;
		high = -1;
	}
}


// This function will scan a string for a range of lines
void line_range_offset(	string &str, int const &low_line, int const &high_line , 
						int &low_offset, int &high_offset)
{
	int i, tmppos;
	
	low_offset = -1;
	high_offset = -1;

	if (low_line > 1) {
		for (i = 1; i < low_line; ++i) {
			tmppos = str.find("\n", low_offset);
			if (tmppos < 0)		return;
			else				low_offset = tmppos + 1;	// Here, we skip the previous carriage return.
			}
				} else {
		low_offset = 0;
	}

	high_offset = low_offset;

	for (i = low_line; i <= high_line; ++i) {
		tmppos = str.find("\n", high_offset);	// Here, we include the following carriage return.
		if (tmppos < 0)		return;
		else				high_offset = tmppos + 1;
				}
}


void list_string(Descriptor *d, string &str, const char *arg)
{
	int		linelow, 
			linehigh,
			i;
	StrLenInt total_len;
	char *s, *t, temp;
  
	*editbuf = '\0';

	skip_spaces(arg);

	if (*arg) {
		strcpy(buf2, str.c_str());	

		line_range(str, arg, linelow, linehigh);
		if (linelow < 1) {
				d->Write("Line numbers must be greater than 0.\r\n");
				return;
		} else if (linehigh < linelow) {
				d->Write("That range is invalid.\r\n");
				return;
			}
			
		if ((linehigh < 30000) || (linelow > 1))
			sprintf(editbuf, "Current buffer range [%d - %d]:\r\n", linelow, linehigh);
			i = 1;
			total_len = 0;
		s = buf2;
		while (s && (i < linelow))
				if ((s = strchr(s, '\n')) != NULL) {
					i++;
					s++;
				}
		if ((i < linelow) || !s) {
				d->Write("Line(s) out of range; no buffer listing.\r\n");
				return;
			}
      
			t = s;
		while (s && (i <= linehigh))
				if ((s = strchr(s, '\n')) != NULL) {
					i++;
					total_len++;
					s++;
				}
			if (s)	{
				temp = *s;
				*s = '\0';
			strcat(editbuf, t);
				*s = temp;
		} else 
			strcat(editbuf, t);

// 		line_range(str, arg, linelow, linehigh);
// 		if (linelow < 1) {
// 			d->Write("Line numbers must be greater than 0.\r\n");
// 			return;
// 		} else if (linehigh < linelow) {
// 			d->Write("That range is invalid.\r\n");
// 			return;
// 		}
// 
// 		line_range_offset(str, linelow, linehigh, poslow, poshigh);
// 		if (poslow < 0 || poshigh < 0) {
// 			d->Write("Line(s) out of range; no buffer listing.\r\n");
// 			return;
// 		}
// 		copylen = poshigh - poslow;
// 		str.copy(buf, copylen, poslow);
// 		buf[copylen] = '\0';
// 		strcat(buf, "\r\n");

		page_string(d, editbuf, true);
			} else {
		page_string(d, str.c_str(), true);
			}
}

      
void list_string_num(Descriptor *d, string &str, const char *arg)
{
	int		linelow = 1, 
			linehigh = 30000,
			i;
	StrLenInt total_len;
	char *s, *t, temp;

	skip_spaces(arg);


	if (*arg) {
		line_range(str, arg, linelow, linehigh);
		if (linelow < 1) {
				d->Write("Line numbers must be greater than 0.\r\n");
				return;
		} else if (linehigh < linelow) {
				d->Write("That range is invalid.\r\n");
				return;
			}
	}
			
	strcpy(buf2, str.c_str());	
	*editbuf = '\0';
			
			i = 1;
			total_len = 0;
	s = buf2;
	while (s && (i < linelow))
				if ((s = strchr(s, '\n')) != NULL) {
					i++;
					s++;
				}
	if ((i < linelow) || !s) {
				d->Write("Line(s) out of range; no buffer listing.\r\n");
				return;
			}
      
			t = s;
	while (s && (i <= linehigh))
				if ((s = strchr(s, '\n')) != NULL) {
					i++;
					total_len++;
					s++;
					temp = *s;
					*s = '\0';
			sprintf(editbuf, "%s%2d: ", editbuf, (i-1));
			strcat(editbuf, t);
					*s = temp;
					t = s;
				}
			if (s && t) {
				temp = *s;
				*s = '\0';
		strcat(editbuf, t);
				*s = temp;
	} else if (t) strcat(editbuf, t);
			
	page_string(d, editbuf, true);
}


void editor_replace(Descriptor *d, string &str, char *arg, bool replace_all, StrLenInt maxlen)
{
	char		*s, 
				*t;
	StrLenInt	s_len, t_len, total_len;
	int			replaced = 0, pos;


	if (!(s = strtok(arg, "'"))) {
		d->Write("Invalid format.\r\n");
		return;
	} else if (!(s = strtok(NULL, "'"))) {
		d->Write("Target string must be enclosed in single quotes.\r\n");
		return;
	} else if (!(t = strtok(NULL, "'"))) {
		d->Write("No replacement string.\r\n");
		return;
	} else if (!(t = strtok(NULL, "'"))) {
		d->Write("Replacement string must be enclosed in single quotes.\r\n");
					return;
				}

	if (str.find(s)  < 0) {
		d->Writef("String '%s' not found.\r\n", s); 
		return;
				}
	
	s_len = strlen(s);
	t_len = strlen(t);
	total_len = (t_len - s_len);

	if (replace_all)
		total_len = (total_len * replaced);

	total_len += str.size();
	replaced = 0;
	pos = 0;

	if (total_len + 3 <= maxlen) {
		while ((pos = str.find(s, pos)) >= 0) {
			str.replace(pos, s_len, t);
			++replaced;
			if (!replace_all)
				break;
			pos += t_len;
				}
		if (replaced) {
			d->Writef("Replaced %d occurance%sof '%s' with '%s'.\r\n", replaced, ((replaced != 1)?"s ":" "), s, t); 
		} else {
			d->Write("ERROR: Replacement string causes buffer overflow, aborted replace.\r\n");
				}
	} else {
		d->Write("Not enough space left in buffer.\r\n");
	}
}


void delete_string(Descriptor *d, string &str, const char *arg)
{
	int linelow, linehigh, poslow, poshigh;
	
	line_range(str, arg, linelow, linehigh);
	
	if (linelow < 1) {
		d->Write("Line numbers must be greater than 0.\r\n");
		return;
	} else if (linehigh < linelow) {
		d->Write("That range is invalid.\r\n");
		return;
	}
	
	line_range_offset(str, linelow, linehigh, poslow, poshigh);

	str.erase(poslow, poshigh - poslow);
}


void edit_string(Descriptor *d, string &str, const char *arg, StrLenInt maxlen)
{
	int linelow, poslow, poshigh;
	char *insert;

	line_range(str, arg, linelow, linelow);

	if (linelow < 1) {
		d->Write("Line numbers must be greater than 0.\r\n");
		return;
			}

	insert = get_buffer(MAX_INPUT_LENGTH);
	half_chop(arg, buf, insert);
	strcat(insert, "\r\n");

	line_range_offset(str, linelow, linelow, poslow, poshigh);

	if ((str.size() + poshigh - poslow + strlen(insert)) > maxlen) {
		release_buffer(insert);
		d->Write("Edit increases size past maximum limit, aborting.\r\n");
		return;
		}
    
	str.erase(poslow, poshigh - poslow);
	str.insert(poslow, insert);

	release_buffer(insert);
}


void insert_string(Descriptor *d, string &str, const char *arg, StrLenInt maxlen)
{
	int linelow, poslow, poshigh;
	char *insert;

	line_range(str, arg, linelow, linelow);

	if (linelow < 1) {
		d->Write("Line numbers must be greater than 0.\r\n");
		return;
		}

	insert = get_buffer(MAX_INPUT_LENGTH);
	half_chop(arg, buf, insert);
	strcat(insert, "\r\n");

	line_range_offset(str, linelow, linelow, poslow, poshigh);

	if ((str.size() + poshigh - poslow + strlen(insert)) > maxlen) {
		release_buffer(insert);
		d->Write("Insert increases size past maximum limit, aborting.\r\n");
		return;
	}

	str.insert(poslow, insert);

	release_buffer(insert);
}


/* **********************************************************************
*  Modification of character skills                                     *
********************************************************************** */

ACMD(do_skillset)
{
	Character *vict;
	char name[MAX_INPUT_LENGTH], buf2[100], buf[MAX_INPUT_LENGTH];
	char help[MAX_STRING_LENGTH];
	int skill, theory, value, i, qend;

	argument = one_argument(argument, name);

	if (!*name) {			/* no arguments. print an informative text */
		ch->Send("Syntax: skillset <name> '<skill>' <value>\r\n");
		return;
	}
	if (!(vict = get_char_vis(ch, name, FIND_CHAR_WORLD))) {
		ch->Send(NOPERSON);
		return;
	}
	skip_spaces(argument);

	/* If there is no chars in argument */
	if (!*argument) {
		ch->Send("Skill name expected.\n\r");
		return;
	}
	if (*argument != '\'') {
		ch->Send("Skill must be enclosed in: ''\n\r");
		return;
	}
	/* Locate the last quote && lowercase the magic words (if any) */

	for (qend = 1; *(argument + qend) && (*(argument + qend) != '\''); qend++)
		*(argument + qend) = LOWER(*(argument + qend));

	if (*(argument + qend) != '\'') {
		ch->Send("Skill must be enclosed in: ''\n\r");
		return;
	}
	strcpy(help, (argument + 1));
	help[qend - 1] = '\0';
	if ((skill = Skill::Number(help)) <= 0) {
		ch->Send("Unrecognized skill.\n\r");
		return;
	}
	argument += qend + 1;		/* skip to next parameter */
	argument = two_arguments(argument, buf, buf2);

	if (!*buf) {
		ch->Send("Learned value expected.\n\r");
		return;
	}
	value = MAX(0, MIN(atoi(buf), 250));

	if (*buf2) {
		theory = MAX(0, MIN(atoi(buf2), 250));
	} else {
		theory = value;
	}
	
	mudlogf(BRF, ch, TRUE, "%s changed %s's %s to %d skill, %d theory.", 
		ch->RealName(), vict->RealName(), Skill::Name(skill), value, theory);

	CHAR_SKILL(vict).SetSkill(skill, NULL, value, theory);

	ch->Sendf("You change %s's %s to %d skill, %d theory.\n\r", GET_NAME(vict),
				Skill::Name(skill), value, theory);
}



/*********************************************************************
* New Pagination Code
* Michael Buselli submitted the following code for an enhanced pager
* for CircleMUD.  All functions below are his.  --JE 8 Mar 96
*********************************************************************/

#define PAGE_WIDTH      80

/*
 * Traverse down the string until the begining of the next page has been
 * reached.  Return NULL if this is the last page of the string.
 */
const char *next_page(const char *str, UInt16 pagelength) {
	SInt32	col = 1,
			line = 1;
	bool	spec_code = false;

	for (;; str++) {
		//	If end of string, return NULL
		if (*str == '\0')				return NULL;
		//	If we're at the start of the next page, return this fact.
		else if ((!pagelength) || (line > pagelength)) return str;
		//	Check for the begining of an ANSI color code block.
		else if (*str == '\x1B' && !spec_code)
			spec_code = true;

		// Check for the beginning of an embedded character code
		else if (!spec_code && (*str=='&') && !((*(str+1))=='&'))
			spec_code = true;

		//	Check for the end of an ANSI color code block.
		else if (*str == 'm' && spec_code)
			spec_code = false;

		// Check for the end of a color code
		else if (spec_code && ((*(str-2)) == '&' ))
			spec_code = false;

		//	Check for everything else.
		else if (!spec_code) {
			//	Carriage return puts us in column one.
			if (*str == '\r')	col = 1;
			//	Newline puts us on the next line.
			else if (*str == '\n')	++line;
			//	We need to check here and see if we are over the page width,
			//	and if so, compensate by going to the begining of the next line.
			else if (col++ > PAGE_WIDTH) {
				col = 1;
				++line;
			}
		}
	}
}


//	Function that returns the number of pages in the string.
int count_pages(const char *str, UInt16 pagelength) {
	int pages;

	if (!pagelength)
		return 1;
	for (pages = 1; (str = next_page(str, pagelength)); pages++);
	return pages;
}


//	This function assigns all the pointers for showstr_vector for the
//	page_string function, after showstr_vector has been allocated and
//	showstr_count set.
void paginate_string(const char *str, Descriptor *d) {
	int i;

	if (d->showstr_count)	*(d->showstr_vector) = const_cast<char *>(str);

	for (i = 1; i < d->showstr_count && str; i++)
		str = d->showstr_vector[i] = const_cast<char *>(next_page(str, GET_PAGE_LENGTH(d->character)));

	d->showstr_page = 0;
}


//	The call that gets the paging ball rolling...
void page_string(Descriptor *d, const char *str, bool keep_internal) {
	if (!d)	return;

	if (!str || !*str) {
/*		d->Write(""); - ?? */
		return;
	}
	d->showstr_count = count_pages(str, GET_PAGE_LENGTH(d->character));
	CREATE(d->showstr_vector, char *, d->showstr_count);

	if (keep_internal) {
		d->showstr_head = str_dup(str);
		paginate_string(d->showstr_head, d);
	} else
		paginate_string(str, d);

	show_string(d, "");
}


//	The call that displays the next page.
void show_string(Descriptor *d, char *input) {
	char *buf = get_buffer(MAX_STRING_LENGTH);
	int diff;

	any_one_arg(input, buf);


	if (LOWER(*buf) == 'q') {		//	Q = quit
		FREE(d->showstr_vector);
		d->showstr_count = 0;
		if (d->showstr_head) {
			FREE(d->showstr_head);
			d->showstr_head = NULL;
		}
		release_buffer(buf);
		return;
	} else if (LOWER(*buf) == 'r')	//	R = refresh, back up one page
		d->showstr_page = MAX(0, d->showstr_page - 1);
	else if (LOWER(*buf) == 'b')	//	B = back, back up two pages
		d->showstr_page = MAX(0, d->showstr_page - 2);
	else if (isdigit(*buf))			//	Feature to 'goto' a page
		d->showstr_page = RANGE(0, atoi(buf) - 1, d->showstr_count - 1);
	else if (*buf) {
		d->Write("Valid commands while paging are RETURN, Q, R, B, or a numeric value.\r\n");
		release_buffer(buf);
		return;
	}
	
	//	If we're displaying the last page, just send it to the character, and
	//	then free up the space we used.
	if (d->showstr_page + 1 >= d->showstr_count) {
		d->Write(d->showstr_vector[d->showstr_page]);
		FREE(d->showstr_vector);
		d->showstr_count = 0;
		if (d->showstr_head) {
			free(d->showstr_head);
			d->showstr_head = NULL;
		}
	} else { /* Or if we have more to show.... */
		diff = (int) d->showstr_vector[d->showstr_page + 1];
		diff -= (int) d->showstr_vector[d->showstr_page];
		strncpy(buf, d->showstr_vector[d->showstr_page], diff);
		buf[diff] = '\0';
		d->Write(buf);
		d->showstr_page++;
	}
	release_buffer(buf);
}


int replace_str(char **edstring, const char *pattern, const char *replacement, int rep_all,
		unsigned int max_size)
{
	string tempstring;
	int pos, replaced = 0;
	const int patternlen = strlen(pattern);
	const int replacelen = strlen(replacement);
	unsigned int begin = 0, end = 0;
	const char *tmpptr = *edstring;

	tempstring = tmpptr;

	pos = tempstring.find(pattern, end);

	while (pos != -1) {
		begin = pos;
		end = begin + patternlen;

		tempstring.replace(begin, patternlen, replacement);

		if (rep_all)	pos = tempstring.find(pattern, begin + replacelen);
		else			pos = -1;

		++replaced;
	}

	delete *edstring;
    tempstring.resize(max_size - 1);
    *edstring = str_dup(tempstring.c_str());

	return replaced;
}


int string_freq(const char *searchin, const char *searchfor)
{
	int found = 0, len;
	const char *ptr;
	
	ptr = searchin;
	len = strlen(searchfor);
	
	while (strchr(ptr, *searchfor)) {
		if (strncmp(ptr, searchfor, len)) {
			++found;
			ptr += len;
		} else {
			++ptr;
		}
	}

	return found;
}
