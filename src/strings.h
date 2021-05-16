
#ifndef __STRINGS_H__
#define __STRINGS_H__


//	utils.c++
char *	str_dup(const char *source);
int		str_cmp(const char *arg1, const char *arg2);
int		strn_cmp(const char *arg1, const char *arg2, int n);
const char *str_str(const char *cs, const char *ct);
void	sprintbit(UInt32 vektor, const char *names[], char *result);
void	sprinttype(int type, const char *names[], char *result);
int		str_prefix(const char *astr, const char *bstr);
bool	is_substring(const char *sub, const char *string);
char *	one_phrase(char *arg, char *first_arg);
bool	word_check(const char *str, const char *wordlist);
char *	matching_quote(char *p);
char *	matching_paren(char *p);


//	interpreter.c++
int		search_block(const char *arg, const char **list, bool exact);
int		search_chars(const char arg, const char *list);
char *	one_argument(const char *argument, char *first_arg);
char *	one_word(const char *argument, char *first_arg);
char *	any_one_arg(const char *argument, char *first_arg);
char *	any_one_name(const char *argument, char *first_arg);
char *	two_arguments(const char *argument, char *first_arg, char *second_arg);
char *	three_arguments(const char *argument, char *first_arg, char *second_arg, char *third_arg);
bool	fill_word(const char *argument);
void	half_chop(const char *string, char *arg1, char *arg2);
bool	is_abbrev(const char *arg1, const char *arg2);
bool	is_number(const char *str);
void	skip_spaces(const char *&string);
void	skip_spaces(char *&string);
char *	delete_doubledollar(char *string);


#endif

