/**************************************************************************
*  File: scripts.h                                                        *
*  Usage: header file for script structures and contstants, and           *
*         function prototypes for scripts.c                               *
*                                                                         *
*                                                                         *
*  $Author: sfn $
*  $Date: 2001/02/04 15:57:58 $
*  $Revision: 1.10 $
**************************************************************************/


#ifndef __SCRIPTS_H__
#define __SCRIPTS_H__


#include "types.h"
#include "mud.base.h"
#include "index.h"
#include "stl.llist.h"

class Event;
class Room;

enum { 
// I'm retaining this for use in zedit trigger attaches. -DH
	MOB_TRIGGER,
	OBJ_TRIGGER,
	WLD_TRIGGER
};


class ZoneSignal
{
public:
	ZoneSignal(int zn, char *sig, char *arg) : zone(zn), signal(NULL), argument(NULL)
		{	if (sig)		signal = strdup(sig);
			if (arg)		argument = strdup(arg);	}
	~ZoneSignal(void) 
		{	if (signal) 	delete signal;	
			if (argument)	delete argument;	}
	
	int		zone;
	char	*signal;
	char	*argument;
};


extern LList<ZoneSignal *> zoneSignals;


/* mob trigger types */
#define MTRIG_GLOBAL			(1 << 0)		/* check even if zone empty   */
#define MTRIG_RANDOM			(1 << 1)		/* checked randomly           */
#define MTRIG_COMMAND			(1 << 2)		/* character types a command  */
#define MTRIG_SPEECH			(1 << 3)		/* a char says a word/phrase  */
#define MTRIG_ACT				(1 << 4)		/* word or phrase sent to act */
#define MTRIG_DEATH				(1 << 5)		/* character dies             */
#define MTRIG_GREET				(1 << 6)		/* something enters room seen */
#define MTRIG_GREET_ALL			(1 << 7)		/* anything enters room       */
#define MTRIG_ENTRY				(1 << 8)		/* the mob enters a room      */
#define MTRIG_RECEIVE			(1 << 9)		/* character is given obj     */
#define MTRIG_FIGHT				(1 << 10)		/* each pulse while fighting  */
#define MTRIG_HITPRCNT			(1 << 11)		/* fighting and below some hp */
#define MTRIG_BRIBE				(1 << 12)		/* coins are given to mob     */
#define MTRIG_LEAVE				(1 << 13)
#define MTRIG_LEAVE_ALL			(1 << 14)
#define MTRIG_DOOR				(1 << 15)
#define MTRIG_LOAD				(1 << 16)
#define	MTRIG_MEMORY			(1 << 17)	  /*  mob sees someone remembered  */
#define	MTRIG_SIGNAL			(1 << 18)
#define MTRIG_SELFCOMMAND		(1 << 19)		/* character types a command  */

/* obj trigger types */
#define OTRIG_GLOBAL			(1 << 0)		// unused
#define OTRIG_RANDOM			(1 << 1)		//	Checked randomly
#define OTRIG_COMMAND			(1 << 2)		//	Character types a command
#define OTRIG_SIT				(1 << 3)		//	Character tries to sit on the object
#define OTRIG_LEAVE				(1 << 4)		//	Character leaves room
#define OTRIG_GREET				(1 << 5)		//	Character enters room
#define OTRIG_GET				(1 << 6)		//	Character tries to get obj
#define OTRIG_DROP				(1 << 7)		//	Character tries to drop obj
#define OTRIG_GIVE				(1 << 8)		//	Character tries to give obj
#define OTRIG_WEAR				(1 << 9)		//	character tries to wear obj
#define OTRIG_DOOR				(1 << 10)
#define OTRIG_SPEECH			(1 << 11)		//	Speech check
#define OTRIG_CONSUME			(1 << 12)		//	Eat/Drink obj
#define OTRIG_LOAD				(1 << 13)
#define OTRIG_TIMER				(1 << 14)
#define OTRIG_REMOVE			(1 << 15)
#define OTRIG_START				(1 << 16)
#define OTRIG_QUIT				(1 << 17)
#define	OTRIG_SIGNAL			(1 << 18)

/* wld trigger types */
#define WTRIG_GLOBAL			(1 << 0)		// check even if zone empty
#define WTRIG_RANDOM			(1 << 1)		// checked randomly
#define WTRIG_COMMAND			(1 << 2)		// character types a command
#define WTRIG_SPEECH			(1 << 3)		// a char says word/phrase
#define WTRIG_DOOR				(1 << 4)
#define WTRIG_LEAVE				(1 << 5)		// character leaves room
#define WTRIG_ENTER				(1 << 6)		// character enters room
#define WTRIG_DROP				(1 << 7)		// something dropped in room
#define WTRIG_RESET				(1 << 8)
#define	WTRIG_SIGNAL			(1 << 18)


#define TRIG_GLOBAL 		    (1 << 0) 	   /* check even if zone empty   */
#define TRIG_RANDOM 		    (1 << 1) 	   /* checked randomly  		 */
#define TRIG_COMMAND		    (1 << 2) 	   /* character types a command  */
#define	TRIG_SIGNAL				(1 << 18)
#define TRIG_DELETED			(1 << 31)

/* obj command trigger types */
enum {
	OCMD_EQUIP		= (1 << 0),		//	obj must be in char's equip
	OCMD_INVEN		= (1 << 1),		//	obj must be in char's inven
	OCMD_ROOM		= (1 << 2)		//	obj must be in char's room
};


namespace VarSpace {
enum {
	Local,
	Global,
	Shared
};
}	//	namespace VarSpace


namespace Foreach {
enum {
	Objs,
	Mobs,
	Rooms,
	Samezone,
	Sameroom,
	Contents,
	Group,
	Fighting
//	Riding,
};
}	//	namespace Foreach


namespace InterpreterTables {
enum  {
	PC,
	Wizard
};
}	//	namespace InterpreterTables


namespace CommandSwitch {
enum Switch {
	None,
	Comment,
	If,
	ElseIf,
	Else,
	While,
	Foreach,
	Switch,
	End,
	Done,
	Break,
	Case,
	Default,
	Context,
	Delay,
	Eval,
	EvalGlobal,
	EvalShared,
	Extract,
	Global,
	Shared,
	Halt,
	MakeUID,
	Remote,
	Return,
	Set,
	SetGlobal,
	SetShared,
	Unset,
	Wait,
    Attach,
    Detach,
	Pop,
	Pushfront,
	Popfront,
	Pushback,
	Popback,
	Label,
	Jump,
	Signal,
	Command,
	PCCommand
};
}


enum {
	TRIG_NEW,		//	trigger starts from top
	TRIG_RESTART	//	trigger restarting
};


const SInt32 MAX_SCRIPT_DEPTH =	10;		//	maximum depth triggers can recurse into each other


//	One line of the trigger
class CmdlistElement {
public:
	CmdlistElement(void);
	CmdlistElement(const char *string);
	~CmdlistElement(void);
	
	char *	cmd;				/* one line of a trigger */
	CmdlistElement *original;
	CmdlistElement *next;
	
	UInt32	loops;
	SInt16	type, number;
};


class TrigVarData {
public:
				TrigVarData(void);
				~TrigVarData(void);

	char *		name;
	char *		value;
	SInt32		context;
};


#if 0
class TrigVarData {
public:
				TrigVarData(void);
				TrigVarData(char *setname, char *setvalue, SInt32 setcontext = 0);
				virtual ~TrigVarData(void);

	virtual char *		Name(void) { return name; };
	virtual void		SetName(char *string);
	virtual char *		Value(void) { return value };
	virtual void		SetValue(char *string);
	virtual SInt32		Context(void) { return context };
	virtual void		SetContext(int num);

	virtual char *		PopFront(void);
	virtual char *		PopBack(void);
	virtual void		PushFront(char *set);
	virtual void		PushBack(char *set);
	virtual char *		Pop(char *set);

private:
	char *		name;
	char *		value;
	SInt32		context;
};
#endif


class Cmdlist {
public:
					Cmdlist(const char *str = NULL);
					~Cmdlist(void);
	
	static Cmdlist *Share(Cmdlist *s);
	void			Free(void);
	void 			ResetCommandOps(void);
					
	CmdlistElement *command;
	SInt32			count;
};


/* structure for triggers */
class Trigger : public Base, public Editable {
public:
							Trigger(void);
							Trigger(const Trigger &trig);
							~Trigger(void);
	
	void					Extract(void);
	void					Parse(char *input);
		
	SInt8					data_type;			//	type of game_data for trig
	
	SString *				name;				//	name of trigger
	Flags					trigger_type_mob;		//	type of trigger (for bitvector)
	Flags					trigger_type_obj;		//	type of trigger (for bitvector)
	Flags					trigger_type_wld;		//	type of trigger (for bitvector)
//	CmdlistElement *		cmdlist;			//	top of command list
	Cmdlist *				cmdlist;
	CmdlistElement *		curr_state;			//	ptr to current line of trigger
	SInt32					narg;				//	numerical argument
	SString *				arglist;			//	argument list
	UInt32					depth;				//	depth into nest ifs/whiles/etc
	UInt32					loops;
	Event *					event;				//	event to pause the trigger
	LList<TrigVarData *>	variables;			//	list of local vars for trigger

public:
	enum Vars {
		TrigName,
		TrigArgument,
		TrigMobTypes,
		TrigObjTypes,
		TrigWldTypes,
		TrigNarg,
		TrigNargBitv,
		TrigScript,
		MAXVAR
	};
	Ptr	GetPointer(int arg);
	virtual const struct olc_fields * Constraints(int arg);

	static void 			SaveDisk(Object *obj, FILE *fp);

//	static RNum				Real(VNum vnum);
	static bool				Find(VNum vnum);
	static Trigger *		Read(VNum nr);
	static Index<Trigger>	_Index;
};


extern LList<Trigger *>	Triggers;
extern LList<Trigger *>	PurgedTrigs;


/* a complete script (composed of several triggers) */
class Script : public Base {
public:
						Script(Type datatype);
						~Script(void);
	
	void				Extract(void);
	
	Flags				types;				//	bitvector of trigger types
	LList<Trigger *>	triggers;			//	list of triggers
	LList<TrigVarData *>variables;			//	list of global variables
	SInt32				context;
	Type				attached;
};


extern LList<Script *>	PurgedScripts;


/* function prototypes from triggers.c */
extern void random_mtrigger(Character *ch);
extern void random_otrigger(Object *obj);
extern void random_wtrigger(Room *ch);
extern void signal_trigger(Scriptable *go, Scriptable *targ, char *signal, char *argument);

extern int death_mtrigger(Character *ch, Character *actor);


/* function prototypes from scripts.c */
void script_trigger_check(void);
void add_trigger(Script *sc, Trigger *t); //, int loc);

void do_stat_trigger(Character * ch, Trigger *trig);
void do_sstat_room(Character * ch);
void do_sstat_object(Character * ch, Object * j);
void do_sstat_character(Character * ch, Character * k);


/* Macros for scripts */

#define GET_TRIG_NAME(t)		((t)->name)
#define GET_TRIG_VNUM(t)		((t)->Virtual())
#define GET_TRIG_DATA_TYPE(t)	((t)->data_type)
#define GET_TRIG_NARG(t)		((t)->narg)
#define GET_TRIG_ARG(t)			((t)->arglist)
#define GET_TRIG_WAIT(t)		((t)->event)
#define GET_TRIG_DEPTH(t)		((t)->depth)
#define GET_TRIG_LOOPS(t)		((t)->loops)

#define GET_TRIG_TYPE_MOB(t)          ((t)->trigger_type_mob)
#define GET_TRIG_TYPE_OBJ(t)          ((t)->trigger_type_obj)
#define GET_TRIG_TYPE_WLD(t)          ((t)->trigger_type_wld)


#define SCRIPT(o)				((o)->script)

#define SCRIPT_TYPES(s)			((s)->types)
#define TRIGGERS(s)				((s)->triggers)


#define SCRIPT_CHECK(go, type)	(SCRIPT(go) && IS_SET(SCRIPT_TYPES(SCRIPT(go)), type))
#define TRIGGER_CHECK_MOB(t, type)	(IS_SET(GET_TRIG_TYPE_MOB(t), type) && !GET_TRIG_DEPTH(t))
#define TRIGGER_CHECK_OBJ(t, type)	(IS_SET(GET_TRIG_TYPE_OBJ(t), type) && !GET_TRIG_DEPTH(t))
#define TRIGGER_CHECK_WLD(t, type)	(IS_SET(GET_TRIG_TYPE_WLD(t), type) && !GET_TRIG_DEPTH(t))

#define ADD_UID_VAR(trig, go, name, context) { \
		char *buf = get_buffer(MAX_INPUT_LENGTH); \
		sprintf(buf, "%c%d", UID_CHAR, GET_ID(go)); \
		add_var(trig->variables, name, buf, context); \
		release_buffer(buf); \
		}


#define SCMD(name)	void (name)(Scriptable * go, char *argument, int subcmd, VNum trigger, ::Room * room)

struct ScriptCommand {
	enum {
		Mob		= (1 << 0),
		Obj		= (1 << 1),
		Room	= (1 << 2),
		All		= Mob | Obj | Room
	};

	char *	command;
	SCMD(*command_pointer);
//	void	(*command_pointer)	(Scriptable *go, char *argument, int subcmd);
	SInt32	type;
	SInt32	subcmd;
};

extern const struct ScriptCommand scriptCommands[];


#endif	// __SCRIPTS_H__

