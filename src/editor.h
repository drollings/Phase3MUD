

#ifndef __EDITOR_H__
#define __EDITOR_H__


#include "types.h"
#include "olc.h"
#include "descriptors.h"
#include "mud.base.h"
#include "spec.h"
#include "stl.vector.h"
#include "stl.map.h"
#include "stl.llist.h"
#include "skills.h"
#include <string>


class MenuEditor;

extern void GenericMenu(MenuEditor *ed);
extern void GenericParse(MenuEditor *ed, char *arg);


class Editor {
public:
	// This is reserved for universally used selections.
	// We want to save 0+ for oasis_gen_edit.
	enum {
		ConfirmSave = 1000,
		MainMenu,
		TriggersMenu,
//		AttributesMenu,
//		NumericalResponse,
		TriggerAdd,
		TriggerPurge,
		SpecProcMenu
	};

						Editor(Descriptor *desc) : d(desc), mode(MainMenu), save(0), value(0), number(0),
								zoneNum(0), func(0), funcarg(NULL), storage(NULL), prev_editor(NULL) { };
	virtual				~Editor(void) { };
	
	virtual void		SaveInternally(void) = 0;
	static void 		SaveDisk(SInt32 number) { };
	
	virtual void		Menu(void) = 0;
	virtual void		Parse(char *arg) = 0;
	virtual Ptr			Pointer(void) = 0;
	virtual const char * Prompt(void) {	return "";	};

	void				DispSpecProcMenu(Descriptor *d, GameData *thing);
	void 				ParseSpecProcMenu(Descriptor *d, char *argument, GameData *thing);

	void 				DispScriptsMenu(Descriptor *d, Vector<VNum> &triggers);
	void 				ParseScriptsMenu(Descriptor *d, char *arg, Vector<VNum> &triggers);

	enum FinishMode { Structs, All };
	void				Finish(void);
	virtual void		Finish(FinishMode mode) = 0;
	
protected:
	Descriptor 			*d;
	SInt32				mode;
	SInt32				save;
	SInt32				value;

public:
	VNum				number;
	SInt16				zoneNum;

	SPECIAL(*func);					//	specfunc
	char *funcarg;

protected:
	char 				*storage;
	Editor 				*prev_editor;

friend class Editable;
friend class Descriptor;
};

inline void Editor::SaveInternally(void) { }
inline void Editor::Menu(void) { }
inline void Editor::Parse(char *arg) { }
inline void Editor::Finish(FinishMode mode) { }


/* misc editor defines **************************************************/

/* format modes for format_text */
#define FORMAT_INDENT		(1 << 0)


class TextEditor : public Editor
{
public:
						TextEditor(	Descriptor *desc, char **edstring, StrLenInt maxlen) : 
									Editor(desc), ptr(edstring), max_str(maxlen)
								{
									if (edstring && *edstring) {
										str = *edstring;
										desc->Write(*edstring);
									}
								};
	virtual				~TextEditor(void) { };

	virtual void		SaveInternally(void);
	virtual void		Menu(void);
	virtual void		Parse(char *arg);
	virtual Ptr			Pointer(void) {		return (Ptr) ptr;	}
	virtual const char * Prompt(void);
	
	virtual void		Finish(FinishMode mode);

protected:
	//	Online String Editor
	std::string				str;
	char **				ptr;					//	& to store * of string being edited
	StrLenInt			max_str;				//	Max length of string being edited
};


class SStringEditor : public TextEditor
{
public:
						SStringEditor(Descriptor *desc, SString **edstring, StrLenInt maxlen);
	virtual				~SStringEditor(void) { };

	virtual void		SaveInternally(void);
	Ptr					Pointer(void) {		return (Ptr) ptr;	}
	
private:
	//	Online String Editor
	SString				**ptr;
};


class FileEditor : public TextEditor
{
public:
						FileEditor(	Descriptor *desc, char **edstring, StrLenInt maxlen, const char *file) :
										TextEditor(desc, NULL, maxlen),
										filename(NULL)
									{ 	if (file)	
											filename = strdup(file);
										if (edstring && *edstring) {
											str = *edstring;
											desc->Write(*edstring);
										}
									};
	virtual				~FileEditor(void) {	if (filename)	delete filename;	};

	virtual void		SaveInternally(void);
	
private:
	char 				*filename;
};


class MailEditor : public TextEditor
{
public:
						MailEditor(	Descriptor *desc, char **edstring, StrLenInt maxlen, 
									SInt32 mailto = 0, char *imcmailto = NULL) : 
									TextEditor(desc, NULL, maxlen), mail_to(mailto), 
									imc_mail_to(NULL)
									{
										if (imcmailto)
											imc_mail_to = strdup(imcmailto);
										if (edstring && *edstring) {
											str = *edstring;
											desc->Write(*edstring);
										}
									};
	virtual				~MailEditor(void)	{	if (imc_mail_to)	delete imc_mail_to;	};

	virtual void		SaveInternally(void);
	
private:
	SInt32				mail_to;				//	ID for Mail system
	char *				imc_mail_to;			//	Name(s) for IMC Mail system
};


class BoardEditor : public TextEditor
{
public:
						BoardEditor(	Descriptor *desc, char **edstring, StrLenInt maxlen, VNum boardnum) :
									TextEditor(desc, edstring, maxlen), board(boardnum)
									{
										if (edstring && *edstring) {
											str = *edstring;
											desc->Write(*edstring);
										}
									};
	virtual				~BoardEditor(void)	{ };

	virtual void		SaveInternally(void);
	
private:
	VNum				board;				//	ID for Mail system
};


class CmdlistEditor : public TextEditor
{
public:
						CmdlistEditor(Descriptor *desc, Cmdlist **edit, StrLenInt maxlen);
	virtual				~CmdlistEditor(void)	{ };

	virtual void		SaveInternally(void);
	Ptr					Pointer(void) {		return (Ptr) ptr;	}

private:
	Cmdlist				**ptr;
};


class MenuEditor : public Editor
{
public:
						MenuEditor(Descriptor *d, SInt16 targ);
	virtual				~MenuEditor(void);
	
	virtual void		SaveInternally(void) = 0;
	virtual void		Menu(void) = 0;
	virtual void		Parse(char *arg) = 0;
	virtual Ptr			Pointer(void);
	virtual void		Finish(FinishMode mode);

	friend void 		GenericMenu(MenuEditor *ed);
	friend void 		GenericParse(MenuEditor *ed, char *arg);

	virtual Editable	*Editing(void) = 0;
	virtual Editable	*Source(void) = 0;
};


class SkillEditor : public MenuEditor
{
public:
						SkillEditor(Descriptor *d, SInt16 targ, Skill *edited);
	virtual				~SkillEditor(void);
	
	virtual void		Menu(void);
	virtual void		Parse(char *arg);
	virtual void		SaveInternally(void);

	virtual Editable	*Editing(void);
	virtual Editable	*Source(void);

protected:
	Skill				*editing;
	Skill				*ptr;
};


class SkillRefEditor : public Editor
{
public:
					SkillRefEditor(Descriptor *d, Vector<SkillRef> *orig);
					~SkillRefEditor();

	virtual void		SaveInternally(void);
	virtual void		Menu(void);
	virtual void		Parse(char *arg);
	virtual Ptr			Pointer(void);
	virtual void		Finish(FinishMode mode);

	virtual Ptr							GetPointer(int arg);
	virtual const struct olc_fields * Constraints(int arg);

	enum {
		InputSkill = 2000,
        InputPoints,
		DeleteSkill
	};

private:
	Vector<SkillRef> *editing;
};


class PropertyListEditor : public Editor
{
public:
						PropertyListEditor(Descriptor *d, Property::PropertyList *editmap, int min, int max);
	virtual				~PropertyListEditor(void);
						
	virtual void		SaveInternally(void);
	virtual void		Menu(void);
	virtual void		Parse(char *arg);
	virtual Ptr			Pointer(void);
	virtual void		Finish(FinishMode mode);

	enum {
		AddProperty = 2000,
		DeleteProperty
	};

private:
	Property::PropertyList *editing;
	int 				mintype;
	int					maxtype;
};


class IntVectorEditor : public Editor
{
public:
						IntVectorEditor(Descriptor *d, Vector<int> *edit);
	virtual				~IntVectorEditor(void);
						
	virtual void		SaveInternally(void);
	virtual void		Menu(void);
	virtual void		Parse(char *arg);
	virtual Ptr			Pointer(void);
	virtual void		Finish(FinishMode mode);

	enum {
		Add = 2000,
		Delete
	};

private:
	Vector<int> *editing;
};


class AffectVectorEditor : public Editor
{
public:
						AffectVectorEditor(Descriptor *d, Vector<AffectEditable> *edit);
	virtual				~AffectVectorEditor(void);
						
	virtual void		SaveInternally(void);
	virtual void		Menu(void);
	virtual void		Parse(char *arg);
	virtual Ptr			Pointer(void);
	virtual void		Finish(FinishMode mode);

	enum {
		Add = 2000,
		Delete
	};

private:
	Vector<AffectEditable> *editing;
};


class GenericEditor : public MenuEditor
{
public:
						GenericEditor(Descriptor *d, Editable *from);
	virtual				~GenericEditor(void);
						
	virtual void		SaveInternally(void);
	virtual void		Menu(void);
	virtual void		Parse(char *arg);
	virtual Ptr			Pointer(void);
	virtual void		Finish(FinishMode mode);

	virtual Editable	*Editing(void);
	virtual Editable	*Source(void);

private:
	Editable			*editing;
};
#endif

