/***************************************************************************\
[*]    __     __  ___                ___  | LexiMUD is a C++ MUD codebase [*]
[*]   / /  ___\ \/ (_) /\/\  /\ /\  /   \ |        for Sci-Fi MUDs        [*]
[*]  / /  / _ \\  /| |/    \/ / \ \/ /\ / | Copyright(C) 1997-1999        [*]
[*] / /__|  __//  \| / /\/\ \ \_/ / /_//  | All rights reserved           [*]
[*] \____/\___/_/\_\_\/    \/\___/___,'   | Chris Jacobson (FearItself)   [*]
[*]      A Creation of the AvP Team!      | fear@technologist.com         [*]
[-]---------------------------------------+-------------------------------[-]
[*] File : combat.c++                                                     [*]
[*] Usage: Combat Engine                                                  [*]
\***************************************************************************/


#include "types.h"

//	Primary Includes
#include "combat.h"
#include "skills.h"

//	External Data
#include "characters.h"
#include "rooms.h"
#include "objects.h"
#include "affects.h"
#include "constants.h"
#include "descriptors.h"
#include "config.h"

//	Utilities
#include "comm.h"
#include "buffer.h"
#include "utils.h"
#include "interpreter.h"
#include "handler.h"
#include "find.h"
#include "track.h"
#include "db.h"


ACMD(do_flee);
char *fread_action(FILE * fl, int nr);
int ok_damage_shopkeeper(Character * ch, Character * victim);
void forget(Character * ch, Character * victim);
void remember(Character * ch, Character * victim);
void AlterHit(Character *ch, SInt32 amount);

extern Object * grab_shield(Character *ch, Attack *attackmove);
extern void adesc_lines(char *fromstring, char *line1, char *line2 = NULL, 
                        char *line3 = NULL, char *line4 = NULL);
extern int auto_defend(Character *ch);

extern void fight_mtrigger(Character *ch);
extern void hitprcnt_mtrigger(Character *ch);
extern Object *create_money(int amount);

extern void mag_damage(int level, Character * ch, Character * victim, int spellnum);

char *replace_string(const char *str, char *damsingular, char *damplural,
					char *attackadj, char *attackadverb,
					char *defenseadj, char *defenseadverb, char *hitlocation,
					char *attacktype);

void perform_violence(void);

extern int max_npc_corpse_time;
extern int max_pc_corpse_time;



LList<Character *>	CombatList;

namespace CombatMessages {
	LList<MessageList *>	Messages;
}


#define POSTURE_NORMAL		0
#define POSTURE_ALLOUT_DEFENSE	1
#define POSTURE_ALLOUT_OFFENSE	2
#define POSTURE_CAUTIOUS	3
#define POSTURE_OFFENSE		4

#define IS_OFFENSIVE(i) ((combat_moves[(i)].type == COMBAT_OFFENSIVE) || \
			(combat_moves[(i)].type == COMBAT_BOTH))
#define IS_DEFENSIVE(i) ((combat_moves[(i)].type == COMBAT_DEFENSIVE) || \
			(combat_moves[(i)].type == COMBAT_BOTH))

#define GET_COMBAT_MOVE(ch, i) ((ch)->combat_profile.GetMove(i))


// If 0 on the skill and offensive, it is a weapon attack.
// If both offensive and defensive, adding 1 to the message value 
// should give an offensive message.

// Format: move, skillnum, message, basecost, location modifier, type, 
//         command, subcmd

ACMD(do_combat);
ACMD(do_weapon);
ACMD(do_melee);
ACMD(do_feint);
ACMD(do_bladesong);
ACMD(do_riposte);
ACMD(do_shield_bash);
ACMD(do_auto_attack);

const struct combat_profile_moves combat_moves[] = {
  { "none", 0, TYPE_UNDEFINED, 0, 0, TYPE_UNDEFINED, 0, Combat::Undefined }, 
  { "hit",           SKILL_WEAPON, TYPE_UNDEFINED, 0, 0, COMBAT_OFFENSIVE, do_weapon, Combat::Hit }, 
  { "right swing",   SKILL_WEAPON_R, TYPE_UNDEFINED, 0, 0, COMBAT_OFFENSIVE, do_weapon, Combat::SwingRight }, 
  { "right thrust",  SKILL_WEAPON_R, TYPE_UNDEFINED, 0, 0, COMBAT_OFFENSIVE, do_weapon, Combat::ThrustRight },
  { "right parry",   SKILL_WEAPON_R, SKILL_PARRY, 0, 0, COMBAT_DEFENSIVE, NULL, Combat::ParryRight },  
  { "left swing",    SKILL_WEAPON_L, TYPE_UNDEFINED, 0, 0, COMBAT_OFFENSIVE, do_weapon, Combat::SwingLeft }, 
  { "left thrust",   SKILL_WEAPON_L, TYPE_UNDEFINED, 0, 0, COMBAT_OFFENSIVE, do_weapon, Combat::ThrustLeft },
  { "left parry",    SKILL_WEAPON_L, SKILL_PARRY, 0, 0, COMBAT_DEFENSIVE, NULL, Combat::ParryLeft },
  { "third swing",   SKILL_WEAPON_3, TYPE_UNDEFINED, 0, 0, COMBAT_OFFENSIVE, do_weapon, Combat::SwingThird },
  { "third thrust",  SKILL_WEAPON_3, TYPE_UNDEFINED, 0, 0, COMBAT_OFFENSIVE, do_weapon, Combat::ThrustThird },
  { "third parry",   SKILL_WEAPON_3, SKILL_PARRY, 0, 0, COMBAT_DEFENSIVE, NULL, Combat::ParryThird },
  { "fourth swing",  SKILL_WEAPON_4, TYPE_UNDEFINED, 0, 0, COMBAT_OFFENSIVE, do_weapon, Combat::SwingFourth },
  { "fourth thrust", SKILL_WEAPON_4, TYPE_UNDEFINED, 0, 0, COMBAT_OFFENSIVE, do_weapon, Combat::ThrustFourth },
  { "fourth parry",  SKILL_WEAPON_4, SKILL_PARRY, 0, 0, COMBAT_DEFENSIVE, NULL, Combat::ParryFourth },
  { "punch",  SKILL_PUNCH, SKILL_PUNCH, 2, 2, COMBAT_OFFENSIVE, do_combat, Combat::Punch },
  { "kick",   SKILL_KICK, SKILL_KICK, 4, -3, COMBAT_OFFENSIVE, do_combat, Combat::Kick },
  { "bash",   SKILL_BASH, SKILL_BASH, 10, 0, COMBAT_OFFENSIVE, do_melee, Combat::Bash },
  { "circle",  SKILL_CIRCLE, SKILL_CIRCLE, 8, 0, COMBAT_OFFENSIVE, do_circle, Combat::Circle },
  { "headbutt",  SKILL_HEADBUTT, SKILL_HEADBUTT, 8, 4, COMBAT_OFFENSIVE, do_combat, Combat::Headbutt },
  { "flip",   SKILL_FLIP, SKILL_FLIP, 12, 0, COMBAT_OFFENSIVE, do_melee, Combat::Flip },
  { "bite",   SKILL_BITE, SKILL_BITE, 8, 0, COMBAT_OFFENSIVE, do_combat, Combat::Bite },
  { "iron fist",   SKILL_IRON_FIST, SKILL_IRON_FIST, 4, 0, COMBAT_OFFENSIVE, do_combat, Combat::IronFist },
  { "uppercut",  0, SKILL_UPPERCUT, 5, 0, COMBAT_DEFENSIVE, do_combat, Combat::Uppercut },
  { "disarm",  SKILL_DISARM, SKILL_DISARM, 8, 0, COMBAT_OFFENSIVE, do_combat, Combat::Disarm },
  { "karate chop",  SKILL_KARATE, SKILL_PUNCH, 1, 0, COMBAT_OFFENSIVE, do_combat, Combat::KarateChop },
  { "karate kick",  SKILL_KARATE, SKILL_KICK, 3, 0, COMBAT_OFFENSIVE, do_combat, Combat::KarateKick },
  { "bladesong",  SKILL_BLADESONG, SKILL_BLADESONG, 5, 0, COMBAT_OFFENSIVE, do_bladesong, Combat::Bladesong },
  { "feint",  SKILL_FEINT, SKILL_FEINT, 2, 0, COMBAT_OFFENSIVE, do_feint, Combat::Feint },
  { "right riposte",  SKILL_WEAPON_R, SKILL_RIPOSTE, 10, 0, COMBAT_DEFENSIVE, do_riposte, Combat::RiposteRight },
  { "left riposte",  SKILL_WEAPON_L, SKILL_RIPOSTE, 10, 0, COMBAT_DEFENSIVE, do_riposte, Combat::RiposteLeft },
  { "third riposte",  SKILL_WEAPON_3, SKILL_RIPOSTE, 10, 0, COMBAT_DEFENSIVE, do_riposte, Combat::RiposteThird },
  { "fourth riposte",  SKILL_WEAPON_4, SKILL_RIPOSTE, 10, 0, COMBAT_DEFENSIVE, do_riposte, Combat::RiposteFourth },
  { "dodge",  SKILL_DODGE, SKILL_DODGE, 0, 0, COMBAT_DEFENSIVE, NULL, Combat::Dodge },
  { "shield block",  SKILL_SHIELD, SKILL_SHIELD, 1, 0, COMBAT_DEFENSIVE, NULL, Combat::Block },
  { "backstab",  SKILL_BACKSTAB, SKILL_BACKSTAB, 5, 0, COMBAT_OFFENSIVE, NULL, Combat::Backstab },
  { "shield bash",  SKILL_SHIELD, SKILL_BASH, 10, 0, COMBAT_OFFENSIVE, do_melee, Combat::ShieldBash },
  { "auto attack", 0, TYPE_UNDEFINED, 0, 0, COMBAT_OFFENSIVE, do_auto_attack, Combat::Undefined }, 
  { "auto defend", 0, TYPE_UNDEFINED, 0, 0, COMBAT_DEFENSIVE, NULL, Combat::Undefined }, 
  { "\n",     0, TYPE_UNDEFINED, 0, 0, 0, NULL, 0 }
};


const char *combat_postures[] = {
  "normal",
  "evasive",
  "allout",
  "defensive",
  "offensive",
  "\n"
};


struct damtype_message
{
	int upto;
	char * singular;
	char * plural;
	char * progressive;
};

const struct damtype_message damage_type_messages[Damage::NumTypes][10] = {
	{ // DAMTYPE_UNDEFINED
		{	80,	"harm", "harms", "harming" },
		{	-1,	 0, 0 },
		{	-1,	 0, 0 },
		{	-1,	 0, 0 },
		{	-1,	 0, 0 },
		{	-1,	 0, 0 },
		{	-1,	 0, 0 },
		{	-1,	 0, 0 },
		{	-1,	 0, 0 },
		{	-1,	 0, 0 }
	},
	{ // DAMTYPE_BASIC
		{	1,	"barely tap", "barely taps", "barely tapping" },
		{	2,	"thump", "thumps", "thumping" },
		{	4,	"smack", "smacks", "smacking" },
		{	8,	"pound", "pounds", "pounding" },
		{	10,	"bludgeon", "bludgeons", "bludgeoning" },
		{	12,	"pummel", "pummels", "pummelling" },
		{	15,	"SMASH", "SMASHES", "SMASHING" },
		{	30,	"CRUSH", "CRUSHES", "CRUSHING" },
		{	60, "SMITE", "SMITES", "SMITING" },
		{	-1, 0, 0 }
	},
	{ // DAMTYPE_SLASHING
		{	1,	"tickle", "tickles", "tickling" },
		{	2,	"whittle", "whittles", "whittling" },
		{	4,	"cut", "cuts", "cutting" },
		{	8,	"slice", "slices", "slicing" },
		{	12,	"slash", "slashes", "slashing" },
		{	15,	"carve into", "carves into", "carving into" },
		{	30,	"LACERATE",	"LACERATES", "LACERATING" },
		{	60,	"EVISCERATE", "EVISCERATES", "EVISCERATING" },
		{	-1,	0, 0 },
		{	-1,	0, 0 }
	},
	{ // DAMTYPE_PIERCING	
		{	1,	"poke", "pokes", "poking" },
		{	2,	"jab", "jabs", "jabbing" },
		{	4,	"perforate", "perforates", "perforating" },
		{	8,	"stick", "sticks", "sticking" },
		{	12,	"stab", "stabs", "stabbing" },
		{	15,	"run through", "runs through", "running through" },
		{	30,	"IMPALE", "IMPALES", "IMPALING" },
		{	60,	"CRUCIFY", "CRUCIFIES", "CRUCIFYING" },
		{	-1,	 0, 0 },
		{	-1,	 0, 0 }
	},
	{ // DAMTYPE_FIRE		
		{	1,	"warm", "warms", "warming" },
		{	2,	"singe", "singes", "singing" },
		{	4,	"burn", "burns", "burning" },
		{	8,	"fry", "fries", "frying" },
		{	12,	"sear", "sears", "searing" },
		{	15,	"scorch", "scorches", "scorching" },
		{	30,	"roast", "roasts", "roasting" },
		{	60,	"INCINERATE", "INCINERATES", "INCINERATING" },
		{	80,	"CREMATE", "CREMATES", "CREMATING" },
		{	-1,	 0, 0 }
	},
	{ // DAMTYPE_ELECTRICITY	
		{	10, "zap", "zaps", "zapping" },
		{	25, "jolt", "jolts", "jolting" },
		{	40, "shock", "shocks", "shocking" },
		{	50, "ELECTROCUTE", "ELECTROCUTES", "ELECTROCUTING" },
		{	-1, 0, 0 },
		{	-1, 0, 0 },
		{	-1, 0, 0 },
		{	-1, 0, 0 },
		{	-1, 0, 0 },
		{	-1, 0, 0 }
	},
	{ // DAMTYPE_ACID		
		{	10, "burn", "burns", "burning" },
		{	30, "melt", "melts", "melting" },
		{	50, "LIQUIFY", "LIQUIFIES", "LIQUIFYING" },
		{	80, "DISSOLVE", "DISSOLVES", "DISSOLVING" },
		{ -1, 0, 0 },
		{ -1, 0, 0 },
		{ -1, 0, 0 },
		{ -1, 0, 0 },
		{ -1, 0, 0 },
		{ -1, 0, 0 }
	},
	{ // DAMTYPE_FREEZING	
		{	10, "chill", "chills", "chilling" },
		{	25, "freeze", "freezes", "freezing" },
		{	50, "CRYONIZE", "CRYONIZES", "CRYONIZING" },
		{ -1, 0, 0 },
		{ -1, 0, 0 },
		{ -1, 0, 0 },
		{ -1, 0, 0 },
		{ -1, 0, 0 },
		{ -1, 0, 0 },
		{ -1, 0, 0 }
	},
	{ // DAMTYPE_POISON		
		{  2, "poison", "poisons", "poisoning" },
		{ -1, 0, 0 },
		{ -1, 0, 0 },
		{ -1, 0, 0 },
		{ -1, 0, 0 },
		{ -1, 0, 0 },
		{ -1, 0, 0 },
		{ -1, 0, 0 },
		{ -1, 0, 0 },
		{ -1, 0, 0 }
	},
	{ // DAMTYPE_PSIONIC		
		{  2, "blast", "blasts", "blasting" },
		{ -1, 0, 0 },
		{ -1, 0, 0 },
		{ -1, 0, 0 },
		{ -1, 0, 0 },
		{ -1, 0, 0 },
		{ -1, 0, 0 },
		{ -1, 0, 0 },
		{ -1, 0, 0 },
		{ -1, 0, 0 }
	},
	{ // DAMTYPE_VIM		
		{ 20, "blast", "blasts", "blasting" },
		{ 40, "ERADICATE", "ERADICATES", "ERADICATING" },
		{ 60, "DESTROY", "DESTROYS", "DESTROYING" },
		{ 80, "ANNIHILATE", "ANNIHILATES", "ANNIHILATING" },
		{ -1, 0, 0 },
		{ -1, 0, 0 },
		{ -1, 0, 0 },
		{ -1, 0, 0 },
		{ -1, 0, 0 },
		{ -1, 0, 0 }
	},
	{ // DAMTYPE_EXPLOSION
		{ 20, "blast", "blasts", "blasting" },
		{ 40, "ERADICATE", "ERADICATES", "ERADICATING" },
		{ 60, "DESTROY", "DESTROYS", "DESTROYING" },
		{ 80, "ANNIHILATE", "ANNIHILATES", "ANNIHILATING" },
		{ -1, 0, 0 },
		{ -1, 0, 0 },
		{ -1, 0, 0 },
		{ -1, 0, 0 },
		{ -1, 0, 0 },
		{ -1, 0, 0 }
	},
	{ // DAMTYPE_FALL
		{ 20, "blast", "blasts", "blasting" },
		{ 40, "ERADICATE", "ERADICATES", "ERADICATING" },
		{ 60, "DESTROY", "DESTROYS", "DESTROYING" },
		{ 80, "ANNIHILATE", "ANNIHILATES", "ANNIHILATING" },
		{ -1, 0, 0 },
		{ -1, 0, 0 },
		{ -1, 0, 0 },
		{ -1, 0, 0 },
		{ -1, 0, 0 },
		{ -1, 0, 0 }
	},
	{ // DAMTYPE_SUFFERING
		{	2, "drain", "drains", "draining" },
		{ -1, 0, 0 },
		{ -1, 0, 0 },
		{ -1, 0, 0 },
		{ -1, 0, 0 },
		{ -1, 0, 0 },
		{ -1, 0, 0 },
		{ -1, 0, 0 },
		{ -1, 0, 0 },
		{ -1, 0, 0 }
	}
};


static char *skillrates[] = {
	"pathetic ",		//	Blunder,
	"clumsy ",			//	AFailure,
	"failed ",			//	Failure,
	"", 				//	PSuccess,
	"", 				//	NSuccess,
	"", 				//	Success,
	"skillful "			//	ASuccess
};

static char *skillhow[] = {
	"pathetically ",		//	Blunder,
	"clumsily ",			//	AFailure,
	"futilely ",			//	Failure,
	"", 				//	PSuccess,
	"", 			//	NSuccess,
	"", 		//	Success,
	"skillfully "			//	ASuccess
};


void kill_increase_exp(Character *ch, long gain)
{

	gain = RANGE(0, gain, max_exp_gain);

	if (gain == 0) {
		ch->MergeSend("Easy victory.  You get no experience.\r\n");
		return;
	} 
	else if (gain == 1)		strcpy(buf1, "one measly point");
	else					sprintf(buf1, "%ld points", gain);

	if (AFF_FLAGGED(ch, AffectBit::Group))
		sprintf(buf2, "You receive your share of experience - %s!\r\n", buf1);
	else
		sprintf(buf2, "You gain experience - %s!\r\n", buf1);
	
	*buf1 = '\0';
	ch->Send(buf2);

	ch->GainExp(gain);
}


namespace Damage {
	Chart			charts[NumTypes];

	const char *	types[] = {
		"UNDEFINED",
		"BASIC",
		"SLASHING",
		"PIERCING",
		"FIRE",
		"ELECTRICITY",
		"ACID",
		"FREEZING",
		"POISON",
		"PSIONIC",
		"VIM",
		"EXPLOSION",
		"FALL",
		"MISC",
		"SUFFERING",
		"\n"
	};
}


struct DamMesgType {
	char *			to_room;
	char *			to_char;
	char *			to_victim;
};


/* Weapon attack texts */
const struct attack_hit_type attack_hit_text[] =
{
	{"hit", "hits"},		/* 0 */
	{"sting", "stings"},
	{"whip", "whips"},
	{"slash", "slashes"},
	{"bite", "bites"},
	{"bludgeon", "bludgeons"},	/* 5 */
	{"crush", "crushes"},
	{"pound", "pounds"},
	{"claw", "claws"},
	{"maul", "mauls"},
	{"thrash", "thrashes"},	/* 10 */
	{"pierce", "pierces"},
	{"blast", "blasts"},
	{"stab", "stabs"},
	{"garotte", "garottes"},
	{"arrow", "shoots"},
	{"bolt", "shoots"},
	{"rock", "shoots"}
};


/*
 * const char *weapon_types[] = {
 * 	"PROJECTILE",
 * 	"LAUNCHER",
 * 	"ENERGY",
 * 	"EDGED",
 * 	"CRUSH",
 * 	"POLEARM",
 * 	"THROWN",
 * 	"BOW",
 * 	"\n"
 * };
 */


CombatMessages::Message::~Message(void) {	
}


void CombatMessages::Message::Set(FILE * fl, int i) {
	attacker = fread_action(fl, i);
	victim = fread_action(fl, i);
	room = fread_action(fl, i);
}


//MESS_FILE
void CombatMessages::Load(const char *filename) {
	FILE 			*fl;
	SInt32			type, i = 0;
	MessageType		*msg;
	MessageList 	*list;
	char 			*string, *ptr;
	
	if (!(fl = fopen(filename, "r"))) {
		log("Error reading combat message file %s.", filename);
		exit(1);
	}
	
	string = get_buffer(256);
	
	fgets(string, 128, fl);
	while (!feof(fl) && (*string == '\n' || *string == '*'))
		fgets(string, 128, fl);
	
	while (*string == 'M') {
		i++;							//	Action counter for error messages
		fgets(string, 128, fl);

		if (sscanf(string, " %s\n", buf) == 1) {
		
			// DH's support for bufs and numbers for skill identifiers.
			if (isdigit(*buf)) {
				type = atoi(buf);
			} else {
				// Replace all underscores with spaces
				while ((ptr = strchr(buf, '_')) != NULL)
					*ptr = ' ';
				// Give preference to the slots for attack messages, to prevent cases
				// like "whip" from being assigned to the skill first.
				type = Skill::Number(buf, FIRST_COMBAT_MESSAGE, LAST_COMBAT_MESSAGE);
                if (type <= 0)
					type = Skill::Number(buf);
			}
		} else {
			sprintf(buf3, "Error reading %s from misc/messages.", string);
			fclose(fl);
			log(buf3);
			exit(1);
		}

		msg = new MessageType;
		
		msg->die.Set(fl, i);
		msg->miss.Set(fl, i);
		msg->hit.Set(fl, i);
		msg->staff.Set(fl, i);
		
		if (!(list = CombatMessages::Find(type))) {
			list = new MessageList;
			list->attack = type;
			CombatMessages::Messages.Append(list);
		}
		list->Messages.Append(msg);
		
		fgets(string, 128, fl);
		while (!feof(fl) && (*string == '\n' || *string == '*'))
			fgets(string, 128, fl);
	}
		
	release_buffer(string);
	fclose(fl);
}


CombatMessages::MessageList *CombatMessages::Find(SInt32 attack) {
	MessageList *		list;
	LListIterator<MessageList *>	iter(CombatMessages::Messages);
	
	while ((list = iter.Next()))
		if (list->attack == attack)
			return list;
	
	return NULL;
}


void Combat::Load(void) {
	char *	buffer = get_buffer(MAX_INPUT_LENGTH);
	char *	element = get_buffer(MAX_INPUT_LENGTH);
	char *	filename = get_buffer(MAX_INPUT_LENGTH);
	char *	bufPtr;
	FILE *	file;
	int		damageLine;
	int		damage;
	char	critical;
	
	for (int attack = 0; attack < Damage::NumTypes; attack++) {
		Damage::Chart &	chart = Damage::charts[attack];
		sprintf(filename, "%s%s.dmg", DMG_PREFIX, Damage::types[attack]);

		file = fopen(filename, "r");
		if (!file)	continue;
		
		damageLine = 0;
		
		fgets(buffer, 255, file);
		while (!feof(file) && damageLine < 50) {
			bufPtr = buffer;
			skip_spaces(bufPtr);
			
			if (*bufPtr && *bufPtr != '#') {
				for (int column = 0; column < 20; column++) {
					bufPtr = one_argument(bufPtr, element);
					if (!*element || *element == '#')	break;
					
					sscanf(element, "%d%c", &damage, &critical);
					
					Damage::Result &	result = chart[column][damageLine];
					
					result.damage = damage;
					result.critical = critical;
				}
				damageLine++;
			}
			fgets(buffer, 255, file);
		}
		
		fclose(file);
	}
	
	release_buffer(buffer);
	release_buffer(element);
	release_buffer(filename);
}


// A piece of code split off from damage() for use in the new Hit().
void Combat::CheckFighting(Character *ch, Character *victim) {
	/* Start the attacker fighting the victim */
	if ((GET_POS(ch) > POS_STUNNED) && (FIGHTING(ch) == NULL))
		ch->Fight(victim);

	/* Start the victim fighting the attacker */
	if ((GET_POS(victim) > POS_STUNNED) && (FIGHTING(victim) == NULL)) {
		victim->Fight(ch);
		if (MOB_FLAGGED(victim, MOB_MEMORY) && !IS_NPC(ch) && !IS_IMMORTAL(ch))
			remember(victim, ch);
	}
}


void Combat::Check(Character *ch, Character *victim) {
	if (pk_allowed || (ch == victim) || NO_STAFF_HASSLE(ch))
		return;
	
	if (ch->GetRelation(victim) == RELATION_ENEMY)
		return;
	
	if (IS_NPC(ch)) {
		SET_BIT(MOB_FLAGS(ch), MOB_KILLER);
	} else {
		SET_BIT(PLR_FLAGS(ch), PLR_KILLER);
		ch->Send("If you want to be a murderer, so be it...\r\n");
		log("PC Killer bit set on %s for initiating attack on %s at %s.", 
			ch->Name(), victim->Name(), world[IN_ROOM(victim)].Name());
	}
}


bool Combat::Valid(Character *ch, Character *victim) {
	if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
		ch->Send("This room just has such a peaceful, easy feeling...\r\n");
		return false;
	}
	if (victim && !ok_damage_shopkeeper(ch, victim))
		return false;

	return true;
}


//	Start attacking
void Character::Fight(Character * victim) {
	SInt8 offense, defense;

	if ((this == victim) || PURGED(this))
		return;

	/* Peaceful room check added as a safeguard */
	if (ROOM_FLAGGED(IN_ROOM(this), ROOM_PEACEFUL))
		return;

	if (FIGHTING(this)) {
		core_dump();
		return;
	}

	CombatList.Add(this);
	
	if (AFF_FLAGGED(this, AffectBit::Sleep))
		Affect::FromThing(this, SPELL_SLEEP);

	if (AFF_FLAGGED(this, AffectBit::Invisible | AffectBit::Hide | AffectBit::Camoflauge))
		Appear();

	if (Affect::AffectedBy(this, SKILL_SNEAK)) {
		Affect::FromThing(this, SKILL_SNEAK);
	}

	if (Affect::AffectedBy(this, SKILL_HIDE)) {
		Affect::FromThing(this, SKILL_HIDE);
	}

	if (Affect::AffectedBy(this, SKILL_CAMOFLAUGE)) {
		Affect::FromThing(this, SKILL_CAMOFLAUGE);
	}

	combat_profile.ZeroPoints();	// Zero out the combat points.

	if (AFF_FLAGGED(this, AffectBit::Sneak) && (!FIGHTING(victim)) &&
		 SKILLCONTEST(this, STAT_INT, 0, victim, STAT_INT, -4, offense, defense)) {
		combat_profile.PointUpdate(this);
		act("You surprised the hell out of $M!", 0, this, 0, victim, TO_CHAR);
		act("$n leaps out at you out of nowhere!", 0, this, 0, victim, TO_VICT);
		act("$n leaps out at $N, surprising $M!", FALSE, this, 0, victim, TO_NOTVICT);
	} else {
		combat_profile.PointUpdate(this);
		victim->combat_profile.PointUpdate(victim);
	}

	FIGHTING(this) = victim;
	if (SittingOn())					SetMount(NULL);

	if ((GET_POS(this) == POS_SLEEPING) && (GET_MOVE(this) > 0)) {
    	GET_POS(this) = POS_RESTING;	/* wake 'em up */
	// else 								GET_POS(this) = POS_FIGHTING;
	}

	/*  if (!pk_allowed)*/
	Combat::Check(this, victim);
	
/*
 * 	Combat::CombatEvent *	CEvent = new Combat::CombatEvent;
 * 	CEvent->attackerId = GET_ID(this);
 * 	events.Add(new Event(Combat::AttackEvent, CEvent, Combat::Delay(this) RL_SEC, NULL));
 */
}


//	Stop attacking
void Character::StopFighting(void) {
	CombatList.Remove(this);
	FIGHTING(this) = NULL;
	GET_POS(this) = POS_STANDING;
	UpdatePos();

	combat_profile.ZeroPoints();	// Zero out the combat points.
	general.damage_taken = 0;
}


void Character::StopFoesFighting(void) 
{
	Character *tch;
	START_ITER(iter, tch, Character *, CombatList) {
		if (FIGHTING(tch) == this)
			tch->StopFighting();
	}
}


//	Returns:
//	<0 = Victim died
//	 0 = No damage
//	>0 = Amount of damage
SInt32 Character::Damage(Character *attacker, Attack *attack = NULL) {
	Character *tch;
	VNum	room;
	bool	stopfighting = false,
			missile = false;
	Attack	temp_attack;
	
	if (material == Materials::Ether) {
		if (!attack)	return 0;
		switch (attack->damtype) {
		case Damage::Psionic:	break;
		default:				return 0;
		}
	}

	if (!attack) {
		attack = &temp_attack;
		attack->succeed_by = 1;	// Such a case assumes success.
	}

	if (GET_POS(this) <= POS_DEAD) {
		log("SYSERR: Attempt to damage a corpse: %s->Damage(ch = %s, ...) in room #%d", RealName(),
				attacker ? attacker->RealName() : "<nobody>", world[IN_ROOM(this)].Virtual());
		Die(NULL);
		return 0;
	}
	
	// BUGFIX:	Possibility of damage spells on a mob that was already dead
	// creates the conditions for a segfault.	We now check for NOWHERE. -DH
	if ((IN_ROOM(this) <= NOWHERE) || (attacker && IN_ROOM(attacker) <= NOWHERE))
		return 0;

	attack->dam = MIN(attack->dam, 100);

	/* Cut attack->damage in half if victim has sanct, to a minimum 1 */
	if ((AFF_FLAGGED(this, AffectBit::Sanctuary)) && attack->dam >= 2)
		attack->dam /= 2;

	if (NO_STAFF_HASSLE(this))
		attack->dam = 0;
	
	if (attacker && (this != attacker)) {

		if (!Combat::Valid(attacker, this))
			stopfighting = true;
		
		/* peaceful rooms - but let imps attack */
		if (ROOM_FLAGGED(IN_ROOM(attacker), ROOM_PEACEFUL) && GET_TRUST(attacker) < TRUST_IMPL) {
			attacker->Send("This room just has such a peaceful, easy feeling...\r\n");
			stopfighting = true;
		}

		/* You can't attack->dam an immortal! */
		if (!IS_NPC(this) && IS_IMMORTAL(this))
			attack->dam = 0;

		if (this == RIDING(attacker))
			this->RemoveRider(attacker, MNT_COMBAT);

		if (!ok_damage_shopkeeper(attacker, this))
			stopfighting = true;
		
		if (stopfighting) {
			if (FIGHTING(attacker) == this)
				attacker->StopFighting();
			if (FIGHTING(this) == attacker)
				StopFighting();
		}

		if (IN_ROOM(this) != IN_ROOM(attacker))
			missile = true;
		else if (GROUP_POS(attacker) && (!in_melee_range(attacker, this, attack->message)))
			missile = true;
		else
			Combat::CheckFighting(this, attacker);
		
		if (master == attacker)	StopFollower();

		if ((attack->message < Damage::Suffering) && 
			AFF_FLAGGED(attacker, AffectBit::Invisible | AffectBit::Hide | AffectBit::Camoflauge))
			attacker->Appear();

		if (!pk_allowed)	Combat::Check(attacker, this);

		// Bonus for a victim in a prone position
		attack->dam += MAX(POS_SITTING - GET_POS(this), 0) * 2;

		if ((AFF_FLAGGED(attacker, AffectBit::VampRegen)) && (!IS_GOOD(attacker)) && attack->dam && 
			(!AFF_FLAGGED(attacker, AffectBit::ProtectEvil)))
			AlterHit(attacker, (MAX(1, MIN((GET_HIT(this) + 10) / 8, attack->dam / 8))) * -1);

	}

	// Adjust attack->dam for attack->dam type
	switch (attack->damtype) {
	case Damage::Slashing: attack->dam += (int) (attack->dam / 2); break;
	case Damage::Piercing: attack->dam *= 2; break;
	}

	if ((attack->dam > 0) && Affect::AffectedBy(this, SPELL_FREEZE_FOE)) {
		Affect::FromThing(this, SPELL_FREEZE_FOE);
		Send("The ice encasing you is shattered!\n");
		act("The ice encasing $n is shattered!", TRUE, this, 0, 0, TO_ROOM);
	}

    if (AFF_FLAGGED(this, AffectBit::MageShield)) {
		if (attack->dam >= GET_MANA(this)) {
			attack->dam -= GET_MANA(this);
			AlterMana(this, GET_MANA(this));
			Affect::FromThing(this, SPELL_MAGE_SHIELD);
			REMOVE_BIT(AFF_FLAGS(this), AffectBit::MageShield);
		} else {
			AlterMana(this, attack->dam);
			attack->dam = 0;
		}
	} 
	
	if (attack->dam > 0) {
		if ((GET_HIT(this) > 0) && (attack->dam > GET_HIT(this)) && (attack->succeed_by < 10)) {
			AlterHit(this, GET_HIT(this) + 5);
			general.damage_taken += GET_HIT(this) + 5;
		} else {
			AlterHit(this, attack->dam);
			general.damage_taken += attack->dam;
		}
		UpdatePos();
	}

	if (attacker && attack->message > 0) {
		if (attack->dam > 0) {
			attacker->Send("&cG");
			Send("&cR");
		} else {
			attacker->Send("&cY");
			Send("&cB");
		}

		if (!IS_WEAPON(attack->message) || missile) {
			Combat::SkillMessage(attack->dam, attacker, this, attack->message, attack);
		} else {
			if (GET_POS(this) == POS_DEAD || attack->dam == 0) {
				if (!Combat::SkillMessage(attack->dam, attacker, this, attack->message, attack))
					Combat::AttackMessage(attacker, this, attack);
			} else {
				Combat::AttackMessage(attacker, this, attack);
			}
		}

		attacker->Send("&c0");
		Send("&c0");
	}
	
	/* make checks to stay mounted */
	if (RIDING(this) && !riding_check(this, RIDING(this), MNT_COMBAT))
		RIDING(this)->RemoveRider(this, MNT_COMBAT);

	if (riders.Count()) {
		START_ITER(iter, tch, Character *, riders) {
			if (!riding_check(tch, this, MNT_COMBAT))
				RemoveRider(tch, MNT_COMBAT);
		}
	}

	// Remove actions
	if (GET_ACTION(this)) {
		GET_ACTION(this)->Cancel();
		Send("You couldn't focus on what you were doing!\r\n");
	}
	
	switch (GET_POS(this)) {
		case POS_MORTALLYW:
			act("$n is mortally wounded, and will die soon, if not aided.", TRUE, this, 0, 0, TO_ROOM);
			Send("You are mortally wounded, and will die soon, if not aided.\r\n");
			break;
		case POS_INCAP:
			act("$n is incapacitated and will slowly die, if not aided.", TRUE, this, 0, 0, TO_ROOM);
			Send("You are incapacitated and will slowly die, if not aided.\r\n");
			break;
		case POS_STUNNED:
			act("$n is stunned, but will probably regain consciousness again.", TRUE, this, 0, 0, TO_ROOM);
			Send("You're stunned, but will probably regain consciousness again.\r\n");
			break;
		case POS_DEAD:	//	Use Character::Send -- act() doesn't send attack->message if you are DEAD.
			act("$n is dead!  R.I.P.", FALSE, this, 0, 0, TO_ROOM);
			Send("You are dead!  Sorry...\r\n");
			break;

		default:			/* >= POSITION SLEEPING */
			if (attack->dam > (GET_MAX_HIT(this) / 4))
				act("That really did HURT!", FALSE, this, 0, 0, TO_CHAR);

			if (GET_HIT(this) < (GET_MAX_HIT(this) / 4)) {
				Send("&cRYou wish that you would stop BLEEDING so much!&c0\r\n");
				if (attacker && MOB_FLAGGED(this, MOB_WIMPY) &&
						!MOB_FLAGGED(this, MOB_SENTINEL) && (attacker != this))
					do_flee(this, "", 0, "flee", 0);
			}
			if (attacker && !IS_NPC(this) && !AFF_FLAGGED(this, AffectBit::Berserk) &&
					GET_WIMP_LEV(this) && (this != attacker) &&
					GET_HIT(this) < GET_WIMP_LEV(this)) {
				Send("You wimp out, and attempt to flee!\r\n");
				do_flee(this, "", 0, "flee", 0);
			}
			break;
	}
	
// 	if ((attack->dam >= 150) && (MOB_FLAGGED(this, MOB_ACIDBLOOD) || (!IS_NPC(this) && IS_ALIEN(this))))
// 		Combat::Acidspray(this, attack->dam);

	if (attacker && !IS_NPC(this) && !(desc)) {
		do_flee(this, "", 0, "flee", 0);
	}

	if (GET_POS(this) <= POS_STUNNED) {
		if (IS_NPC(this) || desc) {
			if (attacker && (attacker->GetRelation(this) != RELATION_FRIEND)) {
				attacker->GainExp(Combat::CalcReward(attacker, this));
			}
		}
		StopFighting();

		START_ITER(iter, tch, Character *, world[IN_ROOM(this)].people) {
			if (FIGHTING(tch) != this)		continue;

			// DH's new code to halt combat if one of the combatants is a PC and one goes down.
				tch->StopFighting();

// 			if (GET_POS(this) == POS_DEAD) {
// 				tch->StopFighting();
// 			} else if ((!IS_NPC(this)) &&
// 				(IS_NPC(tch) ? MOB_FLAGGED(tch, MOB_MERCIFUL) : PRF_FLAGGED(tch, Preference::Merciful))) {
// 				tch->StopFighting();
// 			}
		}
	}

	/* Uh oh.  Victim died. */
	if (GET_POS(this) == POS_DEAD) {
		if (attacker && (attacker != this)) {
			if (MOB_FLAGGED(attacker, MOB_MEMORY))
				forget(attacker, this);
		}
		room = IN_ROOM(this);
		Die(attacker);
		return -1;
	}
	return attack->dam;
}


SInt32 Character::Critical(Character *attacker, char critical, SInt32 type) {
	int		amount = 0;
	Attack	attack;
	attack.dam = amount;
	return Damage(attacker, &attack);
}


//	Returns:
//	<0 = Victim died
//	 0 = No damage
//	>0 = Amount of damage
SInt32 Character::Hit(Character *victim, Attack *attack, SInt32 range) {
	SInt8		passive_defense = 0,	// Chance of armor deflecting attacks.
				damage_resistance = 0;	// How much damage is blocked by armor.
	bool		created_attack = false;	// If true, delete attackmove at end.
	Attack 		defense;
	Character	*tempfighting = NULL;
	Object 		*cweapon = NULL, 
				*vweapon = NULL, 
				*armor = NULL;
	SInt32		retval = 0, attackweight;

	if (PURGED(victim) || PURGED(this))
		return 0;

	if (AFF_FLAGGED(this, AffectBit::Immobilized))
		return 0;

	if (range <= 0 && (IN_ROOM(this) != IN_ROOM(victim)))
		return 0;

	if (attack == NULL) {
		created_attack = true;
		attack = new Attack;
		attack->dam = str_dam(GET_STR(this), attack->move) + GET_DAMROLL(this);
		attack->move = Combat::Hit;
		attack->message = GET_ATTACKTYPE(this) + TYPE_HIT;
	}

	cweapon = attack->weapon;

	if (attack->damtype != Damage::Psionic) {
		if (material == Materials::Ether || (cweapon && cweapon->material == Materials::Ether)) {
			act("Your attack passes through $N harmlessly.", false, this, NULL, victim, TO_CHAR);
			act("$n's attack passes through you harmlessly.", true, this, NULL, victim, TO_VICT | TO_SLEEP);
			act("$n strikes at $N, but $s insubstantial attack passes through harmlessly.", true, this, NULL, victim, TO_NOTVICT);	
			attack->succeed_by = -9;
			return 0;
		} else if (victim->material == Materials::Ether) {
			act("Your attack passes through $N harmlessly.", false, this, NULL, victim, TO_CHAR);
			act("$n's attack passes through you harmlessly.", true, this, NULL, victim, TO_VICT | TO_SLEEP);
			act("$n strikes at $N, sweeping through $S insubstantial form harmlessly.", true, this, NULL, victim, TO_NOTVICT);	
			attack->succeed_by = -9;
			return 0;
		}
	}

	if (ROOM_FLAGGED(IN_ROOM(this), ROOM_PEACEFUL) &&
			(attack->damtype < Damage::Suffering) && !NO_STAFF_HASSLE(this)) {
		Send("This room just has such a peaceful, easy feeling...\r\n");
		if (FIGHTING(this) == victim)
			StopFighting();
		return 0;
	}
	
	if (FIGHTING(this) && IN_ROOM(this) != IN_ROOM(victim)) {
		StopFighting();
		return 0;
	}
	
	if (!Combat::Valid(this, victim))
		return 0;

	if (GROUP_POS(this) && (!in_melee_range(this, victim, attack->skillnum))) {
		act("&c+&cWYou cannot attack them without moving to the front!&c-", 0, this, 0, 0, TO_CHAR);
		return 0;
	}

	victim = check_for_intercept(this, victim, attack);

	Combat::CheckFighting(this, victim);	// Since we may not use damage in cases such
					// as armor deflecting attacks, we want to
					// ensure that the fight starts.

	combat_profile.CombatCost(this, COMBAT_OFFENSIVE, attack->speed);

	// Ambidexterity check.
	if ((attack->hand > WEAR_HAND_R) && (!ADVANTAGED(this, Advantages::Ambidextrous)))
		attack->mod -= 4;

	// Decide who succeeds.
	attack->mod += GET_HITROLL(this);
	attack->succeed_by = MIN(120, MAX(-120, CHAR_SKILL(this).RollSkill(this, attack->skillnum, attack->chance) + attack->mod));
	CHAR_SKILL(this).RollToLearn(this, attack->skillnum, attack->succeed_by);

	if (GET_POS(victim) <= POS_SLEEPING) {
		attack->succeed_by = MAX(attack->succeed_by, 0);
	}

	if (attack->succeed_by >= 0) {

		// Get the place victim's gonna be hit.
		if (attack->location == -1)
			attack->location = find_hit_location(victim, combat_moves[attack->move].locationmod);
	
		// The victim is going to have to defend.	Get the victim's defense roll.
		if (GET_POS(victim) > POS_SLEEPING && (GET_ACTION(victim) == NULL) && 
			!AFF_FLAGGED(victim, AffectBit::Immobilized) && defense.mod < 100) {
			defense.move = victim->combat_profile.CombatMove(COMBAT_DEFENSIVE);
				
			if (defense.move) {
	
				if (defense.move == Combat::AutoDefend)
					defense.move = auto_defend(this);

				if (defense.move == Combat::Block) {
					defense.weapon = grab_shield(victim, &defense);
					if (defense.weapon == NULL) {
						victim->MergeSend("You have no shield to block with!  You try to dodge...\r\n");
						defense.move = Combat::Dodge;
					}
				} else {
					switch (defense.move) {
					case Combat::ParryRight:
					case Combat::ParryLeft:
					case Combat::ParryThird:
					case Combat::ParryFourth:
						attackweight = attack->weapon ? GET_WEIGHT(attack->weapon) : (GET_WEIGHT(victim) / 10);
						if ((attack->damtype == Damage::Basic || 
							attack->damtype == Damage::Undefined || attack->message == TYPE_CRUSH) &&
							attack->dam >= 80) {
							victim->MergeSend("You can't parry such a powerful attack!  You try to dodge...\r\n");
							defense.move = Combat::Dodge;
						} else if (attack->weapon && defense.weapon && attackweight > 10 && 
									attackweight > (GET_WEIGHT(defense.weapon) * 2)) {
							victim->MergeSend("You can't parry such a large weapon!  You try to dodge...\r\n");
							defense.move = Combat::Dodge;
						}
						break;
					}

					if ((defense.hand > WEAR_HAND_R) && (!ADVANTAGED(victim, Advantages::Ambidextrous)))
						defense.mod -= 4;
				}

				defense.mod -= MAX(POS_FIGHTING - GET_POS(victim), 0) * 2;

				if (defense.move == Combat::Block) {
					defense.skillnum = combat_moves[defense.move].skillnum;
					defense.mod += GET_OBJ_VAL(defense.weapon, 2);
					defense.chance = SKILLCHANCE(victim, defense.skillnum, NULL);
					defense.speed = combat_moves[defense.move].speed;
					defense.succeed_by = CHAR_SKILL(victim).RollSkill(victim, defense.skillnum, defense.chance) + defense.mod;
					CHAR_SKILL(victim).RollToLearn(victim, defense.skillnum, defense.succeed_by);
	
					// For a successful shield block, all we've done is change the
					// hit location.	To keep things balanced, from now on we consider 
					// the move a failure.
					if (defense.succeed_by >= 0) {
						attack->location = defense.hand;
						// We subtract from the success roll, meaning that only on successes
						// of 4 or more are blows really "deflected".	Otherwise, the damage
						// resistance of the shield is all that matters.
						defense.succeed_by -= -4;
					}
	
				} else {
					defense.WeaponMove(victim, defense.move);
					defense.chance = SKILLCHANCE(victim, defense.skillnum, NULL);
	
					if (defense.weapon) {
						defense.succeed_by = CHAR_SKILL(victim).RollSkill(victim, defense.skillnum, defense.chance) + defense.mod;
						CHAR_SKILL(victim).RollToLearn(victim, defense.skillnum, defense.succeed_by);
					} else {
						switch (defense.move) {
						case Combat::ParryRight:
						case Combat::ParryLeft:
							attack->location = WEAR_HANDS;
							if (!(GET_EQ(this, WEAR_HANDS) && !Number(0, 3)))
								victim->Sendf("No good!  You're trying to parry with your bare %ss!\r\n", 
												GET_WEARSLOT(WEAR_HAND_R, GET_BODYTYPE(victim)).partname);
							defense.succeed_by = -9;	 // Emulate a failure so that the attack strikes the body part that parried
							break;
						case Combat::ParryThird:
						case Combat::ParryFourth:
							attack->location = WEAR_HANDS_2;
							if (!(GET_EQ(this, WEAR_HANDS_2) && !Number(0, 3)))
								victim->Sendf("No good!  You're trying to parry with your bare %ss!\r\n", 
												GET_WEARSLOT(WEAR_HAND_R, GET_BODYTYPE(victim)).partname);
							defense.succeed_by = -9;	 // Emulate a failure so that the attack strikes the body part that parried
							break;
						default:
							defense.succeed_by = CHAR_SKILL(victim).RollSkill(victim, defense.skillnum, defense.chance) + defense.mod;
							CHAR_SKILL(victim).RollToLearn(victim, SKILL_BRAWLING, defense.succeed_by);
							break;
						}
					}
				}
	
				defense.message = combat_moves[defense.move].message;
				victim->combat_profile.CombatCost(victim, COMBAT_DEFENSIVE, defense.speed);
				vweapon = defense.weapon;
			}
		}	// End of victim defense
	
		if ((attack->location >= 0) && GET_EQ(victim, attack->location) &&
			((GET_OBJ_TYPE(GET_EQ(victim, attack->location)) == ITEM_ARMOR) ||
			 (GET_OBJ_TYPE(GET_EQ(victim, attack->location)) == ITEM_CONTAINER) ||
			 (GET_OBJ_TYPE(GET_EQ(victim, attack->location)) == ITEM_WEAPONCON))) {
			damage_resistance = GET_DR(GET_EQ(victim, attack->location));
			passive_defense = GET_PD(GET_EQ(victim, attack->location));
		} else {
			attack->dam = MAX(attack->dam, 1);
		}

		damage_resistance += GET_DR(victim);

		// Critical success on the attack denies the victim a successful defense
		if (attack->succeed_by >= SKILL_CRIT_SUCCESS) {
			defense.succeed_by = -1;
		}
	
		// Flying gives an automatic dodge defense
		if ((defense.succeed_by < 0) && AFF_FLAGGED(victim, AffectBit::Fly) && 
        		(!AFF_FLAGGED(this, AffectBit::Fly))) {
			defense.move = Combat::Dodge;
			defense.skillnum = combat_moves[Combat::Dodge].skillnum;
			defense.message = combat_moves[Combat::Dodge].message;
			defense.chance = SKILLCHANCE(victim, defense.skillnum, NULL);
			defense.succeed_by = CHAR_SKILL(victim).RollSkill(victim, defense.skillnum, defense.chance) + defense.mod;
			CHAR_SKILL(victim).RollToLearn(victim, defense.skillnum, defense.succeed_by);
		}

		if (defense.succeed_by < 0) {	// Victim's active defense failed.
	
			if (attack->succeed_by - passive_defense >= 0) {
				// Victim's passive defense failed, try to damage through armor!
				if (attack->dam - damage_resistance > 0) {
					attack->dam -= damage_resistance;
					Send("&cY");
					victim->Send("&cB");
					victim->Damage(this, attack);
					Send("&cY");
					victim->Send("&cB");
				} else {
					armor_message(attack->dam, this, victim, attack->message, attack->location);
					attack->response = -99; // We'll use this to indicate armor deflection.
				}
			} else {
				armor_message(0, this, victim, attack->message, attack->location);
				attack->response = -99; // We'll use this to indicate armor deflection.
			}
		} else {
			// Defense roll succeeded.
			Send("&cY");
			victim->Send("&cB");
			if (defense.move == Combat::Block)
				armor_message(0, this, victim, attack->message, attack->location);
			else {
				Send("&cY");
				victim->Send("&cB");
				Combat::SkillMessage(defense.succeed_by + 1, victim, this, defense.message);
				Send("&c0");
				victim->Send("&c0");
			}

			if (*combat_moves[defense.move].command_pointer != NULL) {
				tempfighting = FIGHTING(victim);	// We need to swap this to assure that riposte hits the attacker.
					FIGHTING(victim) = this;
				(*combat_moves[defense.move].command_pointer) // Perform the defense!
				(victim, "", 1, "", combat_moves[defense.move].subcmd);
				if (!PURGED(victim) && !PURGED(tempfighting)) {
					FIGHTING(victim) = tempfighting;
				}
				tempfighting = NULL;
			}
			Send("&c0");
			victim->Send("&c0");
		}
	} else {
		// ch's attack failed
		Send("&cg");
		victim->Send("&cr");
		Combat::SkillMessage(0, this, victim, attack->message);
		Send("&c0");
		victim->Send("&c0");
	}

	retval = attack->dam;

	if (created_attack) {
		delete attack;
	} else {
		// This is going to be passed back.	This helps the calling routine know
		// the outcome.	SKILL_NOCHANCE indicates that counter hasn't been changed
		if (attack->response == SKILL_NOCHANCE)
			attack->response = defense.succeed_by; 
	}

	return retval;
}


inline void CombatMessages::Message::Send(Character *ch, Character *victim, MUDObject *weap) const 
{
	if (Attacker() && ch != victim)		act(Attacker(), FALSE, ch, weap, victim, TO_CHAR);
	if (Victim())						act(Victim(), FALSE, ch, weap, victim, TO_VICT | TO_SLEEP);
	if (Room())							act(Room(), FALSE, ch, weap, victim, TO_NOTVICT);	
}


inline void CombatMessages::Message::SendVar(Character *ch, Character *victim, MUDObject *weap,
									char *damsingular, char *damplural,
									char *attackadj, char *attackadverb,
									char *defenseadj, char *defenseadverb, char *hitlocation,
									char *attacktype) const 
{
	if (Attacker() && (ch != victim)) {
		act(replace_string(Attacker(), damsingular, damplural, attackadj, attackadverb,
			defenseadj, defenseadverb, hitlocation, attacktype), 
			FALSE, ch, weap, victim, TO_CHAR);
	}

	if (Victim()) {
		act(replace_string(Victim(), damsingular, damplural, attackadj, attackadverb,
			defenseadj, defenseadverb, hitlocation, attacktype), 
			FALSE, ch, weap, victim, TO_VICT | TO_SLEEP);
	}

	if (Room()) {
    	act(replace_string(Room(), damsingular, damplural, attackadj, attackadverb,
			defenseadj, defenseadverb, hitlocation, attacktype), 
			FALSE, ch, weap, victim, TO_NOTVICT);
	}
}


void Combat::SendAttack(Character *ch, Character *victim, Attack *attack, SInt16 defensemove = -1) {
	Result offenserate = Success, defenserate = Success;
	char *aaskill, *avskill, *daskill, *dvskill, *hitlocation = NULL;
	int i = 0;
	static char to_ch[256], to_victim[256], to_room[256];
	bool isthrust = false;
	
	if (SKL_IS_SPELL(attack->message)) {
		sprintf(to_ch, "Your %s ", Skill::Name(attack->message));
		sprintf(to_victim, "$n's %s ", Skill::Name(attack->message));
		sprintf(to_room, "$n's %s ", Skill::Name(attack->message));
	} else {
		strcpy(to_ch, "You ");
		strcpy(to_victim, "$n ");
		strcpy(to_room, "$n ");
	}
	
	//	#w	damage singular
	//	#W	damage plural
	//	#a	attack skill adjective
	//	#A	attack skill adverb
	//	#d	defensemove skill adjective
	//	#D	defensemove skill adverb
	//	#l	hitlocation

	// Get a string rating of how well the combatants did
	if (attack->succeed_by <= SKILL_NOCHANCE)	offenserate = Blunder;
	else if (attack->succeed_by <= -10)			offenserate	= AFailure;
	else if (attack->succeed_by <= -1)			offenserate	= Failure;
	else if (attack->succeed_by == 0)			offenserate	= NSuccess;
	else if (attack->succeed_by <= 9)			offenserate	= Success;
	else 										offenserate	= ASuccess;

	if (defensemove <= SKILL_NOCHANCE)			defenserate = Blunder;
	else if (defensemove <= -10)				defenserate	= AFailure;
	else if (defensemove <= -1)					defenserate	= Failure;
	else if (defensemove == 0)					defenserate	= NSuccess;
	else if (defensemove <= 9)					defenserate	= Success;
	else 										defenserate	= ASuccess;

	aaskill = skillrates[offenserate];
	avskill = skillhow[offenserate];
	daskill = skillrates[defenserate];
	dvskill = skillhow[defenserate];

	while (damage_type_messages[attack->damtype][i + 1].upto >= 0) {
		if (attack->dam < damage_type_messages[attack->damtype][i + 1].upto)
			break;
		++i;
	}
	
	if (attack->weapon) {
		if ((attack->move == Combat::ThrustRight) || (attack->move == Combat::ThrustLeft) ||
			(attack->move == Combat::ThrustThird) || (attack->move == Combat::ThrustFourth))
			isthrust = true;

		sprintf(to_ch + strlen(to_ch), "%s%s your $o, %s $N", avskill, isthrust ? "thrust" : "swing",
				damage_type_messages[attack->damtype][i].progressive);
		sprintf(to_victim + strlen(to_victim), "%s%ss $s $o, %s you", avskill, isthrust ? "thrust" : "swing",
				damage_type_messages[attack->damtype][i].progressive);
		sprintf(to_room + strlen(to_room), "%s%ss $s $o, %s $N", avskill, isthrust ? "thrust" : "swing",
				damage_type_messages[attack->damtype][i].progressive);
	} else if (SKL_IS_SPELL(attack->message)) {
		sprintf(to_ch + strlen(to_ch), "%s $N", damage_type_messages[attack->damtype][i].plural);
		sprintf(to_victim + strlen(to_victim), "%s you", damage_type_messages[attack->damtype][i].plural);
		sprintf(to_room + strlen(to_room), "%s $N", damage_type_messages[attack->damtype][i].plural);
	} else {
		sprintf(to_ch + strlen(to_ch), "%s $N", damage_type_messages[attack->damtype][i].singular);
		sprintf(to_victim + strlen(to_victim), "%s you", damage_type_messages[attack->damtype][i].plural);
		sprintf(to_room + strlen(to_room), "%s $N", damage_type_messages[attack->damtype][i].plural);
	}
	
	if (attack->location >= 0) {
		hitlocation = GET_WEARSLOT(attack->location, GET_BODYTYPE(victim)).partname;
		sprintf(to_ch + strlen(to_ch), "'s %s", hitlocation);
		sprintf(to_victim + strlen(to_victim), " in the %s", hitlocation);
		sprintf(to_room + strlen(to_room), "'s %s", hitlocation);
	}

	if (attack->dam >= 20) {
		strcat(to_ch, "!");
		strcat(to_victim, "!");
		strcat(to_room, "!");
	} else {
		strcat(to_ch, ".");
		strcat(to_victim, ".");
		strcat(to_room, ".");
	}

	if (ch != victim) {
		act(to_ch, FALSE, ch, attack->weapon, victim, TO_CHAR);
	}
	act(to_victim, FALSE, ch, attack->weapon, victim, TO_VICT | TO_SLEEP);
	act(to_room, FALSE, ch, attack->weapon, victim, TO_NOTVICT);	
}


char *replace_string(const char *str, char *damsingular, char *damplural,
							char *attackadj, char *attackadverb,
					 		char *defenseadj, char *defenseadverb, char *hitlocation,
							char *attacktype)
{
	static char buf[256];
	register char *from = NULL, *to;

	to = buf;

	while (*str) {
		if (*str == '#') {
			switch (*(++str)) {
			case 'w':	from = damsingular;		break;
			case 'W':	from = damplural;		break;
			case 'a':	from = attackadj;		break;
			case 'A':	from = attackadverb;	break;
			case 'd':	from = defenseadj;		break;
			case 'D':	from = defenseadverb;	break;
			case 'l':	from = hitlocation;		break;
			case 't':	from = attacktype;		break;
			default:
				*(to++) = '#';
				break;
			}
		} else
			*(to++) = *str;

		if (from) {
			while (*from)
				*(to++) = *(from++);
			from = NULL;
		}
		
		++str;

		*to = 0;
	}				/* For */

	return (buf);
}


// Fetch the list of messages for this type
CombatMessages::MessageType	* PickMessage(int move)
{
	int nr;
	CombatMessages::MessageList *messagelist;
	CombatMessages::MessageType	*messages;

	// Fetch the list of messages for this type
	messagelist = CombatMessages::Find(move);
	
	if (messagelist == NULL) {
		return NULL;
		// core_dump();
		// exit(1);
	}

	// Randomly pick a message to send
	nr = Number(1, messagelist->Messages.Count());
	START_ITER(iter, messages, CombatMessages::MessageType *, messagelist->Messages) {
		if (!(--nr))
			break;
	}
	
	return messages;
}


void dummy_attack_message(Character *ch, Character *victim, int message)
{
	sprintf(buf, "SYSERR:  Missing message (%s) for %s's move against %s.", Skill::Name(message), ch->Name(""), victim->Name(""));
    act(buf, 0, ch, 0, victim, TO_ROOM);
	mudlogf(NRM, NULL, FALSE, buf);
}
	

// message for doing damage with a weapon.
// Returns 0 for no message generated, 1 otherwise.
int Combat::AttackMessage(Character *ch, Character *victim, Attack *attack = NULL, SInt16 defense = -1) 
{
	CombatMessages::MessageType	*messages;
	char *aaskill = NULL, *avskill = NULL, *daskill = NULL, *dvskill = NULL, *hitlocation = NULL;
	char *damagesingular = NULL, *damageplural = NULL;
	int i;
	Attack temp_attack;

	if (!attack) {
		attack = &temp_attack;
		attack->succeed_by = 1;	// Such a case assumes success.
	}

	if (attack->succeed_by >= 0) {
		// The attack succeeded
		if (defense >= 0 && attack->response >= 0) {
			// The defense succeeded as well.
			messages = PickMessage(combat_moves[defense].message);
	
			if (!messages) {	// This should never happen, but...
            	dummy_attack_message(ch, victim, combat_moves[defense].message);
                return 0;
				// sprintf(buf, "Missing message for defense move #%d.  Tell the imps!", attack->move);
				// act(buf, 0, ch, 0, 0, TO_ROOM);
				// core_dump();
				// exit(1);
			}

			// The defense succeeded, so we're using the defenses' hit messages.  Otherwise,
			// we'd get a message for a failed defense.
			messages->hit.SendVar(ch, victim, attack->weapon, 
							"", "", aaskill, avskill, 
							daskill, dvskill, "", combat_moves[defense].move);
		} else {
			// The attact connected and the defense failed.
			messages = PickMessage(attack->message);
			
			if (!messages) {	// This should never happen, but...
            	dummy_attack_message(ch, victim, attack->message);
                return 0;
				// sprintf(buf, "Missing message for attack move #%d.  Tell the imps!", attack->move);
				// act(buf, 0, ch, 0, 0, TO_ROOM);	// FIXME:  This broke with 402 "whip".
				// core_dump();
				// exit(1);
			}

			if (attack->location >= 0)
				hitlocation = GET_WEARSLOT(attack->location, GET_BODYTYPE(victim)).partname;
	
			i = 0;
			while (damage_type_messages[attack->damtype][i + 1].upto >= 0) {
				if (attack->dam < damage_type_messages[attack->damtype][i + 1].upto)
					break;
				++i;
			}
			
			damagesingular = damage_type_messages[attack->damtype][i].singular;
			damageplural = damage_type_messages[attack->damtype][i].plural;
	
			// Attack succeeded, defense failed
			if (!IS_NPC(victim) && IS_IMMORTAL(victim) && (!IS_IMMPUN(ch))) {
				Combat::SendAttack(ch, victim, attack, defense);
			} else if (GET_POS(victim) == POS_DEAD) {
				ch->Send("&cG");
				victim->Send("&cR");
				messages->die.SendVar(ch, victim, attack->weapon, 
								damagesingular, damageplural, aaskill, avskill, 
								daskill, dvskill, hitlocation, combat_moves[attack->move].move);
				ch->Send("&c0");
				victim->Send("&c0");
			} else {
				ch->Send("&cG");
				victim->Send("&cR");
				if (!IS_WEAPON(attack->message) || attack->dam == 0) {
					messages->hit.SendVar(ch, victim, attack->weapon, 
								damagesingular, damageplural, aaskill, avskill, 
								daskill, dvskill, hitlocation, combat_moves[attack->move].move);
				} else {
					Combat::SendAttack(ch, victim, attack, defense);
				}
				ch->Send("&c0");
				victim->Send("&c0");
			}
		}
	} else {
		// The attack missed altogether

		messages = PickMessage(attack->message);
		
		if (!messages) {	// This should never happen, but...
            dummy_attack_message(ch, victim, attack->message);
			return 0;
			// sprintf(buf, "Missing message for attack move #%d.  Tell the imps!", attack->move);
			// act(buf, 0, ch, 0, 0, TO_ROOM);
			// core_dump();
			// exit(1);
		}
	
	
		if (!IS_NPC(victim) && IS_IMMORTAL(victim) && (!IS_IMMPUN(ch))) {
			messages->staff.SendVar(ch, victim, attack->weapon, 
							damagesingular, damageplural, aaskill, avskill, 
							daskill, dvskill, hitlocation, combat_moves[attack->move].move);
		} else {
			ch->Send("&cg");
			victim->Send("&cr");
			messages->miss.SendVar(ch, victim, attack->weapon, 
							damagesingular, damageplural, aaskill, avskill, 
							daskill, dvskill, hitlocation, combat_moves[attack->move].move);
			ch->Send("&c0");
			victim->Send("&c0");
		}
	}

	return 1;
}


int Combat::SkillMessage(int dam, Character * ch, Character * vict,
		  int attacktype, Attack *attack = NULL)
{
	int i, j, nr;
	SInt8 weaponslot;
	CombatMessages::MessageType	*msg = NULL;
	bool created_attack = false;
	char *aaskill = NULL, *avskill = NULL, *hitlocation = NULL;
	char *damagesingular = NULL, *damageplural = NULL;

	msg = PickMessage(attacktype);
	
	if (!msg)		return 0;

	if (!attack) {
		attack = new Attack;
		created_attack = true;
	}
	
	// CHANGEPOINT:  This is really really ugly.  Basically we cannot expect a correct message if
	// we want to refer to the weapon - it might not be the one used!
	weaponslot = find_eqtype_pos(ch, ITEM_WEAPON);
	if ((weaponslot >= 0) && !(SKL_IS_SPELL(attack->message)))	// Spells should not involve weapons.
		attack->weapon = GET_EQ(ch, weaponslot);

	if (!IS_NPC(vict) && IS_IMMORTAL(vict) && (!IS_IMMPUN(ch))) {
		msg->staff.Send(ch, vict, attack->weapon);
	} else if (dam != 0) {
		// ch->Send("&cG");
		// vict->Send("&cR");
		if (GET_POS(vict) == POS_DEAD) {
			msg->die.Send(ch, vict, attack->weapon);
		} else {
			if (msg->hit.Attacker()) {
				if (created_attack) {		// If this is the case, we can't expect extra information.
					msg->hit.Send(ch, vict, attack->weapon);
				} else {
					Result offenserate = Success, defenserate = Success;

					// Get a string rating of how well the combatants did
					if (attack->succeed_by <= SKILL_NOCHANCE)	offenserate = Blunder;
					else if (attack->succeed_by <= -10)			offenserate	= AFailure;
					else if (attack->succeed_by <= -1)			offenserate	= Failure;
					else if (attack->succeed_by == 0)			offenserate	= NSuccess;
					else if (attack->succeed_by <= 9)			offenserate	= Success;
					else 										offenserate	= ASuccess;

					aaskill = skillrates[offenserate];
					avskill = skillhow[offenserate];

					i = 0;
					while (damage_type_messages[attack->damtype][i + 1].upto >= 0) {
						if (attack->dam < damage_type_messages[attack->damtype][i + 1].upto)
							break;
						++i;
					}
					damagesingular = damage_type_messages[attack->damtype][i].singular;
					damageplural = damage_type_messages[attack->damtype][i].plural;

					if (attack->location >= 0)
						hitlocation = GET_WEARSLOT(attack->location, GET_BODYTYPE(vict)).partname;
					
					msg->hit.SendVar(ch, vict, attack->weapon, 
							damagesingular, damageplural, aaskill, avskill, 
							"", "", hitlocation, "");
				}
			}
			else							Combat::SendAttack(ch, vict, attack);
		}
		// ch->Send("&c0");
		// vict->Send("&c0");
	} else if (ch != vict) {	/* Dam == 0 */
		// ch->Send("&cY");
		// vict->Send("&cr");
		msg->miss.Send(ch, vict, attack->weapon);
		// ch->Send("&c0");
		// vict->Send("&c0");
	}

	if (created_attack)		delete attack;

	return 1;
}


void Combat::Explosion(SInt32 ownerId, UInt32 diceNumber, UInt32 diceSize, VNum room, bool shrapnel) {
	Character	*ch, *owner;
	UInt32		door;
	SInt32		maxDamage = diceNumber * diceSize;
	VNum		exitRoom;
	LListIterator<Character *>	iter;
	Attack		attack;

	if (room == NOWHERE)
		return;
	
	attack.damtype = Damage::Explosion;

	owner = find_char(ownerId);
	
	iter.Start(world[room].people);
	while ((ch = iter.Next())) {
		if (NO_STAFF_HASSLE(ch))		//	You can't damage an immortal!
			continue;
		
		act("You are caught in the blast!", TRUE, ch, 0, 0, TO_CHAR);
		attack.dam = dice(diceNumber, diceSize) - GET_DR(ch);
		ch->Damage(owner, &attack);
	}
	
	for (door = 0; door < NUM_OF_DIRS; door++) {
		if (!CAN_GO2(room, door) || ((exitRoom = EXITN(room,door)->to_room) == room))
			continue;

		world[exitRoom].Send("You hear a loud explosion come from %s!\r\n", dir_text[rev_dir[door]]);
		
		/* Extended explosion sound */
		if ((maxDamage >= 250) && CAN_GO2(exitRoom, door) && (EXITN(exitRoom, door)->to_room != room))
			world[EXITN(exitRoom, door)->to_room].Send("You hear a loud explosion come from %s!\r\n", dir_text[rev_dir[door]]);
		/* End Extended sound */
		
		if (!shrapnel || ROOM_FLAGGED(exitRoom, ROOM_PEACEFUL) ||
				(maxDamage < 250) || ((maxDamage < 500) && Number(0,1)))
			continue;
			
		world[exitRoom].Send("Shrapnel flies into the room from %s!\r\n", dir_text[rev_dir[door]]);
		
		iter.Restart(world[exitRoom].people);
		while ((ch = iter.Next())) {
			/* You can't damage an immortal! */
			if (NO_STAFF_HASSLE(ch) || ((maxDamage < 750) && Number(0,1)))
				continue;
				
			act("You are hit by shrapnel!", TRUE, ch, 0, 0, TO_CHAR);
			attack.dam = dice(diceNumber / 2, diceSize / 2) - GET_DR(ch);
			ch->Damage(owner, &attack);
		}
	}
}


void Combat::MakeCorpse(Character * ch) {
	Object		*corpse, *o, *money;
	char *		buf = get_buffer(SMALL_BUFSIZE);
	
	corpse = Object::Create();
	
#ifdef GOT_RID_OF_IT
	corpse->SetName(ch->parts ? "corpse" : "torso");
	
	sprintf(buf, "The %s%s of %s is lying here.",
			ch->parts && ch->parts == ch->realParts ? "mutilated " : "",
			ch->parts ? "corpse" : "torso",
			ch->Name());
	corpse->description = SString::Create(buf);
	
	sprintf(buf, "the %s%s of %s",
			ch->parts && ch->parts == ch->realParts ? "mutilated " : "",
			ch->parts ? "corpse" : "torso",
			ch->Name());
	corpse->SetSDesc(buf);
#endif
	
	// sprintf(buf, "corpse %s", IS_NPC(ch) ? ch->Name("someone") : ch->SDesc("someone"));
	sprintf(buf, "corpse %s", ch->Keywords("someone"));
	corpse->SetKeywords(buf);
	sprintf(buf, "the corpse of %s", IS_NPC(ch) ? ch->Name("someone") : ch->SDesc("someone"));
	corpse->SetName(buf);
	sprintf(buf, "The corpse of %s is lying here.", IS_NPC(ch) ? ch->Name("someone") : ch->SDesc("someone"));
	corpse->SetSDesc(buf);

	release_buffer(buf);
	
	corpse->type	= ITEM_CONTAINER;
	corpse->wear	= ITEM_WEAR_TAKE;
	corpse->extra	= ITEM_NODONATE;
	corpse->value[0]= 0;	//	You can't store stuff in a corpse
	corpse->weight	= GET_WEIGHT(ch);
	corpse->material = ch->material;
	
	/* corpse identifier */
	if (IS_NPC(ch))	{
		GET_OBJ_VAL(corpse, 3) = (ch->Virtual());
	} else {
		GET_OBJ_VAL(corpse, 3) = -(GET_IDNUM(ch));
	}

	/* transfer character's equipment to the corpse */
	for (SInt32 i = 0; i < NUM_WEARS; i++)
		if (GET_EQ(ch, i))
			GET_EQ(ch, i)->ExtractNoKeep(corpse, ch);
	
	LListIterator<Object *>	iter(ch->contents);
	while ((o = iter.Next())) {
		o->ExtractNoKeep(corpse, ch);
	}
	
	/* transfer gold */
	if (GET_GOLD(ch) > 0) {
		/* following 'if' clause added to fix gold duplication loophole */
		if (IS_NPC(ch) || (!IS_NPC(ch) && ch->desc)) {
			money = create_money(GET_GOLD(ch));
			money->ToObj(corpse);
		}
		GET_GOLD(ch) = 0;
	}

	if (!IS_NPC(ch)) {
		Crash_crashsave(ch);
	} else {
		corpse->timer = corpse->contents.Count() ? max_npc_corpse_time : 1;
	}

	
	corpse->ToRoom(IN_ROOM(ch));
}


void Combat::DeathCry(Character * ch) {
	act("Your blood freezes as you hear $n's death cry.", FALSE, ch, 0, 0, TO_ROOM);

	for (SInt32 door = 0; door < NUM_OF_DIRS; door++)
		if (CAN_GO(ch, door))
			world[EXIT(ch,door)->to_room].Send("Your blood freezes as you hear someone's death cry.\r\n");
}


// Daniel's support routine for group experience gain as part of infinite levels.
// Returns the sum of the levels of the character's group.
SInt32 rate_group_points(Character *ch)
{
	SInt32 totalpts, maxpts = 0, members = 0, rating;
	Character *k, *follower;

	if (!(k = ch->master))
		k = ch;

	if (AFF_FLAGGED(k, AffectBit::Group) && (IN_ROOM(k) == IN_ROOM(ch))) {
		totalpts = GET_TOTALPTS(k);
		maxpts = totalpts;
		++members;
	} else {
		totalpts = 0;
	}

	START_ITER(iter, follower, Character *, k->followers) {
		if (AFF_FLAGGED(follower, AffectBit::Group) && (IN_ROOM(follower) == IN_ROOM(ch))) {
			totalpts += GET_TOTALPTS(follower);
			if (GET_TOTALPTS(follower) > maxpts)
				maxpts = GET_TOTALPTS(follower);
			++members;
		}
	}

	if (members == 0)		return 0;
	else if (members == 1)	return totalpts;
		
	rating = (maxpts * 3) + totalpts;
	rating /= 4;
		
	return (MAX(rating, 1));
}


SInt32 exp_from_level(SInt32 chlevel, SInt32 fromlevel, SInt32 modifier)
{
	SInt32 gain;

	chlevel = RANGE(1, chlevel, 5000);
	fromlevel = RANGE(1, fromlevel, 5000);

	gain = ((fromlevel + modifier) * 100) / chlevel;
	gain *= gain;
	gain /= 100;
	
	return gain;
}


UInt32 Combat::CalcReward(Character *ch, Character *victim) {
	unsigned long points, gain;
	int hitpercent;
	Character *k, *follower;
	
	if (AFF_FLAGGED(ch, AffectBit::Group))		points = rate_group_points(ch);
	else										points = GET_TOTALPTS(ch);

	if (points < 1)								points = 1;
		
	// This formula allows for automatic calculation of experience based on
	// the relative levels of the character and victim.
	// Formula:	(100 + modifier) * ((victim level / ch level) squared)

	gain = exp_from_level(points, GET_TOTALPTS(victim), GET_EXP_MOD(victim));

	if (victim->general.damage_taken) {
		hitpercent = (victim->general.damage_taken * 100) / MAX(GET_MAX_HIT(victim), 1);
		gain = gain * hitpercent / 100;
	} else {
		gain = 0;
	}

	// sprintf(buf, "Distributing exp:	gain: %ld	points: %ld\r\n", gain, points);
	// log(buf);

	if (AFF_FLAGGED(ch, AffectBit::Group)) {
		if (!(k = ch->master))		k = ch;

		if (AFF_FLAGGED(k, AffectBit::Group) && (IN_ROOM(k) == IN_ROOM(ch))) {
			kill_increase_exp(k, gain * GET_TOTALPTS(k) / points);
		}

		START_ITER(iter, follower, Character *, k->followers) {
			if (AFF_FLAGGED(follower, AffectBit::Group) && (IN_ROOM(follower) == IN_ROOM(ch))) {
				kill_increase_exp(follower, gain * GET_TOTALPTS(follower) / points);
			}
		}
	} else {
		kill_increase_exp(ch, gain);
	}
	
	return gain;
}


void Combat::Announce(const char *buf) {
	Descriptor *pt;
	START_ITER(iter, pt, Descriptor *, descriptor_list) {
		if ((STATE(pt) == CON_PLAYING) && !CHN_FLAGGED(pt->character, Channel::NoInfo))
			pt->character->Send(buf);
	}
}


void Combat::MobReaction(Character *attacker, Character *mob, SInt32 dir) {
	if (!IS_NPC(mob) || FIGHTING(mob) || GET_POS(mob) <= POS_STUNNED)
		return;

	if (MOB_FLAGGED(mob, MOB_MEMORY))		remember(mob, attacker);
	if (GET_POS(mob) != POS_STANDING)		GET_POS(mob) = POS_STANDING;
	if (MOB_FLAGGED(mob, MOB_WIMPY))		do_flee(mob, "", 0, "flee", 0);
//	else	mob->path = Path::ToChar(IN_ROOM(mob), attacker, 10, Path::Global);
	else {
		if ((MOB_FLAGGED(mob, MOB_SENTINEL)) && !ROOMSEEKING(mob)) 
        	ROOMSEEKING(mob) = mob->InRoom();
		HUNTING(mob) = GET_ID(attacker);
	}
}


#ifdef GOT_RID_OF_IT
SInt32 Combat::Delay(Character *ch) {
	Object *	weapon;
	SInt32		speed;
	
	speed = base_response[(int)GET_RACE(ch)];	//	Get the base speed
	
	weapon = GET_EQ(ch, WEAR_HAND_R);
	if (!weapon || (GET_OBJ_TYPE(weapon) != ITEM_WEAPON))
		weapon = GET_EQ(ch, WEAR_HAND_L);
	
	if (weapon && (GET_OBJ_TYPE(weapon) == ITEM_WEAPON)) {
			speed += GET_OBJ_SPEED(weapon);		//	The speed of the weapon is important
			speed += 1;							//	Force it to round up by incrementing
			speed /= 2;							//	Divide by 2, for average
    }
	speed -= GET_STAT(ch, Ag) / 40;	//	Minor bonuses for agility
	return MAX(1, speed);		//	Minimum of 1
}
#endif


#define GET_WAIT(ch) (IS_NPC(ch) ? GET_MOB_WAIT(ch) : ((ch)->desc ? (ch)->desc->wait : 0))


ACMD(do_reload);

// EVENTFUNC(Combat::AttackEvent) {
// 	CombatEvent *	CEData = static_cast<CombatEvent *>(event_obj);
// 	Character		*ch, *victim;
// 	Object			*weapon;
// 	SInt32			attackRate = 1;
// 	
// 	if (!CEData || !(ch = find_char(CEData->attackerId)))		//	Something fucked up... this shouldn't be!
// 		return 0;
// 	
// 	
// 	//	They stopped fighting already.
// 	victim = FIGHTING(ch);
// 	if (!victim || (IN_ROOM(victim) != IN_ROOM(ch)))	{
// 		if (victim)	ch->StopFighting();
// 		victim = NULL;
// 		//	Eventually a routine will be here to find another combatant
// 		//	But for now, it doesn't matter since this code isn't even used yet
// 		
// 		if (victim)	ch->Fight(victim);
// 		else {
// 			ch->events.Remove(event);
// 			return 0;
// 		}
// 	}
// 	
// 	if (!Combat::Valid(ch, victim)) {		//	For wussy PK-optional MUDs
// 		if (!PRF_FLAGGED(ch, Preference::PK))
// 			ch->Send("You must turn on PK first!  Type \"pkill\" for more information.\r\n");
// 		else if (!PRF_FLAGGED(victim, Preference::PK))
// 			act("You cannot kill $N because $E has not chosen to participate in PK.", FALSE, ch, 0, victim, TO_CHAR);
// 		ch->events.Remove(event);
// 		return 0;
// 	}
// 	
// //	if ((GET_WAIT(ch) / PASSES_PER_SEC) > 0)
// 	if (GET_WAIT(ch) > 1)
// 		return GET_WAIT(ch);
// 	
// 	if (GET_POS(ch) < POS_FIGHTING) {
// 		if (GET_POS(ch) <= POS_STUNNED)
// 			return ((base_response[GET_RACE(ch)] * 2) - (GET_STAT(ch, Co) / 25)) RL_SEC;
// 		else if (IS_NPC(ch)) {
// 			GET_POS(ch) = POS_FIGHTING;
// 			if (ch->sitting_on)		ch->sitting_on = NULL;
// 			act("$n scrambles to $s feet!", TRUE, ch, 0, 0, TO_ROOM);
// 			return base_response[GET_RACE(ch)] * 2 RL_SEC;
// 		} else
// 			ch->Send("You're on the ground!  Get up, fast!\r\n");
// 	}
// 	
// 	if (!(weapon = GET_EQ(ch, WEAR_HAND_R)) || (GET_OBJ_TYPE(weapon) != ITEM_WEAPON))
// 		weapon = GET_EQ(ch, WEAR_HAND_L);
// 	
// 	if (IS_NPC(ch) && weapon && IS_GUN(weapon) && (GET_GUN_AMMO(weapon) <= 0)) {
// 		do_reload(ch, "", 0, "reload", SCMD_LOAD);
// 		if (GET_GUN_AMMO(weapon) > 0)
// 			return 6 RL_SEC;
// 	}
// 	
// 	if (ch->Hit(victim, Damage::Undefined, 1) < 0) {
// 		//	Find new victim?
// 		ch->events.Remove(event);
// 		return 0;
// 	} else
// 		return Combat::Delay(ch) RL_SEC;
// }


// Scans through a person's body slots, picking out a random slot that matches
// a 2d6 value on the person's hit location table and returning that slot's 
// number.
SInt8 find_hit_location(Character *victim, SInt8 modifier)
{

	// modifier is used to influence the results, as higher values will result in
	// hits to the lower portions of the body and vice versa.	We can use this to
	// simulate attacking from above and below.
	
	SInt8 i, 		// Counter
			 dice_roll, 	// Random number
			 num_slots = 0,	// Number of slots found corresponding to random dieroll
			 choose_slot = 0,	// Since multiple slots can have the same value on
						// the hit location table, we want a variable to 
						// indicate which one after we've scanned through.
			 bodytype = GET_BODYTYPE(victim);
	
		dice_roll = dice(2, 6);

		dice_roll = MIN(MAX(dice_roll + modifier, 2), 12);

		// Scan through the victims body slots, tallying up the number of slots that
		// match the hit location value.
		for (i = 0; i < NUM_WEARS; i++) {
			if (dice_roll == GET_WEARSLOT(i, bodytype).hitlocation)
	 	num_slots++;
		}

	if (!num_slots) {
		return WEAR_BODY;
	}

	// If we found no possible slots we best not go through the second for loop!
	// Otherwise, pick a number based on the tally to use.
	choose_slot = Number(1, num_slots);	// Pick one on the valid slots.
	
	// Get the number of the chosen wear slot using the random number,
	// based on the tally.
	for (i = 0; (i < NUM_WEARS) && (choose_slot > 0); i++) {
		if (dice_roll == GET_WEARSLOT(i, bodytype).hitlocation)
			choose_slot--;
	}
	
	return (i - 1);

}


// Sends a message for weapons deflecting off of armor.
void armor_message(int dam, Character * ch, Character * victim,
			 int w_type, SInt8 hitlocation = -1)
{
	Object *armor = NULL;
	char to_ch[80], to_victim[80], to_room[80];
	char locationname[40], hitmessage[40], attackmessage[40];

	*to_ch = '\0';
	*to_victim = '\0';
	*to_room = '\0';

	if (hitlocation >= 0) {
		armor = GET_EQ(victim, hitlocation);
		sprintf(locationname, "%s", armor ? "$o" : 
						GET_WEARSLOT(hitlocation, GET_BODYTYPE(victim)).partname);
	}

	if (dam) {
		sprintf(hitmessage, "is absorbed by");
	} else {
		sprintf(hitmessage, "glances off of");
	}

	if (w_type >= TYPE_HIT && w_type < LAST_COMBAT_MESSAGE) {
	// if (SKL_IS_COMBATMSG(w_type)) {
		sprintf(attackmessage, "%s", attack_hit_text[w_type - TYPE_HIT].singular);
	} else {
		sprintf(attackmessage, "%s", Skill::Name(w_type));
	}

	if (hitlocation >= 0) {
		sprintf(to_ch, "&ccYour %s %s $N's %s!&c0", attackmessage, hitmessage, locationname);
		sprintf(to_victim, "&cY$n's %s %s your %s!&c0", attackmessage, hitmessage, locationname);
		sprintf(to_room, "$n's %s %s $N's %s!", attackmessage, hitmessage, locationname);
	} else {
		sprintf(to_ch, "&ccYour %s %s $N!&c0", attackmessage, hitmessage);
		sprintf(to_victim, "&cY$n's %s %s you!&c0", attackmessage, hitmessage);
		sprintf(to_room, "$n's %s %s $N!", attackmessage, hitmessage);
	}

	act(to_ch, FALSE, ch, armor, victim, TO_CHAR);
	act(to_victim, FALSE, ch, armor, victim, TO_VICT);
	act(to_room, FALSE, ch, armor, victim, TO_NOTVICT);
}


int speed_bonus(int strength)
{
	if (strength < 9)
		return 0;

	int bonus = 0;
	
	strength -= 9;
	bonus = strength / 3;
	
	return (bonus);
}


int combat_move_chance(Character *ch, int move)
{
	if (((move == Combat::RiposteRight) ||
			(move == Combat::RiposteLeft) ||
			(move == Combat::RiposteThird) ||
			(move == Combat::RiposteFourth)) && 
			(SKILLCHANCE(ch, SKILL_RIPOSTE, 0) <= SKILL_NOCHANCE))
		return (SKILL_NOCHANCE);
	return (SKILLCHANCE(ch, combat_moves[move].skillnum, 0));
}


// Randomly decides whether to thrust or swing a weapon.
// Returns 1 for thrust, 0 for swing.
SInt16 swing_or_thrust(Character * ch, SInt8 & hand)
{
	Object *weapon = NULL;

	if (hand < 0)
		return (0);

	weapon = GET_EQ(ch, hand);

	if ((weapon == NULL) || (GET_OBJ_TYPE(weapon) != ITEM_WEAPON))
		return (0);

	if ((GET_OBJ_VAL(weapon, 5) == 0) && (GET_OBJ_VAL(weapon, 2) == 0))
		return (0);

	if (Number(0, 1))
		return ((GET_OBJ_VAL(weapon, 5) == 0) ? 0 : 1);

	return ((GET_OBJ_VAL(weapon, 2) == 0) ? 1 : 0);
	
}


// Routine for figuring out what weapon is involved, and if a weapon is
// found, what the skills and bonuses are.	Sets the variables hand, move, 
// weapon, message, skillnum, mod, speed, and dam.
int Attack::WeaponMove(Character * ch, SInt16 do_move)
{
	SInt8 is_thrust = FALSE, is_parry = FALSE;

	if (do_move < Combat::Hit)
		do_move = Combat::Hit;

	// If we're dealing with generic moves, pick a weapon and decide whether
	// to swing or thrust it.
	if (do_move == Combat::Hit) {
		hand = find_eqtype_pos(ch, ITEM_WEAPON, WEAR_HAND_R, WEAR_HAND_4, TRUE);

		switch (hand) {
		case WEAR_HAND_4:
			move = Combat::SwingFourth;
			break;
		case WEAR_HAND_3:
			move = Combat::SwingThird;
			break;
		case WEAR_HAND_L:
			move = Combat::SwingLeft;
			break;
		case WEAR_HAND_R:
			move = Combat::SwingRight;
			break;
		default:
			move = Combat::Punch;
		}

		is_thrust = swing_or_thrust(ch, hand);
		if (is_thrust)
			++move;

		dam = str_dam(GET_STR(ch), move) + GET_DAMROLL(ch);

	} else {
		move = do_move;
		if ((move == Combat::ThrustRight) ||
				(move == Combat::ThrustLeft) ||
				(move == Combat::ThrustThird) ||
				(move == Combat::ThrustFourth))
			is_thrust = TRUE;
		else if ((move == Combat::ParryRight) ||
				(move == Combat::ParryLeft) ||
				(move == Combat::ParryThird) ||
				(move == Combat::ParryFourth))
			is_parry = TRUE;
	}

	skillnum = combat_moves[move].skillnum;

	if ((!hand) && IS_WEAPONSKILL(skillnum)) {
		hand = WEAR_HAND_R + (skillnum - SKILL_WEAPON_R);
	}

	if (IS_WEAPONSKILL(skillnum)) {
		weapon = GET_EQ(ch, hand);
	} else {
		message = combat_moves[move].message;
		speed = combat_moves[move].speed;
	}

	if (weapon) {
		if (GET_OBJ_TYPE(weapon) == ITEM_WEAPON) {

			skillnum = GET_OBJ_VAL(weapon, 0);
			chance = SKILLCHANCE(ch, skillnum, NULL);

			if (is_parry) {
				message = SKILL_PARRY;
			} else if (is_thrust) {
				if ((damtype = GET_OBJ_VAL(weapon, 5)) != -1) {
					dam += GET_OBJ_VAL(weapon, 4);
					message = GET_OBJ_VAL(weapon, 6) + TYPE_HIT;
				} else {
					skillnum = PROFICIENCY_MACE;		// A generic object, use it as
					mod -= 4;								// a mace with penalties.
					message = TYPE_POUND;
				}
			} else {
				if ((damtype = GET_OBJ_VAL(weapon, 2)) != -1) {
					dam += GET_OBJ_VAL(weapon, 1);
					message = GET_OBJ_VAL(weapon, 3) + TYPE_HIT;
				} else {
					skillnum = PROFICIENCY_MACE;	// A generic object, use it as
					mod -= 4;			// a mace with penalties.
					message = TYPE_POUND;
				}
			}
			
			speed = GET_OBJ_VAL(weapon, 7);

			if (speed > 3)
				speed = MAX(speed - speed_bonus(GET_STR(ch)), 4);

			// Adjustments for defensive weapon maneuvers

			if (speed > 4) {
				speed = MAX(4, speed - speed_bonus(GET_STR(ch)));
			}

			if (is_parry) {
				// We used to offer a plus for fencing weapons, but that was designed
	// for a system which only had one attack per round.	Fencing weapons
	// are fast and parry well, so what's the fuss?
	if ((skillnum == PROFICIENCY_STAFF) ||
			(skillnum == PROFICIENCY_POLEARM))
		chance *= 2;

				chance /= 3;
	
	if (speed > 5)
		speed = 5;	
			}

		} else {
			dam -= 4;
			skillnum = PROFICIENCY_MACE;		// A generic object, use it as
			mod -= 4;								// a mace with penalties.
			message = TYPE_POUND;
			speed = 8;
		}
	} else {
		if (IS_WEAPONSKILL(skillnum)) {
			skillnum = STAT_DEX;			// No weapon, we're fighting barehanded.
			dam /= 3;
			speed = combat_moves[Combat::Punch].speed;
			message = TYPE_HIT;
		}
		return (0);
	}

	// We MAX this to make sure TYPE_UNDEFINED is set to TYPE_HIT.
	message = MAX(message, TYPE_HIT); 

	return (1); // We have a weapon!

}


// Daniel Rollings AKA Daniel Houghton's new combat profile

#define POSTURE_NORMAL		0
#define POSTURE_ALLOUT_DEFENSE	1
#define POSTURE_ALLOUT_OFFENSE	2
#define POSTURE_CAUTIOUS	3
#define POSTURE_OFFENSE		4

#define IS_OFFENSIVE(i) ((combat_moves[(i)].type == COMBAT_OFFENSIVE) || \
			(combat_moves[(i)].type == COMBAT_BOTH))
#define IS_DEFENSIVE(i) ((combat_moves[(i)].type == COMBAT_DEFENSIVE) || \
			(combat_moves[(i)].type == COMBAT_BOTH))

#define GET_COMBAT_MOVE(ch, i) ((ch)->combat_profile.GetMove(i))


// Constructor
CombatProfile::CombatProfile()
{
	int i;

	attack_offset = 0;
	defense_offset = 0;
	posture = POSTURE_NORMAL;

	moves[0] = Combat::Hit;
	moves[1] = Combat::AutoDefend;

	for (i = 2; i <= COMBAT_PROFILE_SIZE; ++i)
		moves[i] = Combat::Undefined; // Undefined

}


// Destructor
CombatProfile::~CombatProfile()
{
	// What to do?
}


// Grabs a pertinent offensive move from the profile
int CombatProfile::GetMove(UInt8 offset)
{
	if ((offset > 0) && (offset <= COMBAT_PROFILE_SIZE + 1)) {
		return moves[offset - 1];
	}

	return 0;
}


// Sets a move at an offset to a number correlating to a given string
// as per the combat_moves struct.	
int CombatProfile::SetMove(SInt16 & move, SInt8 & offset)
{
	if ((offset > 0) && (offset <= COMBAT_PROFILE_SIZE + 1)) {
		moves[offset - 1] = move;
		
		if (attack_offset == (offset - 1))
			NextOffense();
		if (defense_offset == (offset - 1))
			NextDefense();
		
		return 1;
	} else
		return 0;
}


inline void CombatProfile::NextMove(SInt8 & move_offset)
{
	// Set to next move.
	if (move_offset < COMBAT_PROFILE_SIZE)
		++move_offset;
	else
		move_offset = 0;
}


// Set the character up for their next attack.
void CombatProfile::NextOffense()
{
	UInt8 i = 0;

	do {
		NextMove(attack_offset);
		++i;
	} while ((!IS_OFFENSIVE(moves[attack_offset])) && (i <= COMBAT_PROFILE_SIZE + 1));
			
	if (i > COMBAT_PROFILE_SIZE + 1)		// Didn't find a valid move.
		attack_offset = -1;	
			
#ifdef TLO_DEBUG_MESSAGES
	sprintf(buf3, "Grabbing attack offset %d: %s Type: %d", attack_offset,
			combat_moves[moves[attack_offset]].move,
			combat_moves[moves[attack_offset]].type);
	log(buf3);
#endif

}


// Set the character up for their next defense.
void CombatProfile::NextDefense()
{
	UInt8 i = 0;

	do {
		NextMove(defense_offset);
		++i;
	} while ((!IS_DEFENSIVE(moves[defense_offset])) && (i <= COMBAT_PROFILE_SIZE + 1));
			
	if (i > COMBAT_PROFILE_SIZE + 1)		// Didn't find a valid move.
		defense_offset = -1;	
			
#ifdef TLO_DEBUG_MESSAGES
	sprintf(buf3, "Grabbing attack offset %d: %s Type: %d", defense_offset,
							combat_moves[moves[defense_offset]].move,
							combat_moves[moves[defense_offset]].type);
	log(buf3);
#endif
	
}


// Perform a move of an offensive/defensive nature
// Returns 1 if a move is performed, 0 if combat_pts do not allow any action.
int CombatProfile::CombatMove(SInt8 type)
{
	SInt8 offset = -1;

	// Return 0 on any invalid move type (i.e. COMBAT_BOTH)
	// or move type with no offset (hence, no valid move)
	switch (type) {
	case COMBAT_OFFENSIVE:
		if ((posture == POSTURE_ALLOUT_DEFENSE) || (attack_pts <= 0))
			return 0;
		NextOffense();
		offset = attack_offset;
		break;
	case COMBAT_DEFENSIVE:
		if ((posture == POSTURE_ALLOUT_OFFENSE) || (defense_pts <= 0))
			return 0;
		NextDefense();
		offset = defense_offset;
		break;
	default:
		return 0;
	}

	if (offset < 0)
		return 0;

	return moves[offset];

}


void CombatProfile::CombatCost(Character *ch, SInt8 type, SInt16 cost)
{
	if (cost == 0)
		return;

	switch (type) {
	case COMBAT_OFFENSIVE:
		if (attack_pts >= 0) {
			attack_pts = RANGE(-120, attack_pts - cost, 20);
		} else {
			cost /= 2;
			defense_pts = RANGE(-120, defense_pts - cost, 20);
			attack_pts = RANGE(-120, attack_pts - cost, 20);
		}
		break;
	case COMBAT_DEFENSIVE:
		if (defense_pts >= 0) {
			defense_pts = RANGE(-120, defense_pts - cost, 20);
		} else {
			cost /= 2;
			defense_pts = RANGE(-120, defense_pts - cost, 20);
			attack_pts = RANGE(-120, attack_pts - cost, 20);
		}
		break;
	default:
		cost /= 2;
		defense_pts = RANGE(-120, defense_pts - cost, 20);
		attack_pts = RANGE(-120, attack_pts - cost, 20);
	}
    
	if (cost)	AlterMove(ch, MAX(1, cost / 10));
}


void CombatProfile::ZeroPoints(SInt8 type = COMBAT_BOTH)
{
  // Return 0 on any invalid move type (i.e. COMBAT_BOTH)
  // or move type with no offset (hence, no valid move)
  switch (type) {
  case COMBAT_OFFENSIVE:
    attack_pts = 0;
  case COMBAT_DEFENSIVE:
    defense_pts = 0;
  default:
    attack_pts = 0;
    defense_pts = 0;
  }
}


// Distribute points to increment attack and defense values.
// This default routine has built-in values for the point update.
void CombatProfile::PointUpdate(Character * ch)
{
	SInt8 increase;

	// What if they've been knocked out?
	if ((IS_NPC(ch) ? GET_MOB_WAIT(ch) : CHECK_WAIT(ch)) && 
		 (GET_POS(ch) < POS_STANDING)) {
		return;
	}

	// increase = (GET_DEX(ch) + GET_STR(ch) + 8) / 3;
	increase = (5 + (GET_DEX(ch) / 2)) / 2;

	increase = MAX(increase - GET_ENCUMBRANCE(ch, IS_CARRYING_W(ch)), 0);
	// Daniel Rollings AKA Daniel Houghton's haste support
	if (AFF_FLAGGED(ch, AffectBit::Haste))
		increase *= 2;
	if (ADVANTAGED(ch, Advantages::CombatReflexes))
		increase += 3;
	if (GET_MOVE(ch) < 20) {		 // Are they tired?
		increase /= 2;
		if (!Number(0, 2))
			ch->Send("Your body feels heavy with fatigue!\r\n");
	}

	if (AFF_FLAGGED(ch, AffectBit::Berserk)) {
		increase += 6;
		attack_pts = RANGE(-120, attack_pts + increase, 20);
	} else {
		switch (posture) {
		case POSTURE_ALLOUT_DEFENSE:
			defense_pts = RANGE(-120, defense_pts + increase, 20);
			break;
		case POSTURE_ALLOUT_OFFENSE:
			attack_pts = RANGE(-120, attack_pts + increase, 20);
			break;
		case POSTURE_CAUTIOUS:
			defense_pts = RANGE(-120, defense_pts + ((increase * 3) / 4), 20);
			attack_pts = RANGE(-120, attack_pts + (increase / 4), 20);
			break;
		case POSTURE_OFFENSE:
			defense_pts = RANGE(-120, defense_pts + (increase / 4), 20);
			attack_pts = RANGE(-120, attack_pts + ((increase * 3) / 4), 20);
			break;
		default:
			increase /= 2;
			defense_pts = RANGE(-120, defense_pts + increase, 20);
			attack_pts = RANGE(-120, attack_pts + increase, 20);
		}
	}
}


// Distribute points to increment attack and defense values.
void CombatProfile::PointUpdate(int increase)
{
	int i;

	switch (posture) {
	case POSTURE_ALLOUT_DEFENSE:
		defense_pts = RANGE(-120, defense_pts + increase, 20);
		break;
	case POSTURE_ALLOUT_OFFENSE:
		attack_pts = RANGE(-120, attack_pts + increase, 20);
		break;
	default:
		increase /= 2;
		defense_pts = RANGE(-120, defense_pts + increase, 20);
		attack_pts = RANGE(-120, attack_pts + increase, 20);
	}
}


int CombatProfile::SetPosture(char *string)
{
  int temp_pos = -1;

  temp_pos = search_block(string, combat_postures, FALSE);

  if (temp_pos >= 0)
    posture = temp_pos;
  
  return (temp_pos);
}


void CombatProfile::ProfileDisplay(Character * ch)
{
  int i;

  ch->Sendf("&cY&br    Combat Profile    &c0&b0\r\n\r\n");

  ch->Sendf(" Posture: %-10.10s\r\n"
                    " Offense Points: %d\r\n"
                    " Defense Points: %d\r\n\r\n", 
                combat_postures[posture], attack_pts, defense_pts);
  // ch->Sendf(" Posture: %-10.10s\r\n\r\n", combat_postures[posture]);
  
  for (i = 0; i <= COMBAT_PROFILE_SIZE; ++i) {
    ch->Sendf("&cR%d&cr)  %s%-18.18s&c0\r\n", i + 1, 
                  moves[i] ? (IS_DEFENSIVE(moves[i]) ? "&cY" : "&cW") : "&cL", 
                  combat_moves[moves[i]].move);
  }

}


void CombatProfile::Read(char *string) 
{
	int i = 0;

	half_chop(string, buf2, buf);

	while (*buf2) {
	    moves[i] = atoi(buf2);
        ++i;
		half_chop(buf, buf2, buf);
    }
}


void CombatProfile::Save(FILE *outfile)
{
	*buf = '\0';

	for (int i = 0; i <= COMBAT_PROFILE_SIZE; ++i) {
    	sprintf(buf + strlen(buf), 
        	"%d%s", 
            moves[i], (i == COMBAT_PROFILE_SIZE) ? "" : " ");
	}
    fprintf(outfile, buf);
}


// Usage:	combat <slot> <'skill'>
ACMD(do_combatprofile)
{
	int i = 0, j = 0;
	SInt16 move = -1;
	UInt8 valid_arg = FALSE;
	SInt8 offset = 0, posture;
	char field[MAX_INPUT_LENGTH], field2[MAX_INPUT_LENGTH];

	half_chop(argument, field, field2);

	if ((!*field) && (!*field2)) {
		ch->combat_profile.ProfileDisplay(ch);
		return;
	}

	if (is_abbrev(field, "posture")) {
		if ((posture = (SInt8) ch->combat_profile.SetPosture(field2)) == -1) {
			ch->Send("Valid postures are evasive, defensive, normal, offensive, and allout.\r\n");
		} else {
			ch->Sendf("Posture set to %s.\r\n", combat_postures[posture]);
		}
		return;
	} else if (!*field2) {
		*buf2 = '\0';
		strcat(buf2, "Valid combat moves:\r\n");
		for (i = 1; *(combat_moves[i].move) != '\n'; ++i) {
			if (combat_move_chance(ch, i) != SKILL_NOCHANCE)
				sprintf(buf2, "%s%-23.23s %s", buf2, combat_moves[i].move, 
								(++j % 3 ? " " : "\r\n"));
		}
		strcat(buf2, "\r\n");
		ch->Send(buf2);
		return;
	} else if (isdigit(*field)) {
		offset = atoi(field);
		valid_arg = TRUE;
	}

	// Locate the move in the combat_moves struct.
	for (i = 0; (*(combat_moves[i].move) != '\n') && (move == -1); i++) {
		if (is_abbrev(field2, combat_moves[i].move))
			move = i;
	}

	// Did they specify a valid move?
	if (move >= 0) {
		ch->combat_profile.SetMove(move, offset);
		ch->Sendf("Combat slot %d set to %s!\r\n", offset, 
									combat_moves[move].move);
	} else {
		valid_arg = FALSE;
		ch->Sendf("That's not a valid move!\r\n");
		return;
	}
		
	if (!valid_arg) {	// They didn't specify a valid slot and move.
		ch->Send("Usage:	combat <slot> <move>\r\nTo see a list of moves, type \"combat moves\".\r\n");
		ch->combat_profile.ProfileDisplay(ch);
	}

}


// This routine checks to see if two characters, at least one of whom would 
// have to be in a group, can engage without intervention from other group
// members.
int in_melee_range(Character * ch, Character * victim, int attacktype)
{
	int ranged = 1;
	
	Character *k, *f;
	
	// Are they in the rear?	
	if (GROUP_POS(ch)) {
		if (!(k = ch->master))
			k = ch;

		if ((IN_ROOM(k) == IN_ROOM(ch)) && (!GROUP_POS(k))) {
			++ranged;
		} else {
			START_ITER(iter, f, Character *, k->followers) {
				if ((IN_ROOM(f) == IN_ROOM(ch)) && (!GROUP_POS(f))) {
					++ranged;
					break;
				}
			}
		}
	}
	
	// Is the target in the rear?
	if (GROUP_POS(victim)) {
		if (!(k = victim->master))
			k = ch;

		if ((IN_ROOM(k) == IN_ROOM(victim)) && (!GROUP_POS(k))) {
			++ranged;
		} else {
			START_ITER(iter, f, Character *, k->followers) {
				if ((IN_ROOM(f) == IN_ROOM(victim)) && (!GROUP_POS(f))) {
					++ranged;
					break;
				}
			}
		}
	}
	
	if (attacktype == PROFICIENCY_POLEARM)
		--ranged;
	
	if (ranged <= 1)
		return 1;

	return 0;
}


Character * check_for_intercept(Character * ch, Character * victim, Attack *attack)
{
	Character *f = NULL, *k = NULL, *interceptor = NULL;

	if (in_melee_range(ch, victim, attack->skillnum) || !AFF_FLAGGED(victim, AffectBit::Group))
		return victim;	// They're in immediate range!

	// Wait, so they're not in range.	Check for intervention from group members.
	if (victim->master)
		k = victim->master;
	else
		k = victim;
		
	if (AFF_FLAGGED(k, AffectBit::Group) && 
		 (IN_ROOM(k) == IN_ROOM(ch)) &&
		 ((!FIGHTING(k)) || (FIGHTING(k) == ch) || 
			(SKILLROLL(k, SKILL_COMBAT_TECHNIQUE) >= 0)) && 
		 (!GROUP_POS(k)))
		interceptor = k;

	START_ITER(iter, f, Character *, k->followers) {
		if (AFF_FLAGGED(f, AffectBit::Group) && (!AFF_FLAGGED(f, AffectBit::Immobilized)) &&
			 (IN_ROOM(f) == IN_ROOM(ch)) &&
			 ((!FIGHTING(f)) || (FIGHTING(f) == ch) ||
			 (SKILLROLL(f, SKILL_COMBAT_TECHNIQUE) >= 0)) && 
			 (!GROUP_POS(f)))
			interceptor = f;
	}
	
	if (interceptor) {
		if (FIGHTING(interceptor) == ch) {
			act("&c+&cW$N blocks your attack!&c-", 0, ch, 0, interceptor, TO_CHAR);
			act("&c+&cWYou block $n's attack!&c-", 0, ch, 0, interceptor, TO_VICT);
		} else {
			act("&c+&cW$N blocks your attack and engages you!&c-", 0, ch, 0, interceptor, TO_CHAR);
			act("&c+&cWYou block $n's attack and engage $m!&c-", 0, ch, 0, interceptor, TO_VICT);
			act("&c+&cW$N diverts $n's attack and engages $m!&c-", FALSE, ch, 0, interceptor, TO_NOTVICT);
		}
		if (FIGHTING(ch) && (FIGHTING(ch) == victim))
			ch->StopFighting();
		Combat::CheckFighting(interceptor, ch);
		return interceptor;
	} else {
		return victim;
	}
	
}


// For perform_violence and other routines - executes a round of violence!
void combat_round(Character *ch, Character *victim)
{
	SInt16	attack_move = 0;
	SInt8	arms[4],
			i,
			combat_tech_roll,
			highest_pts = 0,
			tmp_attack_pts = 0,	// Temporary storage for attack
			pts_this_round = 0,	// Calculation of move cost
			attacks_done = 0,	// Number of attacks performed
			number_attacks = 1;	// Our cap-off of the attacks per round.
	bool	did_attack = false;	// Used to test whether defense spells should hit.
	SInt8 *arm_pts = NULL;

	if (victim == NULL)
		return;

	if (AFF_FLAGGED(ch, AffectBit::Immobilized))
		return;

	if (GET_ENCUMBRANCE(ch, IS_CARRYING_W(ch)) > 5) { // This message may not always be appropriate
		ch->Send("You are too encumbered to fight!\r\n");
		return;
	}

	if ((GET_MOVE(ch) < 5) && (GET_POS(ch) >= POS_RESTING) && !AFF_FLAGGED(ch, AffectBit::Berserk)) {
		ch->Send("Your body aches with fatigue!\r\n");
	}

	arms[0] = 0;
	arms[1] = 0;
	arms[2] = 0;
	arms[3] = 0;

	// Get moves from combat profiles - an offset for the combat_profile_moves 
	attack_move = ch->combat_profile.CombatMove(COMBAT_OFFENSIVE);

	while ((attack_move > 0) && (IN_ROOM(ch) == (IN_ROOM(victim)))) {

#if 0
		sprintf(buf, "%s doing combat move %d (%s)", ch->Name(), attack_move, combat_moves[attack_move].move);
		ch->Inside()->Echo(buf);
#endif

		// Daniel Rollings AKA Daniel Houghton's dual-wielding support.	The theory behind this is
		// that if people wield dual weapons and succeed in their skillrolls, 
		// we'll record how many points were spent on each arm and deduct the
		// highest value, not the sum of all costs.

		if (attacks_done > 0)
			ch->combat_profile.attack_pts = RANGE(-120, ch->combat_profile.attack_pts - attacks_done, 120);

		tmp_attack_pts = ch->combat_profile.attack_pts;

		// The subroutine should test for the -1 cmd variable
		if (*combat_moves[attack_move].command_pointer != NULL) {

			(*combat_moves[attack_move].command_pointer) // Perform the attack!
			(ch, "", -1, "", combat_moves[attack_move].subcmd);
			did_attack = true;
		}

		if (IS_WEAPONSKILL(combat_moves[attack_move].skillnum)) {
			// Find out how many attack points we spent this round.
			pts_this_round = tmp_attack_pts - ch->combat_profile.attack_pts;

			switch (combat_moves[attack_move].skillnum) {
			case SKILL_WEAPON_R: arm_pts = &arms[0]; break;
			case SKILL_WEAPON_L: arm_pts = &arms[1]; break;
			case SKILL_WEAPON_3: arm_pts = &arms[2]; break;
			case SKILL_WEAPON_4: arm_pts = &arms[3]; break;
			}

			// This isn't the most any one arm has done?
			if ((pts_this_round + *arm_pts) <= highest_pts) {
				if (SKILLROLL(ch, SKILL_DUAL_WIELD) >= 0) {
					// Reimburse part that was not in excess of previous high.
					ch->combat_profile.attack_pts = RANGE(-120, ch->combat_profile.attack_pts + pts_this_round, 120);
				}
			} else { // We've just superceded the previous high
				if ((highest_pts > 0) && (SKILLROLL(ch, SKILL_DUAL_WIELD) >= 0)) {
					ch->combat_profile.attack_pts = RANGE(-120, ch->combat_profile.attack_pts + highest_pts - *arm_pts, 120);
				}
			}

			*arm_pts += pts_this_round;
			if (*arm_pts > highest_pts)
				highest_pts = *arm_pts;
		}

		++attacks_done;

		// Here is Nestor's suggested cumulative cost for multiple attacks.
		if (attacks_done > 1)		ch->combat_profile.CombatCost(ch, COMBAT_OFFENSIVE, attacks_done - 1);

		if (!PURGED(ch) && !PURGED(victim)) {
			attack_move = ch->combat_profile.CombatMove(COMBAT_OFFENSIVE);
		} else {
			attack_move = 0;	// They're dead or we're out of attacks!	We can stop now!
		}
	}

	/* Daniel Rollings AKA Daniel Houghton's new code:	support for defensive spells */
	if (did_attack && (IN_ROOM(ch) == IN_ROOM(victim))) {
		CHAR_SKILL(ch).RollToLearn(ch, SKILL_COMBAT_TECHNIQUE, dice(3, 6));
	
		if (AFF_FLAGGED(victim, AffectBit::BladeBarrier) && (Number(0, 1)))
			mag_damage(15, victim, ch, SPELL_BLADE_BARRIER);
		if ((IN_ROOM(ch) == IN_ROOM(victim)) &&
			AFF_FLAGGED(victim, AffectBit::FireShield) && (Number(0, 1)))
			mag_damage(15, victim, ch, SPELL_FIRE_SHIELD);
	}

	COMBAT_PT_UPDATE(ch);
}


/* control the fights going on.	Called every 2 seconds from comm.c. */
void perform_violence(void)
{
	void fight_mtrigger(Character *ch);
	void hitprcnt_mtrigger(Character *ch);
	void mob_combat_action(Character *ch);

	Character *ch;

	START_ITER(iter, ch, Character *, CombatList) {
		if (FIGHTING(ch) == NULL || PURGED(FIGHTING(ch)) || 
		   (IN_ROOM(ch) != IN_ROOM(FIGHTING(ch))) || GET_POS(ch) < POS_SLEEPING) {
			ch->StopFighting();
			continue;
		}

		if (AFF_FLAGGED(ch, AffectBit::Immobilized))
			continue;

		if (IS_NPC(ch)) {
			if (GET_MOB_WAIT(ch) > 0)
				GET_MOB_WAIT(ch) -= PULSE_VIOLENCE;
			else
				GET_MOB_WAIT(ch) = 0;
		}
		 
		if (GET_POS(ch) < POS_STANDING) {
			if (IS_NPC(ch) ? !GET_MOB_WAIT(ch) : !CHECK_WAIT(ch)) {
				GET_POS(ch) = POS_STANDING;
				act("You scramble to your feet!", FALSE, ch, 0, 0, TO_CHAR);
				act("$n scrambles to $s feet!", TRUE, ch, 0, 0, TO_ROOM);
			}
		} else {

			combat_round(ch, FIGHTING(ch));

			if (IS_NPC(ch) && !GET_MOB_WAIT(ch) && FIGHTING(ch)) {
				mob_combat_action(ch);
			}

				fight_mtrigger(ch);
			if (!PURGED(ch))
				hitprcnt_mtrigger(ch);
			
			ch->UpdatePos();

			if (FIGHTING(ch) && (!PURGED(FIGHTING(ch))) && 
				GET_HIT(FIGHTING(ch)) < (GET_MAX_HIT(FIGHTING(ch)) / 4)) {
				strcpy(buf2, "&crYou wish that your wounds would stop BLEEDING so much!&c0\r\n");
				FIGHTING(ch)->Send(buf2);
			}
			if (MOB_FLAGGED(ch, MOB_SPEC) && Character::Index[ch->Virtual()].func)
				(Character::Index[ch->Virtual()].func) (ch, ch, 0, "");
		}
	}
}


int Ranged::FireMissile(Character * ch, Character * victim, Attack * attack, int dir)
{
	char locationname[40];
	extern const char *thedirs[];
	int damage_resistance = 0;	// How much damage is blocked by armor.
	char hittochar[MAX_INPUT_LENGTH], hittoroom[MAX_INPUT_LENGTH];
	char misstochar[MAX_INPUT_LENGTH], misstoroom[MAX_INPUT_LENGTH];
	bool reaction = TRUE;
		
	*hittochar = '\0';
	*hittoroom = '\0';
	*misstochar = '\0';
	*misstoroom = '\0';

//	// Set up the action descriptions
//	if (attack->weapon && attack->weapon->action_description) {
//		adesc_lines(ss_data(attack->weapon->action_description), hittochar, hittoroom, misstochar, misstoroom);
//	}
		
	if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
		ch->Send("This room just has such a peaceful, easy feeling...\r\n");
		return 0;
	}

	/* Daniel Rollings AKA Daniel Houghton's missile modification */
	if ROOM_FLAGGED(IN_ROOM(victim), ROOM_PEACEFUL) {
		ch->Send("Nah.	Leave them in peace.\r\n");
		return 0;
	}

	attack->succeed_by = CHAR_SKILL(ch).RollSkill(ch, attack->skillnum, attack->chance) + attack->mod;
	CHAR_SKILL(ch).RollToLearn(ch, attack->skillnum, attack->succeed_by);
				
	if ((!MOB_FLAGGED(victim, MOB_NOSHOOT)) &&
			(!AFF_FLAGGED(victim, AffectBit::Sanctuary)) && 
			(attack->succeed_by >= 0)) {

		attack->location = find_hit_location(victim, 0);
		strcpy(locationname, GET_WEARSLOT(attack->location, GET_BODYTYPE(victim)).partname);

		if (GET_EQ(victim, attack->location) &&
			 ((GET_OBJ_TYPE(GET_EQ(victim, attack->location)) == ITEM_ARMOR) ||
				(GET_OBJ_TYPE(GET_EQ(victim, attack->location)) == ITEM_CONTAINER) ||
				(GET_OBJ_TYPE(GET_EQ(victim, attack->location)) == ITEM_WEAPONCON))) {
			damage_resistance = GET_DR(GET_EQ(victim, attack->location));
		} else {
			attack->dam = MAX(attack->dam, 1);
		}
		damage_resistance += GET_DR(victim);

		sprintf(buf, "Your $o strikes $M in the %s!", locationname);
		victim->Send("&cW");
		act(buf, FALSE, ch, attack->weapon, victim, TO_CHAR);
		victim->Send("&c0");

		if (dir >= 0) {
			if (!*hittochar) {
				sprintf(hittochar, "$n's $o flies in from %s, striking you in the %s!", 
								thedirs[rev_dir[dir]], locationname);
			}
			victim->Send("&cR");
			act(hittochar, FALSE, ch, attack->weapon, victim, TO_VICT);
			victim->Send("&c0");

			if (!*hittoroom) {
				sprintf(hittoroom, "$n's $o flies in from %s, striking $N in the %s.", 
								thedirs[rev_dir[dir]], locationname);
			}
			act(hittoroom, FALSE, ch, attack->weapon, victim, TO_NOTVICT);
		} else {
			if (!*hittochar) {
				sprintf(hittochar, "$n's $o strikes you in the %s!", locationname);
			}
			victim->Send("&cR");
			act(hittochar, FALSE, ch, attack->weapon, victim, TO_VICT);
			victim->Send("&c0");

			if (!*hittoroom) {
				sprintf(hittoroom, "$n's $o strikes $N in the %s!", locationname);
			}
			act(hittoroom, FALSE, ch, attack->weapon, victim, TO_NOTVICT);
		}

		if (ok_damage_shopkeeper(ch, victim)) {
			if (attack->dam - damage_resistance > 0) {
				attack->dam -= damage_resistance;
				reaction = FALSE;
				victim->Damage(ch, attack);
			} else {
				ch->Send("&cY");
				victim->Send("&cB");
				armor_message(0, ch, victim, attack->skillnum, attack->location);
				ch->Send("&c0");
				victim->Send("&c0");
			}
		}
		
	} else {
		attack->succeed_by = -1;	// Just to be sure, in case we failed due to sanctuary.
		
		victim->Send("&cW");
		act("Your $o goes wide!", FALSE, ch, attack->weapon, victim, TO_CHAR);
		victim->Send("&c0");

		if (dir >= 0) {
			if (!*misstochar) {
				sprintf(misstochar, "$n's $o flies in from %s, just missing you!", thedirs[rev_dir[dir]]);
			}
			victim->Send("&cR");
			act(misstochar, FALSE, ch, attack->weapon, victim, TO_VICT);
			victim->Send("&c0");

			if (!*misstoroom) {
				sprintf(misstoroom, "$n's $o flies in from %s, just missing $N!", thedirs[rev_dir[dir]]);
			}

			act(misstoroom, FALSE, ch, attack->weapon, victim, TO_NOTVICT);

		} else {
			if (!*misstochar) {
				strcpy(misstochar, "$n's $o flies past you, missing you!");
			}
			victim->Send("&cR");
			act(misstochar, FALSE, ch, attack->weapon, victim, TO_VICT);
			victim->Send("&c0");

			if (!*misstoroom) {
				strcpy(misstoroom, "$n's $o flies past $N, missing $M!");
			}
			act(misstoroom, FALSE, ch, attack->weapon, victim, TO_NOTVICT);
		}
	}

	if (reaction) { // We didn't go through damage, which has its own reaction routine; do it here!
		if (IN_ROOM(ch) == IN_ROOM(victim) && in_melee_range(ch, victim, attack->skillnum))	{
			Combat::CheckFighting(ch, victim);
		} else {
			Combat::MobReaction(ch, victim, dir);
		}
	}

	WAIT_STATE(ch, PULSE_VIOLENCE);

	return 1;
}



/* Routine which checks a given direction and range to see if the victim
	 is an available ranged target - DH */
Character *	Ranged::GrabRangedTarget(Character *attacker, char *arg1, SInt32 dir, SInt32 range, SInt32 &exception) {
	int room, nextroom, distance = 1;
	Character *victim = NULL;
		
	room = IN_ROOM(attacker);

	if (CAN_VIEW(room, dir))	nextroom = EXITN(room, dir)->to_room;
	else						nextroom = NOWHERE;
	
	while (((room != nextroom) && (nextroom != NOWHERE) && (distance <= range))) {

		if (ROOM_FLAGGED(room, ROOM_NOMOB) || 
				ROOM_FLAGGED(nextroom, ROOM_NOMOB)) {
			attacker->Send("You can't find your aim!\r\n");
			return NULL;
		}

		if ((ROOM_AFFECTED(room, RAFF_FOG) || ROOM_AFFECTED(nextroom, RAFF_FOG)) &&
			 (!PRF_FLAGGED(attacker, Preference::HolyLight))) {
			attacker->Send("Your view is obscured by a thick fog.\r\n");
			 return NULL;
		}

		START_ITER(iter, victim, Character *, world[nextroom].people) {
			if (victim->CanBeSeen(attacker) && (isname(arg1, victim->CalledName(attacker)))) {
				return victim;
			}
		}

		room = nextroom;

		if (CAN_VIEW(room, dir))	nextroom = EXITN(room, dir)->to_room;
		else						nextroom = NOWHERE;
			
		++distance;
	}

	attacker->Send("Can't find your target within range!\r\n");
	exception = TRUE;
	return NULL;
}


Character * Ranged::AcquireTarget(Character * ch, char *argument, SInt32 &dir, SInt32 &exception) {
	char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
	Character *target = NULL;

	two_arguments(argument, arg1, arg2);
	dir = -1;

	if (!*arg1) {
		ch->Send("You should specify a target and a direction!\r\n");
		exception = TRUE;
		return NULL;
	}

	if (!*arg2) {
		target = get_char_vis(ch, arg1, FIND_CHAR_ROOM);
		return (target);
	} else {
		dir = search_block(arg2, dirs, FALSE);
		if (dir	== -1) {
			ch->Send("What direction?\r\n");
			return NULL;
		}

		if (!CAN_VIEW(IN_ROOM(ch), dir)) {
		ch->Send("Something blocks the way!\r\n");
			exception = 1;
			return NULL;
		}

		target = GrabRangedTarget(ch, arg1, dir, MAX_DISTANCE, exception);
	}

	return target;
}


/* Routine which checks a given direction and range to see if the victim
	 is an available ranged target - DH */
SInt32 Ranged::DistanceToTarget(Character * ch, Character * victim, SInt32 range, SInt32 dir)
{
	int room, nextroom, distance;
	
	if (dir < 0)
		return 1;	// Quick kludge to take care of same-room ranged attacks.

	if (victim == NULL)
		return 0;
		
	room = IN_ROOM(ch);

	if (CAN_VIEW(room, dir))	nextroom = EXITN(room, dir)->to_room;
	else						nextroom = NOWHERE;
	
	distance = 1; 

	while (((room != nextroom) && (nextroom != NOWHERE) && (distance <= range))) {

		if (ROOM_FLAGGED(room, ROOM_NOMOB) || 
				ROOM_FLAGGED(nextroom, ROOM_NOMOB)) {
			ch->Send("You can't find your aim!\r\n");
			return 0;
		}

		if ((ROOM_AFFECTED(room, RAFF_FOG) || ROOM_AFFECTED(nextroom, RAFF_FOG)) &&
			 (!PRF_FLAGGED(ch, Preference::HolyLight))) {
			 ch->Send("Your view is obscured by a thick fog.\r\n");
			 return 0;
	}

		// ch->Sendf("Checking room %d, range %d, distance %d, seeking %d\r\n", 
		//							 nextroom, range, distance, IN_ROOM(victim));

		if (nextroom == IN_ROOM(victim))	return distance;

		room = nextroom;

		if (CAN_VIEW(room, dir))	nextroom = EXITN(room, dir)->to_room;
		else 						nextroom = NOWHERE;
			
		++distance;
	}

	ch->Send("Can't find your target within range!\r\n");
	return 0;
}



Object * Ranged::GrabMissile(Character * ch, Object * weapon)
{
	int i;
	Object *quiver = NULL, *tmp_obj = NULL, *missile = NULL;

	for (i = 0; i < NUM_WEARS; ++i) {
		if (GET_EQ(ch, i) && CAN_SEE_OBJ(ch, GET_EQ(ch, i)) &&
			(GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_WEAPONCON) &&
			(GET_OBJ_VAL(GET_EQ(ch, i), 3) == GET_OBJ_VAL(weapon, 0)) &&
			(GET_EQ(ch, i)->contents.Count())) {
			quiver = GET_EQ(ch, i);
			break;
		}
	}

	if (!quiver) {
		START_ITER(iter, tmp_obj, Object *, ch->contents) {
			if (CAN_SEE_OBJ(ch, tmp_obj) && 
				(GET_OBJ_TYPE(tmp_obj) == ITEM_WEAPONCON) &&
				(GET_OBJ_VAL(tmp_obj, 3) == GET_OBJ_VAL(weapon, 0)) && 
				tmp_obj->contents.Count()) {
					quiver = tmp_obj;
					break;
			}
		}
	}

	if (quiver) {
		missile = quiver->contents.Top();
		missile->FromObj();
		missile->ToChar(ch);
	}

	return missile;
}
