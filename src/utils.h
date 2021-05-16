/* ************************************************************************
*   File: utils.h                                       Part of CircleMUD *
*  Usage: header file: utility macros and prototypes of utility funcs     *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

/* external declarations and prototypes **********************************/
#ifndef __UTILS_H__
#define __UTILS_H__

#include "types.h"
#include "strings.h"
#include "memory.h"

#undef log

#define mudlog(a,b,c,d)	mudlogf(b,c,d,a)

class Room;
class Character;
class Descriptor;

/* public functions in utils.c */
int		touch(const char *path);
void	log_death_trap(Character *ch);
int		Number(int from, int to);
int		dice(int number, int size);
int		get_line(FILE *fl, char *buf);
int		get_filename(const char *orig_name, char *filename);
struct TimeInfoData age(Character *ch);
int		num_pc_in_room(Room *room);
int		replace_str(char **string, const char *pattern, const char *replacement, int rep_all, StrLenInt max_size);

void	format_text(char **ptr_string, int mode, Descriptor * d,
			StrLenInt maxlen, unsigned int rmargin = 78);

void	core_dump_func(const char *who, SInt16 line);

extern void adesc_lines(char *fromstring, char *line1, char *line2 = NULL,
                 char *line3 = NULL, char *line4 = NULL);

extern void garble_text(char * origtext, char * newtext, struct syllable *syllables,
                 StrLenInt len, SInt8 howmuch);
extern bool same_alignment(Character *ch, Character *vict);
extern bool opposed_alignment(Character *ch, Character *vict);


#define core_dump()	core_dump_func(__FUNCTION__, __LINE__)

/* random functions in random.c */
void circle_srandom(unsigned int initial_seed);
UInt32 circle_random(void);


// limits.cc

extern void AlterHit(Character *ch, SInt32 amount);
extern void AlterMana(Character *ch, SInt32 amount);
extern void AlterMove(Character *ch, SInt32 amount);

/* in log.c */
/*
 * The attribute specifies that mudlogf() is formatted just like printf() is.
 * 4,5 means the format string is the fourth parameter and start checking
 * the fifth parameter for the types in the format string.  Not certain
 * if this is a GNU extension so if you want to try it, put #if 1 below.
 */
void	log(const char *format, ...) __attribute__ ((format (printf, 1, 2)));

void    mudlogf(SInt8 type, Character *ch, bool file, const char *format, ...) __attribute__ ((format (printf, 4, 5)));


/* undefine MAX and MIN so that our functions are used instead */
#ifdef MAX
#undef MAX
#endif

#ifdef MIN
#undef MIN
#endif

// This stuff just makes things harder than they need to be, IMHO.
// template<class T> const T& MAX(const T& a, const T& b) {
// 	return (a > b) ? a : b;
// }
// template<class T> const T& MIN(const T& a, const T& b) {
// 	return (a < b) ? a : b;
// }
// template<class T> const T& RANGE(const T& low, const T& a, const T& high) {
// 	return (low > a) ? low : ((a > high) ? high : a);
// }

inline SInt32 MAX(SInt32 a, SInt32 b)		{ return a > b ? a : b;			}
inline SInt32 MIN(SInt32 a, SInt32 b)		{ return a < b ? a : b;			}
inline SInt32 RANGE(SInt32 low, SInt32 value, SInt32 high)
	{ return MAX(low, MIN(high, value)); }

/* in magic.c */
int	circle_follow(Character *ch, Character * victim);

/* in act.informative.c */
void	look_at_room(Character *ch, int mode);
void	look_at_rnum(Character *ch, VNum room, int mode);

/* in act.movmement.c */
int	do_simple_move(Character *ch, int dir, int following);
int	perform_move(Character *ch, int dir, int following);

/* in limits.c */
int	hit_gain(Character *ch);
int	mana_gain(Character *ch);
int	move_gain(Character *ch);
void	gain_condition(Character *ch, int condition, int value);
void	check_idling(Character *ch);
void	point_update(void);


/* various constants *****************************************************/


/* defines for mudlog() */
#define OFF	0
#define BRF	1
#define NRM	2
#define CMP	3

/* breadth-first searching */
#define BFS_ERROR			-1
#define BFS_ALREADY_THERE	-2
#define BFS_NO_PATH			-3

/* mud-life time */
#define SECS_PER_MUD_HOUR	90
#define SECS_PER_MUD_DAY	(24*SECS_PER_MUD_HOUR)
#define SECS_PER_MUD_MONTH	(30*SECS_PER_MUD_DAY)
#define SECS_PER_MUD_YEAR	(12*SECS_PER_MUD_MONTH)

#define PULSES_PER_MUD_HOUR     (SECS_PER_MUD_HOUR*PASSES_PER_SEC)

/* real-life time (remember Real Life?) */
#define SECS_PER_REAL_MIN	60
#define SECS_PER_REAL_HOUR	(60*SECS_PER_REAL_MIN)
#define SECS_PER_REAL_DAY	(24*SECS_PER_REAL_HOUR)
#define SECS_PER_REAL_YEAR	(365*SECS_PER_REAL_DAY)


/* string utils **********************************************************/


#define YESNO(a) ((a) ? "YES" : "NO")
#define ONOFF(a) ((a) ? "ON" : "OFF")

// IS_UPPER and IS_LOWER added by dkoepke
#define IS_UPPER(c) ((c) >= 'A' && (c) <= 'Z')
#define IS_LOWER(c) ((c) >= 'a' && (c) <= 'z')

#define LOWER(c)   (IS_UPPER(c) ? ((c)+('a'-'A')) : (c))
#define UPPER(c)   (IS_LOWER(c) ? ((c)+('A'-'a')) : (c))

#define ISNEWL(ch) ((ch) == '\n' || (ch) == '\r')
#define IF_STR(st) ((st) ? (st) : "\0")
#define CAP(st)  (*(st) = UPPER(*(st)), st)

#define AN(string) (strchr("aeiouAEIOU", *string) ? "an" : "a")

#define END_OF(buffer)		((buffer) + strlen((buffer)))


/* memory utils **********************************************************/


#if !defined(__STRING)
#define __STRING(x)	#x
#endif

#if BUFFER_MEMORY

#define CREATE(result, type, number)  do {\
	if (!((result) = (type *) debug_calloc ((number), sizeof(type), __STRING(result), __FUNCTION__, __LINE__)))\
		{ perror("malloc failure"); abort(); } } while(0)

#define RECREATE(result,type,number) do {\
	if (!((result) = (type *) debug_realloc ((result), sizeof(type) * (number), __STRING(result), __FUNCTION__, __LINE__)))\
		{ perror("realloc failure"); abort(); } } while(0)

#define free(variable)	debug_free((variable), __FUNCTION__, __LINE__)

#define str_dup(variable)	debug_str_dup((variable), __STRING(variable), __FUNCTION__, __LINE__)

#else

#define CREATE(result, type, number)  do {\
	if ((number * sizeof(type)) <= 0) {\
	mudlogf(BRF, NULL, TRUE, "CODEERR: Attempt to alloc 0 at %s:%d", __FUNCTION__, __LINE__);\
	}\
	if (!((result) = (type *) ALLOC ((number ? number : 1), sizeof(type))))\
		{ perror("malloc failure"); abort(); } } while(0)

#define RECREATE(result,type,number) do {\
  if (!((result) = (type *) realloc ((result), sizeof(type) * (number))))\
		{ perror("realloc failure"); abort(); } } while(0)

#endif
/*
 * the source previously used the same code in many places to remove an item
 * from a list: if it's the list head, change the head, else traverse the
 * list looking for the item before the one to be removed.  Now, we have a
 * macro to do this.  To use, just make sure that there is a variable 'temp'
 * declared as the same type as the list to be manipulated.  BTW, this is
 * a great application for C++ templates but, alas, this is not C++.  Maybe
 * CircleMUD 4.0 will be...
 */
#define REMOVE_FROM_LIST(item, head, next)	\
	if ((item) == (head))		\
		head = (item)->next;		\
	else {				\
		temp = head;			\
		while (temp && (temp->next != (item))) \
			temp = temp->next;		\
		if (temp)				\
			temp->next = (item)->next;	\
	}					\


/* basic bitvector utils *************************************************/


#define IS_SET(flag,bit)  ((flag) & (bit))
#define SET_BIT(var,bit)  ((var) |= (bit))
#define REMOVE_BIT(var,bit)  ((var) &= ~(bit))
#define TOGGLE_BIT(var,bit) ((var) = (var) ^ (bit))

#define DESC_ORIG(ch)	((IS_NPC(ch) && (ch)->desc) ? (ch)->desc->Original() : (ch))

#define MOB_FLAGS(ch)	((ch)->general.act)
#define PLR_FLAGS(ch)	((ch)->general.act)
#define PRF_FLAGS(ch)	(DESC_ORIG(ch)->player->special.preferences)
#define AFF_FLAGS(ch)	((ch)->affectbits)
#define CHN_FLAGS(ch)	(DESC_ORIG(ch)->player->special.channels)
#define ROOM_FLAGS(loc)	(world[(loc)].RoomFlags())
#define ROOM_AFFECTIONS(loc)    (world[(loc)].affectbits)
#define ROOM_AFFECTED(loc, aff) (IS_SET(ROOM_AFFECTIONS(loc), (aff)))

#define STF_FLAGS(ch)	((ch)->player->special.staff_flags)

#define IS_NPC(ch)		(IS_SET(MOB_FLAGS(ch), MOB_ISNPC))
#define IS_MOB(ch)		(IS_NPC(ch) && ((ch)->Virtual() >-1))

#define MOB_FLAGGED(ch, flag)		(IS_NPC(ch) && IS_SET(MOB_FLAGS(ch), (flag)))
#define PLR_FLAGGED(ch, flag)		(!IS_NPC(ch) && IS_SET(PLR_FLAGS(ch), (flag)))
#define AFF_FLAGGED(ch, flag)		(IS_SET(AFF_FLAGS(ch), flag))
#define PRF_FLAGGED(ch, flag)		(!IS_NPC(ch) && IS_SET(PRF_FLAGS(ch), flag))
#define CHN_FLAGGED(ch, flag)		(!IS_NPC(ch) && IS_SET(CHN_FLAGS(ch), flag))
#define ROOM_FLAGGED(loc, flag)		(IS_SET(ROOM_FLAGS(loc), (flag)))
#define EXIT_FLAGGED(exit, flag)	(IS_SET((exit)->exit_info, (flag)))
#define OBJVAL_FLAGGED(obj, flag)	(IS_SET(GET_OBJ_VAL((obj), 1), (flag)))
#define OBJWEAR_FLAGGED(obj, flag)	(IS_SET((obj)->wear, (flag)))
#define OBJ_FLAGGED(obj, flag)		(IS_SET(GET_OBJ_EXTRA(obj), (flag)))
#define STF_FLAGGED(ch, flag)		(IS_STAFF(ch) && IS_SET(STF_FLAGS(ch), (flag)))

#define PLR_TOG_CHK(ch,flag)	((TOGGLE_BIT(PLR_FLAGS(ch), (flag))) & (flag))
#define PRF_TOG_CHK(ch,flag)	((TOGGLE_BIT(PRF_FLAGS(DESC_ORIG(ch)), (flag))) & (flag))


/* room utils ************************************************************/


#define SECT(room)	(world[(room)].SectorType())

#define IS_DARK(room)	(!world[room].Light() && \
						(ROOM_FLAGGED(room, ROOM_DARK) || \
						( ( SECT(room) != SECT_INSIDE && \
						SECT(room) != SECT_CITY && !ROOM_FLAGGED(room, ROOM_INDOORS)) && \
						(sunlight == SUN_SET || sunlight == SUN_DARK)) ) )

#define IS_LIGHT(room)  (!IS_DARK(room))

#define GET_ROOM_SPEC(room) ((room) >= 0 ? world[(room)].func : NULL)

#define REALM(room)		(zone_table[(room).zone].realm)


/* char utils ************************************************************/

#define IN_ROOM(o)			((o)->InRoom())
#define GET_WAS_IN(ch)		((ch)->was_in_room)
#define GET_AGE(ch)			(age(ch).year)

#define GET_NAME(thing)		((thing)->Name())
//#define GET_NAME(ch)
//	(IS_NPC(ch) ? SSData((ch)->general.shortDesc) : SSData((ch)->general.name))
//#define GET_NAME(ch)		((ch)->GetName())
// CHANGEPOINT:  This check should not be necessary
#define GET_TITLE(ch)		((ch)->general.title ? SSData((ch)->general.title) : "")
#define GET_LEVEL(ch)		((ch)->general.level)
#define GET_MAX_LEVEL(ch)	((ch)->player->special.max_level)
#define GET_ACTION(ch)		((ch)->action)

#define GET_TRUST(ch) \
		(ch->desc && ch->desc->original ? \
        ch->desc->original->player->special.trust : \
        ch->player->special.trust)
#define GET_STAFF_INVIS(ch) \
		(ch->desc && ch->desc->original ? \
        ch->desc->original->player->special.staff_invis : \
        ch->player->special.staff_invis)
#define IS_IMMPUN(ch)	(GET_TRUST(ch) == 1)

#define GET_PASSWD(ch)		((ch)->player->passwd)
#define GET_PROMPT(ch)		((ch)->player->prompt)
#define GET_PFILEPOS(ch)	((ch)->pfilepos)

#define CAN_SEE(ch, obj)	(obj)->CanBeSeen((ch))
#define CAN_SEE_OBJ(ch, obj)	(obj)->CanBeSeen((ch))

#define GET_OBJ_NUM(obj)	(obj->Virtual())

#define GET_WEIGHT(thing)		((thing)->weight)
#define GET_MATERIAL(thing)		((thing)->material)
#define GET_INVIS_LEV(thing)	((thing)->invis_level)

/*
 * I wonder if this definition of GET_REAL_LEVEL should be the definition
 * of GET_LEVEL?  JE
 */
#define GET_REAL_LEVEL(ch) \
	(ch->desc && ch->desc->original ? GET_LEVEL(ch->desc->original) : GET_LEVEL(ch))

#define GET_REAL_LEVEL(ch) \
	(ch->desc && ch->desc->original ? GET_LEVEL(ch->desc->original) : GET_LEVEL(ch))

#define GET_RACE(ch)		((ch)->general.race)
#define GET_CLASS(ch)		((ch)->general.clss)
#define GET_HEIGHT(ch)		((ch)->general.height)
#define GET_SEX(ch)			((ch)->general.sex)
#define GET_MORTALITY(ch)	((ch)->general.mortality)

#define GET_STAT(ch, stat)	((ch)->aff_abils.stat)
#define GET_STR(ch)			((ch)->aff_abils.Str)
#define GET_DEX(ch)			((ch)->aff_abils.Dex)
#define GET_INT(ch)			((ch)->aff_abils.Int)
#define GET_WIS(ch)			((ch)->aff_abils.Wis)
#define GET_CON(ch)			((ch)->aff_abils.Con)
#define GET_CHA(ch)			((ch)->aff_abils.Cha)

#define GET_EXP(ch)			((ch)->points.exp)
#define GET_EXP_MOD(ch)		((ch)->general.exp_modifier)
#define GET_MAX_RIDERS(ch)	((ch)->max_riders)
#define GET_DISPOSITION(ch)	((ch)->mob->disposition)
#define GET_ATTACKTYPE(ch)	((ch)->mob->attack_type)
#define RIDING(ch)			((ch)->Riding())
#define RIDER(ch)			((ch)->Rider())


#define GET_BODYTYPE(ch)		((ch)->general.bodytype)
#define GET_WEARSLOT(where, i)	(wear_slots[i][(where)])

#define GET_HIT(ch)			((ch)->points.hit)
#define GET_MAX_HIT(ch)		((ch)->points.max_hit)
#define GET_MANA(ch)		((ch)->points.mana)
#define GET_MAX_MANA(ch)	((ch)->points.max_mana)
#define GET_MOVE(ch)		((ch)->points.move)
#define GET_MAX_MOVE(ch)	((ch)->points.max_move)

#define GET_GOLD(ch)		((ch)->points.gold)
#define GET_BANK_GOLD(ch)	((ch)->points.bank_gold)
#define GET_HITROLL(ch)		((ch)->points.hitroll)
#define GET_DAMROLL(ch)		((ch)->points.damroll)
#define GET_PRACTICES(ch)	((ch)->points.pracs)

#define GET_POS(ch)			((ch)->general.position)
#define GET_STATE(ch)		((ch)->general.state)
#define GET_IDNUM(ch)		((ch)->player->idnum)
#define IS_CARRYING_W(ch)	((ch)->general.misc.carry_weight)
#define IS_CARRYING_N(ch)	((ch)->general.misc.carry_items)
#define GET_ENCUMBRANCE(ch, weight) (rate_encumbrance((ch), (weight)))
#define GET_MOVEMENT(ch)	(MAX((((GET_CON(ch) + GET_DEX(ch)) / 4) - \
							(GET_ENCUMBRANCE((ch), IS_CARRYING_W(ch)))), 0))
#define FIGHTING(ch)		((ch)->general.fighting)
#define STALKING(ch)		((ch)->general.stalking)
#define AIMING(ch)			((ch)->general.aiming)

#define GET_ALIGNMENT(ch)	((ch)->general.alignment)
#define GET_LAWFULNESS(ch)	((ch)->general.lawfulness)
#define GROUP_POS(ch)		((ch)->general.group_pos)

#define GET_NDD(ch)			((ch)->mob->damage.number)
#define GET_SDD(ch)			((ch)->mob->damage.size)
#define GET_ATTACK(ch)		((ch)->mob->attack_type)

#define GET_COND(ch, i)		((ch)->general.conditions[(i)])
#define GET_LOADROOM(ch)	((ch)->player->special.load_room)
#define GET_WIMP_LEV(ch)	((ch)->player->special.wimp_level)
#define GET_BAD_PWS(ch)		((ch)->player->special.bad_pws)
#define POOFIN(ch)			((ch)->player->poofin)
#define POOFOUT(ch)			((ch)->player->poofout)
#define GET_PAGE_LENGTH(ch)	((DESC_ORIG(ch))->player->special.page_length)
#define GET_SAYCOLOR(ch)	((ch)->player->special.saycolor)
#define GET_AFK(ch)			((ch)->player->afk)

#define AFFECTS(thing) ((thing)->affected)

#define ADV_FLAGS(ch)		((ch)->general.advantages)
#define DISADV_FLAGS(ch)	((ch)->general.disadvantages)

#define ADV_FLAGGED(ch, flag) (IS_SET(ADV_FLAGS(ch), (flag)))

#define ADVANTAGED(ch, flag) (IS_SET(ADV_FLAGS(ch), (flag)) || \
                              IS_SET(race_advantages[GET_RACE(ch)], (flag)))

#define DISADVANTAGED(ch, flag) (IS_SET(DISADV_FLAGS(ch), (flag)) || \
                                 IS_SET(race_disadvantages[GET_RACE(ch)], (flag)))

#define HAS_INFRAVISION(ch) (AFF_FLAGGED((ch), AffectBit::Infravision) || \
                             ADVANTAGED((ch), Advantages::Infravision))

#define GET_ALIASES(ch)		(DESC_ORIG(ch)->player->aliases)
#define GET_LAST_TELL(ch)	(DESC_ORIG(ch)->player->last_tell)

#define CHAR_SKILL(ch)		((ch)->general.skills)
#define GET_SKILL(ch, i)	(CHAR_SKILL(ch).GetSkillPts(i, NULL))
#define GET_SKILLTHEORY(ch, i)	(CHAR_SKILL(ch).GetSkillTheory(i, NULL))
#define SET_SKILL(ch, i, pts)	(CHAR_SKILL(ch).SetSkill(ch, i, NULL, pts))
#define LEARN_SKILL(ch, i, pts)	(GET_TOTALPTS(ch) += CHAR_SKILL(ch).LearnSkill(i, NULL, pts))
/* #define GET_SKILL(ch, i)	((ch)->player_specials->saved.skills[i]) */
/* #define SET_SKILL(ch, i, pct)	do { (ch)->player_specials->saved.skills[i] = pct; } while (0) */
#define FINDSKILL(ch, i, ptr)	(CHAR_SKILL(ch).FindSkill(i, &ptr))
#define SKILLCHANCE(ch, i, ptr)	(CHAR_SKILL(ch).GetSkillChance(ch, i, ptr, TRUE))
#define SKILLROLL(ch, i)	(CHAR_SKILL(ch).RollSkill(ch, i))
#define SKILLCONTEST(ch, i, cmod, vict, j, vmod, o, d)	(CHAR_SKILL(ch).SkillContest(ch, i, cmod, vict, j, vmod, o, d))

#define GET_WEAPONSKILL(ch, hand)	(get_weapon_skill((ch), (hand)))

#ifdef GOT_RID_OF_IT
#define GET_SKILL(ch, i)	((ch)->general.skills[i]->proficient)
#define SET_SKILL(ch, i, pct)	{ (ch)->general.skills[i] = pct; }
#endif

#define ENERGY(ch)			((ch)->player->MagicPool)

#define GET_EQ(ch, i)		((ch)->equipment[i])

#define GET_MOB_SPEC(ch)	(IS_MOB(ch) ? Character::Index[(ch)->Virtual()].func : NULL)
#define GET_MOB_TRGS(ch)	(IS_MOB(ch) ? Character::Index[(ch)->Virtual()].triggers : NULL)

#define GET_MOB_WAIT(ch)	((ch)->mob->wait_state)

#define MOB_TYPE(ch)		((ch)->mob->type)
#define IS_SIMPLE_MOB(ch)	(MOB_TYPE((ch)) == 0)
#define HUNTING(ch)			((ch)->mob->hunting)
#define SEEKING(ch)			((ch)->mob->seeking)
#define ROOMSEEKING(ch)		((ch)->mob->roomseeking)

#define GET_CLAN(ch)		((ch)->general.misc.clannum)

#define GET_DEFAULT_POS(ch)	((ch)->mob->default_pos)
#define MEMORY(ch)			((ch)->mob->memory)

#define CAN_CARRY_W(ch)		(GET_STR(ch) * 20)
#define CAN_CARRY_N(ch)		(5 + GET_DEX(ch))
#define AWAKE(ch)			(GET_POS(ch) > POS_SLEEPING)
#define CAN_SEE_IN_DARK(ch) \
   (AFF_FLAGGED(ch, AffectBit::Infravision) || PRF_FLAGGED(ch, Preference::HolyLight))

#define IS_GOOD(ch)    (GET_ALIGNMENT(ch) >= 350)
#define IS_EVIL(ch)    (GET_ALIGNMENT(ch) <= -350)
#define IS_NEUTRAL(ch) (!IS_GOOD(ch) && !IS_EVIL(ch))

#define CHAR_WATCHING(ch)	((ch)->general.misc.watching)


#define NO_STAFF_HASSLE(ch)	(IS_STAFF(ch) && PRF_FLAGGED(ch, Preference::NoHassle))

/* descriptor-based utils ************************************************/


#define WAIT_STATE(ch, cycle) { \
	if ((ch)->desc) (ch)->desc->wait = (cycle); \
	else if (IS_NPC(ch)) GET_MOB_WAIT(ch) = (cycle); }

#define CHECK_WAIT(ch)	(((ch)->desc) ? ((ch)->desc->wait > 1) : 0)
#define STATE(d)	((d)->connected)


/* object utils **********************************************************/


#define GET_OBJ_TYPE(obj)	((obj)->type)
#define GET_OBJ_COST(obj)	((obj)->cost)
#define GET_OBJ_EXTRA(obj)	((obj)->extra)
#define GET_OBJ_WEAR(obj)	((obj)->wear)
#define GET_OBJ_VAL(obj, val)	((obj)->value[(val)])
#define GET_OBJ_WEIGHT(obj)	((obj)->weight)
#define GET_OBJ_TIMER(obj)	((obj)->timer)

#define GET_OBJ_SPEED(obj)	((obj)->speed)

#define GET_OBJ_SPEC(obj) (obj->Virtual() >= 0 ? Object::Index[obj->Virtual()].func : NULL)

#define CAN_WEAR(obj, part) (IS_SET((obj)->wear, (part)))

#define IS_SITTABLE(obj)	((GET_OBJ_TYPE(obj) == ITEM_FURNITURE) && (GET_OBJ_VAL(obj, 1) >= POS_SITTING))
#define IS_SLEEPABLE(obj)	((GET_OBJ_TYPE(obj) == ITEM_FURNITURE) && (GET_OBJ_VAL(obj, 1) >= POS_RESTING))

#define IS_MOUNTABLE(obj)	((GET_OBJ_TYPE(obj) == ITEM_VEHICLE) && (GET_OBJ_VAL(obj, 0) == -1))

#define NO_LOSE(obj)		(OBJ_FLAGGED(obj, ITEM_NOLOSE))
#define NO_DROP(obj)		(OBJ_FLAGGED(obj, ITEM_NODROP))

/* compound utilities and other macros **********************************/


#define HSHR(ch) (GET_SEX(ch) ? (GET_SEX(ch)==Male ? "his":"her") :"its")
#define HSSH(ch) (GET_SEX(ch) ? (GET_SEX(ch)==Male ? "he" :"she") : "it")
#define HMHR(ch) (GET_SEX(ch) ? (GET_SEX(ch)==Male ? "him":"her") : "it")
#define HSHRS(ch) (GET_SEX(ch) ? (GET_SEX(ch)==Male ? "his":"hers") :"its")

#define ANA(obj) (strchr("aeiouyAEIOUY", *(obj)->Name()) ? "An" : "A")
#define SANA(obj) (strchr("aeiouyAEIOUY", *(obj)->Name()) ? "an" : "a")

#define PERS(ch, vict)   ((ch)->CanBeSeen(vict) ? (ch)->CalledName(vict) : "someone")

/* Various macros building up to CAN_SEE */

#define SELF(sub, obj)  ((sub) == (obj))

#define CAN_CARRY_OBJ(ch,obj)  \
   (((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) <= CAN_CARRY_W(ch)) &&   \
    ((IS_CARRYING_N(ch) + 1) <= CAN_CARRY_N(ch)))

#define CAN_GET_OBJ(ch, obj)   \
   (CAN_WEAR((obj), ITEM_WEAR_TAKE) && CAN_CARRY_OBJ((ch),(obj)) && \
    (obj)->CanBeSeen(ch))

#define OBJS(obj, vict) ((obj)->CanBeSeen(vict) ? \
	(obj)->CalledName(vict)  : "something")

#define OBJN(obj, vict) ((obj)->CanBeSeen(vict) ? \
	fname((obj)->Keywords()) : "something")


#define EXIT2(roomnum, door) (world[(roomnum)].Direction(door))
#define EXITN(roomnum, door) (EXIT2(roomnum, door))

#define EXIT(ch, door)  (EXIT2(IN_ROOM(ch), (door)))

#define VALID_EXIT(in_room, door) (EXIT2(in_room, door) && \
	     (EXIT2(in_room, door)->to_room > NOWHERE))

#define CAN_GO2(in_room, door) (VALID_EXIT((in_room), (door)) && \
			 (!IS_SET(EXIT2(in_room, door)->exit_info, Exit::Closed) || \
			 (IS_SET(EXIT2(in_room, door)->exit_info, Exit::Automatic) && \
			 !IS_SET(EXIT2(in_room, door)->exit_info, Exit::Vista) && \
			 !IS_SET(EXIT2(in_room, door)->exit_info, Exit::Locked))))

#define CAN_GO(ch, door) (CAN_GO2((IN_ROOM(ch)), (door)))

#define CAN_VIEW(roomnum, door) (VALID_EXIT((roomnum), (door)) && \
                       (IS_SET(EXIT2(roomnum,door)->exit_info, Exit::SeeThrough) || \
                       !IS_SET(EXIT2(roomnum,door)->exit_info, Exit::Closed)))


#ifdef GOT_RID_OF_IT


#define EXIT(ch, door)  (world[IN_ROOM(ch)].Direction(door))
#define EXITN(room, door)		(world[room].Direction(door))

#define EXIT2(roomnum, door) (world[(roomnum)].Direction(door))
#define CAN_GO2(roomnum, door) (EXIT2(roomnum, door) && \
                       (EXIT2(roomnum, door)->to_room != NOWHERE) && \
                       !IS_SET(EXIT2(roomnum,door)->exit_info, Exit::Closed))

#define CAN_GO(ch, door) ((IN_ROOM(ch) != NOWHERE) && EXIT(ch,door) && \
			 (EXIT(ch,door)->to_room != NOWHERE) && \
			 (!IS_SET(EXIT(ch, door)->exit_info, Exit::Closed) || \
			 (IS_SET(EXIT(ch, door)->exit_info, Exit::Automatic) && \
			 !IS_SET(EXIT(ch, door)->exit_info, Exit::Locked))))
#endif




#define IS_HIDDEN_EXIT(ch, door)		(IS_SET(EXIT(ch, door)->exit_info, Exit::Hidden))
#define EXIT_IS_HIDDEN(ch, door)		(IS_HIDDEN_EXIT(ch, door) &&\
										(!IS_SET(EXIT(ch, door)->exit_info, Exit::Door) ||\
										IS_SET(EXIT(ch,door)->exit_info, Exit::Closed)) &&\
										!GET_TRUST(ch))
#define IS_HIDDEN_EXITN(room, door)		(IS_SET(EXITN(room, door)->exit_info, Exit::Hidden))
#define EXITN_IS_HIDDEN(room, door, ch)	(IS_HIDDEN_EXITN(room, door) &&\
										(!IS_SET(EXITN(room, door)->exit_info, Exit::Door) ||\
										IS_SET(EXITN(room, door)->exit_info, Exit::Closed)) &&\
										!IS_STAFF(ch))

#define RACE_ABBR(ch) (IS_NPC(ch) ? "--" : race_abbrevs[(int)GET_RACE(ch)])

#define EXP_TO_LEVEL			(1000) // Temporary
#define GET_MATERIAL(thing)		((thing)->material)
#define GET_PD(thing)			((thing)->passive_defense)
#define GET_DR(thing)			((thing)->damage_resistance)
#define GET_TOTALPTS(ch)		((ch)->general.totalpts)
#define COMBAT_PT_UPDATE(ch)	((ch)->combat_profile.PointUpdate(ch))

#define IS_KILLER(ch)	(IS_NPC(ch) ? IS_SET(MOB_FLAGS(ch), MOB_KILLER) \
						: IS_SET(PLR_FLAGS(ch), PLR_KILLER))

#define IS_THIEF(ch)	(IS_NPC(ch) ? 0 : IS_SET(PLR_FLAGS(ch), PLR_THIEF))

#define IS_MORTAL(ch)	(GET_MORTALITY(ch) == MORTALITY_MORTAL)
#define IS_IMMORTAL(ch)	(GET_MORTALITY(ch) == MORTALITY_IMMORTAL)
#define IS_DEITY(ch)	(GET_MORTALITY(ch) == MORTALITY_DEITY)
#define IS_STAFF(ch)	(!IS_NPC(ch) && (GET_TRUST(ch) > 0) && STF_FLAGS(ch))
#define IS_INVULN(ch)	(PRF_FLAGGED(ch, Preference::Invuln))
#define IS_ADMIN(ch)	((GET_TRUST(ch) >= TRUST_ADMIN) || STF_FLAGGED(ch, Staff::Admin))

#define OUTSIDE(ch) (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_INDOORS) && (SECT(IN_ROOM(ch)) != SECT_INSIDE) && (SECT(IN_ROOM(ch)) != SECT_UNDERGROUND))

#define CLASS_ABBR(ch) (IS_NPC(ch) ? "--" : class_abbrevs[(int)GET_CLASS(ch)])
#define RACE_ABBR(ch)  (IS_NPC(ch) ? "--" : race_abbrevs[(int)GET_RACE(ch)])

#define IS_ANIMAL(ch)		(GET_RACE(ch) == RACE_ANIMAL)
#define IS_HYUMANN(ch)		(GET_RACE(ch) == RACE_HYUMANN)
#define IS_ELF(ch)			(GET_RACE(ch) == RACE_ELF)
#define IS_DWOMAX(ch)		(GET_RACE(ch) == RACE_DWOMAX)
#define IS_SHASTICK(ch)		(GET_RACE(ch) == RACE_SHASTICK)
#define IS_MURRASH(ch)		(GET_RACE(ch) == RACE_MURRASH)
#define IS_LAORI(ch)		(GET_RACE(ch) == RACE_LAORI)
#define IS_THADYRI(ch)		(GET_RACE(ch) == RACE_THADYRI)
#define IS_TERRAN(ch)		(GET_RACE(ch) == RACE_TERRAN)
#define IS_TRULOR(ch)		(GET_RACE(ch) == RACE_TRULOR)
#define IS_DRAGON(ch)		(GET_RACE(ch) == RACE_DRAGON)
#define IS_DEVA(ch)			(GET_RACE(ch) == RACE_DEVA)
#define IS_SOLAR(ch)		(GET_RACE(ch) == RACE_SOLAR)

#define IS_HUMANOID(ch)  ((GET_RACE(ch) == RACE_HYUMANN)  || \
			  (GET_RACE(ch) == RACE_ELF)    || \
			  (GET_RACE(ch) == RACE_DWOMAX)  || \
			  (GET_RACE(ch) == RACE_SHASTICK) || \
			  (GET_RACE(ch) == RACE_LAORI) || \
			  (GET_RACE(ch) == RACE_TERRAN))

#define IS_SOMEWHERE(ch)	(IN_ROOM(ch) != NOWHERE)

#define PURGED(i)	((i)->Purged())
#define GET_ID(i)	((i)->ID())

/* OS compatibility ******************************************************/


/* there could be some strange OS which doesn't have NULL... */
#ifndef NULL
#define NULL (Ptr)0
#endif

#if !defined(FALSE)
#define FALSE 0
#endif

#if !defined(TRUE)
#define TRUE  (!FALSE)
#endif

/* defines for fseek */
#ifndef SEEK_SET
#define SEEK_SET	0
#define SEEK_CUR	1
#define SEEK_END	2
#endif

//#if defined(NOCRYPT) || !defined(HAVE_CRYPT)
#define CRYPT(a,b) (a)
//#else
//#define CRYPT(a,b) ((char *) crypt((a),(b)))
//#endif

/* Special enhancements macros */

//char freeBuf[200];
void Free_Error(char * file, int line);

/*
#define FREE(ptr)		if (ptr == NULL) {					\
							Free_Error(__FILE__, __LINE__);	\
						} else {							\
							free(ptr);						\
							ptr = NULL;						\
						}
*/

#define	FOREACH(iterName, list, type)	\
	for (type::iterator iterName = list.begin(), iterName ## End = list.end(); iterName != iterName ## End; ++iterName)

#define	FOREACH_R(iterName, list, type)	\
	for (type::reverse_iterator iterName = list.rbegin(), iterName ## End = list.rend(); iterName != iterName ## End; ++iterName)

#endif

