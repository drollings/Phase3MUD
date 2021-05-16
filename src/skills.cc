/* ************************************************************************
*	File: skills.c																Part of TLO Phase ][		*
*																																				 *
*	Usage: Contains routines related to the SkillStore class,			*
*		which contains a linked list of the player's skills.			*
*	This code is Copyright (C) 1997 by Daniel Rollings AKA Daniel Houghton.				*
*																																				 *
*	$Author: sfn $
*	$Date: 2000/11/18 08:48:54 $
*	$Revision: 1.15 $
************************************************************************ */

#include "types.h"

#include "structs.h"
#include "skills.h"
#include "characters.h"
#include "utils.h"
#include "buffer.h"
#include "interpreter.h"
#include "find.h"
#include "rooms.h"
#include "comm.h"
#include "constants.h"

Vector<Skill *>	Skills;

void list_skills(Character * ch);
SInt16 spell_sort_info[TOP_SKILLO_DEFINE+1];

namespace Property {
	extern const char * skill_properties[];
}

#define SKILL_BASE(skill)        Skills[(skill)]->stats


char *spells[] =
{
	"!RESERVED!",					/* 0 - reserved */
	"air",
	"wind",
	"lightning",
	"sound",
	"fire",		// 5
	"light",
	"heat",
	"water",
	"mist",
	"ice",		// 10
	"earth",
	"stone",
	"metal",
	"chi",
	"life",		// 15
	"necromantic",
	"flora",
	"fauna",
	"psyche",
	"spirit",		// 20
	"illusion",
	"vim",
	"creator",
	"negator",
	"builder",		// 25
	"destroyer",
	"attract",
	"repel",
	"control",
	"aura",		// 30
	"infuse",
	"scry",
	"transmute",
	"radiate",
	"consume",		// 35
	"ward",
	"!UNUSED37!", "!UNUSED38!", "!UNUSED39!", 
	"chant",
	"somatic",
	"thought",
	"rune",
	"!UNUSED44!",
	"!UNUSED45!",	// 45
	"!UNUSED46!", "!UNUSED47!", "!UNUSED48!", "!UNUSED49!", "!UNUSED50!",	// 50
	"!UNUSED51!", "!UNUSED52!", "!UNUSED53!", "!UNUSED54!", "!UNUSED55!",	// 55
	"!UNUSED56!", "!UNUSED57!", "!UNUSED58!", "!UNUSED59!", 
	"ordination of pelendra",		// 60
	"ordination of talvindas",
	"ordination of dymordian",
	"ordination of kharsus",
	"ordination of nymris",
	"ordination of vaadh",		// 65
	"ordination of shestu",
	"!UNUSED67!", "!UNUSED68!", "!UNUSED69!", "!UNUSED70!",	// 70
	"!UNUSED71!", "!UNUSED72!", "!UNUSED73!", "!UNUSED74!", "!UNUSED75!",	// 75
	"!UNUSED76!", "!UNUSED77!", "!UNUSED78!", "!UNUSED79!", 
	"sphere of healing",		// 80
	"sphere of divination",
	"sphere of nature",
	"sphere of protection",
	"sphere of war",
	"sphere of heavens",		// 85
	"sphere of necromancy",
	"sphere of creation",
	"!UNUSED88!", "!UNUSED89!", "!UNUSED90!",	// 90
	"!UNUSED91!", "!UNUSED92!", "!UNUSED93!", "!UNUSED94!", "!UNUSED95!",	// 95
	"!UNUSED96!", "!UNUSED97!", "!UNUSED98!", "!UNUSED99!", "!UNUSED100!",	// 100
	"backstab",
	"bash",
	"hide",
	"kick",
	"pick lock",		// 105
	"punch",
	"rescue",
	"sneak",
	"steal",
	"berserk",		// 110
	"search",
	"disarm",
	"ride",
	"flip",
	"headbutt",		// 115
	"camoflauge",
	"iron fist",
	"strangle",
	"circle",
	"shield",		// 120
	"brawling",
	"!UNUSED122!", 
	"fast draw",
	"judo",
	"karate",		// 125
	"swimming",
	"bard",
	"animal handling",
	"falconry",
	"!UNUSED130!",	// 130
	"blacksmith",
	"carpentry",
	"cooking",
	"!UNUSED134!", 
	"leatherworking",		// 135
	"!UNUSED136!", 
	"elven language",
	"elven writing",
	"elven lore",
	"hyumann language",		// 140
	"hyumann writing",
	"hyumann lore",
	"laori language",
	"laori writing",
	"laori lore",		// 145
	"dwomax language",
	"dwomax writing",
	"dwomax lore",
	"murrash language",
	"murrash writing",		// 150
	"murrash lore",
	"shastick language",
	"shastick writing",
	"shastick lore",
	"thadyri language",		// 155
	"thadyri writing",
	"thadyri lore",
	"terran language",
	"terran writing",
	"terran lore",		// 160
	"trulor language",
	"trulor writing",
	"trulor lore",
	"sign language",
	"first aid",		// 165
	"boating",
	"climbing",
	"!UNUSED168!", "!UNUSED169!", "!UNUSED170!",	// 170
	"!UNUSED171!", "!UNUSED172!", "!UNUSED173!", "!UNUSED174!", "!UNUSED175!",	// 175
	"!UNUSED176!", "!UNUSED177!", 
	"tracking",
	"computer ops",
	"electronics ops",		// 180
	"!UNUSED181!", "!UNUSED182!", 
	"fasttalk",
	"!UNUSED184!", 
	"leadership",		// 185
	"merchant",
	"!UNUSED187!", 
	"sex appeal",
	"teaching",
	"demolition",		// 190
	"!UNUSED191!", 
	"disguise",
	"escape",
	"forgery",
	"!UNUSED195!",	// 195
	"lip reading",
	"streetwise",
	"mix poisons",
	"shadowing",
	"traps",		// 200
	"alchemy",
	"terran powered armor",
	"trulor powered armor",
	"terran piloting",
	"trulor piloting",		// 205
	"bladesong",
	"shield bash",
	"bite",
	"uppercut",
	"karate chop",		// 210
	"karate kick",
	"feint",
	"riposte",
	"dual wield",
	"combat technique",		// 215
	"!UNUSED216!", "!UNUSED217!", "!UNUSED218!", "!UNUSED219!", "!UNUSED220!",	// 220
	"!UNUSED221!", "!UNUSED222!", "!UNUSED223!", "!UNUSED224!", "!UNUSED225!",	// 225
	"!UNUSED226!", "!UNUSED227!", "!UNUSED228!", "!UNUSED229!", "!UNUSED230!",	// 230
	"!UNUSED231!", "!UNUSED232!", "!UNUSED233!", "!UNUSED234!", "!UNUSED235!",	// 235
	"!UNUSED236!", "!UNUSED237!", "!UNUSED238!", "!UNUSED239!", "!UNUSED240!",	// 240
	"!UNUSED241!", "!UNUSED242!", "!UNUSED243!", "!UNUSED244!", "!UNUSED245!",	// 245
	"!UNUSED246!", "!UNUSED247!", "!UNUSED248!", "!UNUSED249!", "!UNUSED250!",	// 250
	"bow",
	"crossbow",
	"sling",
	"throw",
	"mace",		// 255
	"axe",
	"axe throwing",
	"broadsword",
	"fencing",
	"flail",		// 260
	"knife",
	"knife throwing",
	"lance",
	"polearm",
	"shortsword",		// 265
	"spear",
	"spear throwing",
	"staff",
	"twohanded mace",
	"twohanded axe",		// 270
	"twohanded sword",
	"whip",
	"lasso",
	"net",
	"bola",		// 275
	"gun",
	"beamgun",
	"beamsword",
	"!UNUSED279!", "!UNUSED280!",	// 280
	"!UNUSED281!", "!UNUSED282!", "!UNUSED283!", "!UNUSED284!", "!UNUSED285!",	// 285
	"!UNUSED286!", "!UNUSED287!", "!UNUSED288!", "!UNUSED289!", "!UNUSED290!",	// 290
	"!UNUSED291!", "!UNUSED292!", "!UNUSED293!", "!UNUSED294!", "!UNUSED295!",	// 295
	"!UNUSED296!", "!UNUSED297!", "!UNUSED298!", "!UNUSED299!", "!UNUSED300!",	// 300
	"fire breath",
	"gas breath",
	"frost breath",
	"acid breath",
	"lightning breath",		// 305
	"!UNUSED306!", "!UNUSED307!", "!UNUSED308!", "!UNUSED309!", "!UNUSED310!",	// 310
	"!UNUSED311!", "!UNUSED312!", "!UNUSED313!", "!UNUSED314!", "!UNUSED315!",	// 315
	"!UNUSED316!", "!UNUSED317!", "!UNUSED318!", "!UNUSED319!", "!UNUSED320!",	// 320
	"!UNUSED321!", "!UNUSED322!", "!UNUSED323!", "!UNUSED324!", "!UNUSED325!",	// 325
	"!UNUSED326!", "!UNUSED327!", "!UNUSED328!", "!UNUSED329!", "!UNUSED330!",	// 330
	"!UNUSED331!", "!UNUSED332!", "!UNUSED333!", "!UNUSED334!", "!UNUSED335!",	// 335
	"!UNUSED336!", "!UNUSED337!", "!UNUSED338!", "!UNUSED339!", "!UNUSED340!",	// 340
	"!UNUSED341!", "!UNUSED342!", "!UNUSED343!", "!UNUSED344!", "!UNUSED345!",	// 345
	"!UNUSED346!", "!UNUSED347!", "!UNUSED348!", "!UNUSED349!", "!UNUSED350!",	// 350
	"!UNUSED351!", "!UNUSED352!", "!UNUSED353!", "!UNUSED354!", "!UNUSED355!",	// 355
	"!UNUSED356!", "!UNUSED357!", "!UNUSED358!", "!UNUSED359!", "!UNUSED360!",	// 360
	"!UNUSED361!", "!UNUSED362!", "!UNUSED363!", "!UNUSED364!", "!UNUSED365!",	// 365
	"!UNUSED366!", "!UNUSED367!", "!UNUSED368!", "!UNUSED369!", "!UNUSED370!",	// 370
	"!UNUSED371!", "!UNUSED372!", "!UNUSED373!", "!UNUSED374!", "!UNUSED375!",	// 375
	"!UNUSED376!", "!UNUSED377!", "!UNUSED378!", "!UNUSED379!", "!UNUSED380!",	// 380
	"!UNUSED381!", "!UNUSED382!", "!UNUSED383!", "!UNUSED384!", "!UNUSED385!",	// 385
	"!UNUSED386!", "!UNUSED387!", "!UNUSED388!", "!UNUSED389!", "!UNUSED390!",	// 390
	"!UNUSED391!", "!UNUSED392!", "!UNUSED393!", "!UNUSED394!", "!UNUSED395!",	// 395
	"!UNUSED396!", "!UNUSED397!", "!UNUSED398!", "!UNUSED399!", 
	"hit",		// 400
	"sting",
	"whip",
	"slash",
	"bite",
	"bludgeon",		// 405
	"crush",
	"pound",
	"claw",
	"maul",
	"thrash",		// 410
	"pierce",
	"blast",
	"stab",
	"garrote",
	"arrow",		// 415
	"bolt",
	"rock",
	"!UNUSED418!", "!UNUSED419!", "!UNUSED420!",	// 420
	"!UNUSED421!", "!UNUSED422!", "!UNUSED423!", "!UNUSED424!", "!UNUSED425!",	// 425
	"!UNUSED426!", "!UNUSED427!", "!UNUSED428!", "!UNUSED429!", "!UNUSED430!",	// 430
	"!UNUSED431!", "!UNUSED432!", "!UNUSED433!", "!UNUSED434!", "!UNUSED435!",	// 435
	"!UNUSED436!", "!UNUSED437!", "!UNUSED438!", "!UNUSED439!", "!UNUSED440!",	// 440
	"!UNUSED441!", "!UNUSED442!", "!UNUSED443!", "!UNUSED444!", "!UNUSED445!",	// 445
	"!UNUSED446!", "!UNUSED447!", "!UNUSED448!", "!UNUSED449!", 
	"dodge",		// 450
	"parry",
	"!UNUSED452!", "!UNUSED453!", "!UNUSED454!", "!UNUSED455!",	// 455
	"!UNUSED456!", "!UNUSED457!", "!UNUSED458!", "!UNUSED459!", "!UNUSED460!",	// 460
	"!UNUSED461!", "!UNUSED462!", "!UNUSED463!", "!UNUSED464!", "!UNUSED465!",	// 465
	"!UNUSED466!", "!UNUSED467!", "!UNUSED468!", "!UNUSED469!", "!UNUSED470!",	// 470
	"!UNUSED471!", "!UNUSED472!", "!UNUSED473!", "!UNUSED474!", "!UNUSED475!",	// 475
	"!UNUSED476!", "!UNUSED477!", "!UNUSED478!", "!UNUSED479!", "!UNUSED480!",	// 480
	"!UNUSED481!", "!UNUSED482!", "!UNUSED483!", "!UNUSED484!", "!UNUSED485!",	// 485
	"!UNUSED486!", "!UNUSED487!", "!UNUSED488!", "!UNUSED489!", "!UNUSED490!",	// 490
	"!UNUSED491!", "!UNUSED492!", "!UNUSED493!", "!UNUSED494!", "!UNUSED495!",	// 495
	"!UNUSED496!", "!UNUSED497!", "!UNUSED498!", 
	"suffering",
	"armor",		// 500
	"teleport",
	"bless",
	"blindness",
	"burning hands",
	"call lightning",		// 505
	"charm",
	"chill touch",
	"clone",
	"color spray",
	"control weather",		// 510
	"create food",
	"create water",
	"cure blind",
	"cure critical",
	"cure light",		// 515
	"curse",
	"detect align",
	"detect invis",
	"detect magic",
	"detect poison",		// 520
	"dispel evil",
	"earthquake",
	"enchant weapon",
	"energy drain",
	"fireball",		// 525
	"harm",
	"heal",
	"invisible",
	"lightning bolt",
	"locate object",		// 530
	"magic missile",
	"poison",
	"protection from evil",
	"remove curse",
	"mage shield",		// 535
	"shocking grasp",
	"sleep",
	"strength",
	"summon",
	"ventriloquate",		// 540
	"word of recall",
	"remove poison",
	"sense life",
	"animate dead",
	"dispel good",		// 545
	"group armor",
	"group heal",
	"group recall",
	"infravision",
	"fly",		// 550
	"identify",
	"earthspike",
	"silence",
	"levitate",
	"refresh",		// 555
	"faerie fire",
	"stoneskin",
	"haste",
	"blade barrier",
	"fire shield",		// 560
	"create light",
	"cause light",
	"cause critical",
	"holy word",
	"ice storm",		// 565
	"inferno",
	"recharge",
	"create feast",
	"phantom fist",
	"holy armor",		// 570
	"acid rain",
	"psychic attack",
	"horrific illusion",
	"regenerate",
	"flame dagger",		// 575
	"poultice",
	"grenade",
	"poison gas",
	"freeze foe",
	"wall of fog",		// 580
	"relocate",
	"peace",
	"planeshift",
	"group planeshift",
	"vampiric regeneration",		// 585
	"sanctuary",
	"portal",
	"bind", 
	"raise dead", 
	"reincarnate",	// 590
	"converse with earth",
	"entangle",
	"earthseize",
	"caltrops",
	"scorched earth",
	"flame affinity",
	"flammability",
	"tornado",
	"tremor",
	"ice wall",	// 600
	"controlled illusion", 
	"fire elemental",
	"earth elemental",
	"air elemental",
	"water elemental",
	"dehydration",
	"raucous laughter",
	"wracking pain",
	"!UNUSED609!", "!UNUSED610!",	// 610
	"!UNUSED611!", "!UNUSED612!", "!UNUSED613!", "!UNUSED614!", "!UNUSED615!",	// 615
	"!UNUSED616!", "!UNUSED617!", "!UNUSED618!", "!UNUSED619!", "!UNUSED620!",	// 620
	"!UNUSED621!", "!UNUSED622!", "!UNUSED623!", "!UNUSED624!", "!UNUSED625!",	// 625
	"!UNUSED626!", "!UNUSED627!", "!UNUSED628!", "!UNUSED629!", "!UNUSED630!",	// 630
	"!UNUSED631!", "!UNUSED632!", "!UNUSED633!", "!UNUSED634!", "!UNUSED635!",	// 635
	"!UNUSED636!", "!UNUSED637!", "!UNUSED638!", "!UNUSED639!", "!UNUSED640!",	// 640
	"!UNUSED641!", "!UNUSED642!", "!UNUSED643!", "!UNUSED644!", "!UNUSED645!",	// 645
	"!UNUSED646!", "!UNUSED647!", "!UNUSED648!", "!UNUSED649!", 
	"str",		// 650
	"int",
	"wis",
	"dex",
	"con",
	"cha",		// 655
	"weapon",
	"vision",
	"hearing",
	"movement",
	"weapon r",		// 660
	"weapon l",
	"weapon 3",
	"weapon 4",
	"!UNUSED664!", "!UNUSED665!",	// 665
	"!UNUSED666!", "!UNUSED667!", "!UNUSED668!", "!UNUSED669!", "!UNUSED670!",	// 670
	"!UNUSED671!", "!UNUSED672!", "!UNUSED673!", "!UNUSED674!", "!UNUSED675!",	// 675
	"!UNUSED676!", "!UNUSED677!", "!UNUSED678!", "!UNUSED679!", "!UNUSED680!",	// 680
	"!UNUSED681!", "!UNUSED682!", "!UNUSED683!", "!UNUSED684!", "!UNUSED685!",	// 685
	"!UNUSED686!", "!UNUSED687!", "!UNUSED688!", "!UNUSED689!", "!UNUSED690!",	// 690
	"!UNUSED691!", "!UNUSED692!", "!UNUSED693!", "!UNUSED694!", "!UNUSED695!",	// 695
	"!UNUSED696!", "!UNUSED697!", "!UNUSED698!", "!UNUSED699!", "!UNUSED700!",	// 700
	"\n"
};


Skill::Skill(void) : number(-1), delay(0), lag(0), name(NULL), wearoff(NULL), wearoffroom(NULL), types(0), 
					stats(0), min_position(0), target(0), routines(0), mana(0), startmod(0), maxincr(0), 
					violent(false), learnable(true) { }

Skill::Skill(const Skill &from) :	number(from.number), delay(from.delay), lag(from.lag), 
				name(NULL), wearoff(NULL), wearoffroom(NULL), types(from.types),
				stats(from.stats), min_position(from.min_position), target(from.target), routines(from.routines),
				mana(from.mana), startmod(from.startmod), maxincr(from.maxincr), violent(from.violent), 
				learnable(from.learnable), properties(from.properties)
{
	Skill * skillptr = (Skill *) &from;

	if (from.name)
		name = strdup(from.name);
	if (from.wearoff)
		wearoff = strdup(from.wearoff);
	if (from.wearoffroom)
		wearoffroom = strdup(from.wearoffroom);

	prerequisites = from.prerequisites;
	defaults = from.defaults;
	anrequisites = from.anrequisites;
}
									

Skill::~Skill(void) { }


Result Skill::CalculateResult(SInt32 roll) {
	if (roll <= -26)						return Blunder;
	else if (roll >= -25 && roll <= 4)		return AFailure;
	else if (roll >= 5 && roll <= 75)		return Failure;
	else if (roll >= 76 && roll <= 90)		return PSuccess;
	else if (roll >= 91 && roll <= 110)		return NSuccess;
	else if (roll >= 111 && roll <= 175)	return Success;
	else									return ASuccess;
}


const char *Skill::Name(SInt32 skill) {
	if (skill < 1)				return (skill == -1) ? "UNUSED" : "UNDEFINED";
	if (skill > TOP_SKILLO_DEFINE)	return "UNDEFINED";
	
	return spells[skill];
}


SInt32 Skill::Number(const char *name, int type) 
{
	int i;
	bool ok;
	char *temp, *temp2;
	char first[256], first2[256];

	for (i = 1; i < Skills.Count(); ++i) {
		if (!IS_SET(Skills[i]->types, type))
			continue;
		if (is_abbrev(name, Skill::Name(i)))
			return i;

		ok = true;
		temp = any_one_arg(Skill::Name(i), first);
		temp2 = any_one_arg(name, first2);
		while (*first && *first2 && ok) {
			if (!is_abbrev(first2, first))
				ok = false;
			temp = any_one_arg(temp, first);
			temp2 = any_one_arg(temp2, first2);
		}

		if (ok && !*first2)
			return i;
	}

	return -1;
}


SInt32 Skill::Number(const char *name, SInt16 index = 1, SInt16 top = TOP_SKILLO_DEFINE) 
{
	bool ok;
	char *temp, *temp2;
	char first[256], first2[256];

	// Done to allow intuitive use for searching a given range.
	index = MAX(--index, 0);

	while (*spells[++index] != '\n') {
		if (index > top)
			return -1;
		if (is_abbrev(name, spells[index]))
			return index;

		ok = true;
		temp = any_one_arg(spells[index], first);
		temp2 = any_one_arg(name, first2);
		while (*first && *first2 && ok) {
			if (!is_abbrev(first2, first))
				ok = false;
			temp = any_one_arg(temp, first);
			temp2 = any_one_arg(temp2, first2);
		}

		if (ok && !*first2)
			return index;
	}

	return -1;
}


const Skill *Skill::ByNumber(SInt32 skill) {
	if (skill < 1 || skill > NUM_SKILLS)	return NULL;
	for (SInt32 index = 0; index < NUM_SKILLS; index++) {
		if (Skills[index]->number == skill)
			return Skills[index];
	}
	return NULL;
}


const Skill *Skill::ByName(const char *skill) {
	if (!skill || !*skill)		return NULL;
	for (SInt32 index = 0; index < NUM_SKILLS; index++) {
		if (is_abbrev(skill, Skills[index]->name))
			return Skills[index];
	}
	return NULL;
}


void sort_spells(void)
{
	int a, b, tmp;

	/* initialize array */
	for (a = 1; a < TOP_SKILLO_DEFINE; a++)
		spell_sort_info[a] = a;

	/* Sort.	'a' starts at 1, not 0, to remove 'RESERVED' */
	for (a = 1; a < TOP_SKILLO_DEFINE - 1; a++)
		for (b = a + 1; b < TOP_SKILLO_DEFINE; b++)
			if (strcmp(spells[spell_sort_info[a]], spells[spell_sort_info[b]]) > 0) {
				tmp = spell_sort_info[a];
				spell_sort_info[a] = spell_sort_info[b];
				spell_sort_info[b] = tmp;
			}
}


int barpoints(int percent)
{
	if (percent <= 2) 			return 0;
	else if (percent <= 4)		return 1;
	else if (percent <= 6)		return 2;
	else if (percent <= 8)		return 3;
	else if (percent <= 9)		return 4;
	else if (percent <= 10)		return 5;
	else if (percent <= 11)		return 6;
	else if (percent <= 12)		return 7;
	else if (percent <= 14)		return 8;
	else if (percent <= 16)		return 9;
	else						return 10;
}


char *how_good(int percent, int theory)
{
	static char buf[256];
	int barLearned = 0, barKnown = 0, barRemainder = 10, i;
	register char *pos = 0;
	
	strcpy(buf, " &cy[");
	
	barLearned = barpoints(percent);
	barKnown = barpoints(theory);

	if (barLearned > barKnown) {
		barKnown = 0;
	} else {
		barKnown = MAX(0, barKnown - barLearned);
	}
	
	barRemainder = MAX(0, 10 - (barLearned + barKnown));
	
	if (barLearned) {
		strcat(buf, "&cm");
		pos = buf + strlen(buf);
		for (i = barLearned;
			 i-- > 0;
			 *pos++ = '*');
		*pos = '\0';
	}

	if (barKnown) {
		strcat(buf, "&cb");
		pos = buf + strlen(buf);
		for (i = barKnown;
			 i-- > 0;
			 *pos++ = '+');
		*pos = '\0';
	}
	
	if (barRemainder) {
		strcat(buf, "&cb");
		pos = buf + strlen(buf);

		for (i = barRemainder;
		 i-- > 0;
		 *pos++ = '-');
		*pos = '\0';
	}


	strcat(buf, "&cy]&c0");
	return (buf);
}


void list_skills(Character *ch, bool show_known_only, Flags type)
{
	SkillKnown *ch_skill = NULL;
	SInt16 i, sortpos, bestdefault = 0;
	SInt16 tempvalue, temptheory;	// Used to store the "how good" value and avoid checking SKILLCHANCE twice.
	SInt8 tempskillchances[TOP_SKILLO_DEFINE + 1];
	UInt8 temptheorychance[TOP_SKILLO_DEFINE + 1];
	UInt8 tempskillpoints[TOP_SKILLO_DEFINE + 1];
	UInt8 temptheorypoints[TOP_SKILLO_DEFINE + 1];
	SInt8 chance = 0, mod = 0;

	extern int avg_base_chance(Character *ch, SInt16 skillnum);
	extern int gen_mob_skillpoints(Character *ch, SInt16 &skillnum);
	extern const char *named_bases[];

	extern void stat_spell_to_ch(Character *ch, SInt16 num);

	*buf = '\0';

	// We're setting up the temporary array.	If we're only looking for known 
	// skills, we rely on the character's SkillStore linked list; otherwise,
	// we'll stick with looping through.	The point of this approach is that
	// we only search through the SkillStore linked list ONCE.

	if (show_known_only) {
		// Clear and set up the temporary array.
		for (i = TOP_SKILLO_DEFINE; i; --i) {
			tempskillchances[i] = SKILL_NOCHANCE;
			tempskillpoints[i] = 0;
			temptheorypoints[i] = 0;
			temptheorychance[i] = 0;
		}

		CHAR_SKILL(ch).GetTable(&ch_skill);
	
		while (ch_skill) {
			i = ch_skill->skillnum;

			if (LEARNABLE_SKILL(i) && IS_SET(Skills[i]->types, type)) {
				tempskillchances[i] = SKILLCHANCE(ch, i, ch_skill);
				tempskillpoints[i] = ch_skill->proficient;
				temptheorypoints[i] = ch_skill->theory;
	
				tempvalue = ch_skill->proficient;
				ch_skill->proficient = ch_skill->theory;
				temptheorychance[i] = SKILLCHANCE(ch, i, ch_skill);
				ch_skill->proficient = tempvalue;
			}

			ch_skill = ch_skill->next;
		}
	} else {
		for (i = TOP_SKILLO_DEFINE; i; --i) {
			tempskillchances[i] = SKILL_NOCHANCE;
			tempskillpoints[i] = 0;
			temptheorychance[i] = 0;
			temptheorypoints[i] = 0;

			if (!Skills[i]->stats) 
				continue;

			if (Skills[i]->types && (!IS_SET(Skills[i]->types, type)))
				continue;

			if (!IS_SET(Skills[i]->types, type))
				continue;

			FINDSKILL(ch, i, ch_skill);
			tempskillchances[i] = SKILLCHANCE(ch, i, ch_skill);

			if (ch_skill) {
				tempskillpoints[i] = ch_skill->proficient;
				tempvalue = ch_skill->proficient;
				temptheorypoints[i] = ch_skill->theory;
				ch_skill->proficient = ch_skill->theory;
				temptheorychance[i] = SKILLCHANCE(ch, i, ch_skill);
				ch_skill->proficient = tempvalue;
			}
			
		}
	}
			
	for (sortpos = 1; sortpos < TOP_SKILLO_DEFINE; ++sortpos) {
		i = spell_sort_info[sortpos];

		const Skill *	skill = Skill::ByNumber(i);
		if (!skill)	continue;

		tempvalue = tempskillchances[i];
		temptheory = temptheorychance[i];
			
		if (tempvalue <= SKILL_NOCHANCE)
			continue;
			
		if (!IS_SET(Skills[i]->types, type))
			continue;

		if (strlen(buf) >= MAX_STRING_LENGTH - 100) {
			strcat(buf, "**OVERFLOW**\n");
			break;
		}

		if ((IS_NPC(ch)) && (tempskillpoints[i] == 0))
			tempskillpoints[i] = gen_mob_skillpoints(ch, i);

		if (SKL_IS_SPELL(i) || SKL_IS_PRAYER(i)) {
			CHAR_SKILL(ch).FindBestRoll(ch, i, bestdefault, chance, mod);
			sprintf(buf2, "&cB%-23.23s &cG%3d", Skill::Name(bestdefault), mod);
		} else {
			sprintbit(SKILL_BASE(i), named_bases, buf3);
			// sprintf(buf2, "&cb[&cB%-14.14s&cb] &cG%3d %3d", 
			//				 buf3, tempskillpoints[i], tempvalue - avg_base_chance(ch, i));
			sprintf(buf2, "&cb[&cB%-12.12s&cb] &cG%3d/%3d %7d", 
							buf3, tempskillpoints[i], temptheorypoints[i], tempvalue);
		}

		sprintf(buf + strlen(buf), "&cc%-25s %s %s\r\n", 
			Skill::Name(i), 
			buf2,
			how_good(tempvalue, temptheory));

	}

	page_string(ch->desc, buf, 1);
	*buf = '\0';
	*buf2 = '\0';
	*buf3 = '\0';
}


SInt8 skill_pts_to_mod(SInt16 skillnum, UInt8 pts);
UInt8 skill_mod_to_pts(SInt16 skillnum, SInt8 mod);
UInt8 skill_lvl_to_pts(Character *ch, SInt16 skillnum);
int get_skill_pts(Character *ch, SInt16 skillnum, SkillKnown *i);

#define SKLBASE(num, i) (IS_SET(SKILL_BASE(num), (i)))

const char * Skill::namedtypes[] = {
	"general",
	"movement",
	"weapon",
	"combat",
	"rangedcombat",
	"stealth",
	"engineering",
	"language",
	"root",
	"method",
	"technique",
	"ordination",
	"sphere",
	"vow",
	"spell",
	"prayer",
	"psi",
	"stat",
	"message",
	"default",
	"\n"
};


const char * Skill::targeting[] = {
	"Ignore",
	"CharRoom",
	"CharWorld",
	"FightSelf",
	"FightVict",
	"SelfOnly",
	"NotSelf",
	"ObjInv",
	"ObjRoom",
	"ObjWorld",
	"ObjEquip",
	"ObjDest",
	"CharObj",
	"IgnChar",
	"IgnObj",
	"Melee",
	"Ranged",
	"\n"
};


// Constructor
SkillStore::SkillStore()
{
	skills = NULL;
}


// Destructor
SkillStore::~SkillStore()
{
	ClearSkills();
}


// Clears contents of the object
void SkillStore::ClearSkills()
{
	SkillKnown *i = NULL, *prev = NULL;
	
	for(i = skills; i; prev = i, i = i->next, delete prev);
	
	skills = NULL;
}


// Sets pointer i to start of skill table; returns 1 if skill found, 0 if not.
int SkillStore::GetTable(SkillKnown **i) const
{
	*i = skills;

	if (skills == NULL)
		return 0;
	else 
		return 1;
}


// Sets pointer i to a given skill.	Returns 1 if skill found, 0 if not.
int SkillStore::FindSkill(SInt16 skillnum, SkillKnown **i) const
{
	int n = TOP_SKILLO_DEFINE;	// A kludge.	We're having trouble with infinite loops!
	SkillKnown *j;
	
	if (!LEARNABLE_SKILL(skillnum))
		return 0;	// Skill does not exist!

	*i = NULL;	// We're looking for the blasted thing, hence we don't have it!
	j = NULL;
	
	// First get the skill pointer set.	Since skills are only added
	// to the list in ascending order of skillnums, we can break
	// if the skillnum exceeds the num we're searching for.

	for (j = skills; 
			 j && (j->skillnum < skillnum) && n; 
			 j = j->next, n--);
			 
	if (!n) {
		log("SYSERR:	Infinite loop barely avoided in FindSkill.");
		return 0;	// Uh oh.	We just barely avoided an infinite loop.	Gotta fix something!
	}
	
	if (j && (j->skillnum != skillnum))	// Skillnum found exceeds the number
		j = NULL;				// we're looking for, it don't exist.
		
	*i = j;

	if (j == NULL)	// Either list is empty, or has contents sans what
		return 0;		// we're looking for.

	return 1;		// Q'aplah!
}


// Gets the character's default modifier on a skill.	Returns -20 if not learned.
int SkillStore::GetSkillMod(SInt16 skillnum, SkillKnown *i) const
{
	if ((!i) && (LEARNABLE_SKILL(skillnum)))
		FindSkill(skillnum, &i);

	if (i)
		return skill_pts_to_mod(skillnum, i->proficient);
	else
		return SKILL_NOCHANCE;
}


// Gets the character's point investment on a skill.	Returns 0 if not learned.
int SkillStore::GetSkillPts(SInt16 skillnum, SkillKnown *i) const
{
	if ((!i) && (LEARNABLE_SKILL(skillnum)))
		FindSkill(skillnum, &i);

	if (i)
		return i->proficient;

	return 0;
}


// Gets the character's point investment on a skill.	Returns 0 if not learned.
int SkillStore::GetSkillTheory(SInt16 skillnum, SkillKnown *i) const
{
	if ((!i) && (LEARNABLE_SKILL(skillnum)))
		FindSkill(skillnum, &i);

	if (i)
		return i->theory;
	else
		return 0;
}


int SkillStore::CheckSkillPrereq(Character *ch, SInt16 &skillnum)
{
	int skillpts = 0, i;

	// Prerequisite check.  The .chance variable represents the points allocated
	// to the skill. -DH

	if ((skillnum == METHOD_THOUGHT) && (ADVANTAGED(ch, Advantages::Sapient))) {
		return true;
	}

	for (i = 0; i < Skills[skillnum]->prerequisites.Count(); ++i) {
		if (LEARNABLE_SKILL(Skills[skillnum]->prerequisites[i].skillnum))
			skillpts = get_skill_pts(ch, Skills[skillnum]->prerequisites[i].skillnum, NULL);
		else
			skillpts = GetSkillChance(ch, Skills[skillnum]->prerequisites[i].skillnum, NULL, false);

		if (skillpts < Skills[skillnum]->prerequisites[i].chance)
			return false;
	}

	// Anrequisite check.	

	for (i = 0; i < Skills[skillnum]->anrequisites.Count(); ++i) {
		if (LEARNABLE_SKILL(Skills[skillnum]->anrequisites[i].skillnum))
			skillpts = get_skill_pts(ch, Skills[skillnum]->anrequisites[i].skillnum, NULL);
		else
			skillpts = GetSkillChance(ch, Skills[skillnum]->anrequisites[i].skillnum, NULL, false);

		if (skillpts >= Skills[skillnum]->anrequisites[i].chance)
			return false;
	}
	
	return (true);
}


int avg_base_chance(Character *ch, SInt16 skillnum)
{
	SInt16 chance = 0, bases = 0; 

	if (SKLBASE(skillnum, SKLBASE_STR)) { chance += GET_STR(ch); ++bases; }
	if (SKLBASE(skillnum, SKLBASE_INT)) { chance += GET_INT(ch); ++bases; }
	if (SKLBASE(skillnum, SKLBASE_WIS)) { chance += GET_WIS(ch); ++bases; }
	if (SKLBASE(skillnum, SKLBASE_DEX)) { chance += GET_DEX(ch); ++bases; }
	if (SKLBASE(skillnum, SKLBASE_CON)) { chance += GET_CON(ch); ++bases; }
	if (SKLBASE(skillnum, SKLBASE_CHA)) { chance += GET_CHA(ch); ++bases; }	
	if (SKLBASE(skillnum, SKLBASE_VISION)) { chance += GET_INT(ch); ++bases; }	
	if (SKLBASE(skillnum, SKLBASE_HEARING)) { chance += GET_INT(ch); ++bases; }	
	if (SKLBASE(skillnum, SKLBASE_MOVE)) { chance += (GET_DEX(ch) + GET_CON(ch)) / 4; ++bases; }

	if (bases)
		chance /= bases;				// Average out skill bases

	return (chance);

}


int gen_mob_skillpoints(Character *ch, SInt16 &skillnum)
{

	int points = 0;	// This variable is later initialized to a number which
				// will represent a percentage of the character's level.
			// This number is equivalent to the points "invested".
	
	if (SKL_IS_RUNE(skillnum)) {
		switch (GET_CLASS(ch)) {
		case CLASS_MAGICIAN:	points = 300; break;
		case CLASS_PSIONICIST: 
			 if (skillnum == SPHERE_PSYCHE)
				 points = 100;
			 else 
				 break;
		default: return (0);
		}
	} else if (SKL_IS_CSPHERE(skillnum)) {
		switch (GET_CLASS(ch)) {
		case CLASS_CLERIC:		points = 300; break;
		default:
			return (0);
		}
	} else if (SKL_IS_ORDINATION(skillnum)) {
		switch (GET_CLASS(ch)) {
		case CLASS_CLERIC:		points = 300; break;
		default:
			return (0);
		}
	} else if (SKL_IS_WEAPONPROF(skillnum)) {
		switch (GET_CLASS(ch)) {
		case CLASS_ALCHEMIST:
		case CLASS_PSIONICIST:
		case CLASS_MAGICIAN:	points = 75; break;
		case CLASS_CLERIC:
		case CLASS_THIEF:		points = 100; break;
		case CLASS_ASSASSIN:	
		case CLASS_RANGER:
		case CLASS_VAMPIRE:		points = 200; break;
		case CLASS_WARRIOR:		points = 300; break;
		default:
			return (0);
		}
	} else if (SKL_IS_COMBATSKILL(skillnum)) {
		switch (GET_CLASS(ch)) {
		case CLASS_ALCHEMIST:
		case CLASS_PSIONICIST:
		case CLASS_MAGICIAN:	points = 40; break;
		case CLASS_CLERIC:
		case CLASS_THIEF:		points = 80; break;
		case CLASS_ASSASSIN:	
		case CLASS_RANGER:
		case CLASS_VAMPIRE:		points = 200; break;
		case CLASS_WARRIOR:		points = 300; break;
		default:
			return (0);
		}
	} else if (SKL_IS_SKILL(skillnum)) {
		switch (GET_CLASS(ch)) {
		case CLASS_MAGICIAN:
		case CLASS_PSIONICIST:	points = 50; break;
		case CLASS_CLERIC:
		case CLASS_WARRIOR:		points = 100; break;
		case CLASS_ALCHEMIST:
		case CLASS_THIEF:
		case CLASS_ASSASSIN:
		case CLASS_RANGER:
		case CLASS_VAMPIRE:		points = 150; break;
		default:
			return (0);
		}
	}
	
	// Okay, if we've gotten this far, this is a skill that the mob can
	// perform given its class, and we take the points as a percentage of level
	// to give in points in a skillroll.
	
	points *= GET_LEVEL(ch);
	points /= 100;	

	return (MIN(points, 250));
	
}


// This function returns default skills for mobs that don't have any skills
// defined.	Why a switch statement?	We don't want the rigid dimensions of
// an array or the processing of a linked list.	So there. :)	-DH
int gen_mob_skillchance(Character *ch, SInt16 &skillnum)
{

	int points = 0;
	
	points = gen_mob_skillpoints(ch, skillnum);

	if (points)
		return (avg_base_chance(ch, skillnum) + skill_pts_to_mod(skillnum, points));
	else
		return (SKILL_NOCHANCE);

}


// Gets a number to roll against on 3d6.	Returns SKILL_NOCHANCE if there's not a
// snowball's chance in hell of the character using the skill.
int SkillStore::GetSkillChance(Character *ch, SInt16 skillnum, 
				SkillKnown *i, SInt8 chkdefault)
{
	int chance = SKILL_NOCHANCE, temp = SKILL_NOCHANCE;
	SInt8 bases = 0, j = 0, tempmodifier = 0;
	SInt16 tempnum = 0, points = 0;

	if (!VALID_SKILL(skillnum))
		return (SKILL_NOCHANCE);

	if ((skillnum == METHOD_THOUGHT) && ADVANTAGED(ch, Advantages::Sapient) && chkdefault) {
		chance = GetSkillChance(ch, skillnum, NULL, false);
		return (GET_INT(ch) + MAX(-6, chance));
	}


	// Special cases, these.
	if (SKL_IS_STAT(skillnum)) {
		switch (skillnum) {

		// Cases for rolling against a character's stats.
		case STAT_STR: chance = GET_STR(ch); break;
		case STAT_INT: chance = GET_INT(ch); break;
		case STAT_WIS: chance = GET_WIS(ch); break;
		case STAT_DEX: chance = GET_DEX(ch); break;
		case STAT_CON: chance = GET_CON(ch); break;
		case STAT_CHA: chance = GET_CHA(ch); break;
		case SKILL_WEAPON: chance = SKILLCHANCE(ch, GET_WEAPONSKILL(ch, 0), NULL); break;
		case STAT_VISION: chance = GET_INT(ch); break;
		case STAT_HEARING: chance = GET_INT(ch); break;
		case STAT_MOVEMENT: chance = GET_MOVEMENT(ch); break;
		case SKILL_WEAPON_R: chance = SKILLCHANCE(ch, GET_WEAPONSKILL(ch, WEAR_HAND_R), NULL); break;
		case SKILL_WEAPON_L: chance = SKILLCHANCE(ch, GET_WEAPONSKILL(ch, WEAR_HAND_L), NULL); break;
		case SKILL_WEAPON_3: chance = SKILLCHANCE(ch, GET_WEAPONSKILL(ch, WEAR_HAND_3), NULL); break;
		case SKILL_WEAPON_4: chance = SKILLCHANCE(ch, GET_WEAPONSKILL(ch, WEAR_HAND_4), NULL); break;
		}
		
		return (chance);
	}

	if ((!IS_NPC(ch)) && (IS_IMMORTAL(ch))) // This should save us some work.
		return (30);

	if (!CheckSkillPrereq(ch, skillnum))
			return (SKILL_NOCHANCE);	// We failed!	Skillroll is not possible.
	
	// Let's be sure we have a pointer to the skill.
	// If i is NULL, let's try going through FindSkill
	if ((i == NULL) && (LEARNABLE_SKILL(skillnum))) {
		FindSkill(skillnum, &i);
	}

	// points = GetSkillPts(skillnum, i);
	points = get_skill_pts(ch, skillnum, i);

	// Okay, the player has spent points on the skill.
	if (points && Skills[skillnum]->stats) {
		chance = avg_base_chance(ch, skillnum) + skill_pts_to_mod(skillnum, points);
	}

	if (!LEARNABLE_SKILL(skillnum))
		chkdefault = 1;

	// Check the skill's defaults, setting chance if one is found that is better.
	if (chkdefault) {		// Skip if this is already a default check

		if (LEARNABLE_SKILL(skillnum))
			--chkdefault;	// Decrement chkdefault.	If it's 0, this skill is
												// considered a dead end for default checks.

		for (j = 0; j < Skills[skillnum]->defaults.Count(); ++j) {
			tempnum = Skills[skillnum]->defaults[j].skillnum;
			tempmodifier = Skills[skillnum]->defaults[j].chance;

			if (tempnum) {
				temp = GetSkillChance(ch, tempnum, i, chkdefault);
			if (temp != SKILL_NOCHANCE)
				temp += tempmodifier;

			if (chance < temp)
				(chance = temp);
			}
		}
	}

	return (chance);
}


// Takes skillnum, and sets default to the best skillnum to roll and chance to
// the best chance.	Intended for use by RollSkill.
void SkillStore::FindBestRoll(Character *ch, SInt16 &skillnum, SInt16 &defaultnum,
							SInt8 &chance, SInt8 &mod, int type = 0)
{
	SInt8 tmpchance = SKILL_NOCHANCE, tmpmod = 0, j;
	SInt16 tmpdefault = 0;

	chance = SKILL_NOCHANCE;
	mod = 0;
	defaultnum = skillnum;

	if (IS_SET(Skills[skillnum]->types, Skill::STAT))
		return;
		
	if (!CheckSkillPrereq(ch, skillnum)) {
		chance = SKILL_NOCHANCE;	// We failed!	Skillroll is not possible.
		return;
	}

	if (LEARNABLE_SKILL(skillnum) || SKL_IS_STAT(skillnum))
		chance = GetSkillChance(ch, skillnum, NULL, FALSE);

	for (j = 0; j < Skills[skillnum]->defaults.Count(); ++j) {
		tmpdefault = Skills[skillnum]->defaults[j].skillnum;
		tmpmod = Skills[skillnum]->defaults[j].chance;

		if (!tmpdefault)
			continue;

		// Is this within the specified range?	This allows us to do searches for
		// specific types of defaults, such as mage runes or cleric spheres.
		if (type && (!IS_SET(Skills[tmpdefault]->types, type)))
			continue;

		tmpchance = GetSkillChance(ch, tmpdefault, NULL, (!LEARNABLE_SKILL(skillnum)));

		if (tmpchance == SKILL_NOCHANCE)
			continue;

		tmpchance += tmpmod;

		if (tmpchance > chance) {
			defaultnum = tmpdefault;
			chance = tmpchance;
			mod = tmpmod;
		}
	}
}


// Rolls 3d6 for a given skill at a given chance.
// Returns the difference between the chance and the result of the roll.
// Returns SKILL_CRIT_SUCCESS (120) for critical successes, 
// SKILL_CRIT_FAILURE (-120) for critical failures, 
SInt8 SkillStore::DiceRoll(Character *ch, SInt16 &skillnum, SInt8 &chance, SkillKnown *i = NULL)
{
	SInt16 roll, pts = 0, critical = 0;

	if (chance == SKILL_NOCHANCE)	return SKILL_NOCHANCE;

	roll = dice(3, 6);		// The diceroll for skill success
	
	switch (roll) {
	case 3: 
	case 4: 
		return (SKILL_CRIT_SUCCESS);		// Critical success.	We assume dumb luck.
	case 5: 
	case 6:
		return MAX(MIN(chance - roll, 120), -120);
	case 17: 
	case 18:
		return (SKILL_CRIT_FAILURE);
	}
	
	return MAX(MIN(chance - roll, 9), -9);

}


// Rolls 3d6 for a given skill at a given chance.
// Returns the difference between the chance and the result of the roll.
// Returns SKILL_CRIT_SUCCESS (120) for critical successes, 
// SKILL_CRIT_FAILURE (-120) for critical failures, 
void SkillStore::RollToLearn(Character *ch, SInt16 skillnum, SInt16 roll, SkillKnown *i = NULL)
{
	SInt16 pts = 0;
	bool highertheory = false;

	// An internal enum for skill gain/loss.  Make sure all Losses come after Learns.
	enum {
		None,
		Learn,
		LearnByBotch,
		LearnByLesson,
		LoseByBotch
	} critical = None;
	
	if (LEARNABLE_SKILL(skillnum)) {
		if (!i)		FindSkill(skillnum, &i);

		if (i) {
			pts = i->proficient;
			if (i->theory > i->proficient)
				highertheory = true;
		}
	} else {
		return;
	}

	// This is to make sure that mobs that learn skills don't get set to
	// one point after going from a default.
	if (IS_NPC(ch) && (pts == 0)) {
		pts = gen_mob_skillpoints(ch, skillnum);
	} else {
		pts = 0;
	}

	switch (roll) {
	// case 3:		if ((GetSkillChance(ch, skillnum, i, true) < roll) && GET_INT(ch) >= dice(3, 6)) {
	//    				critical = Learn;
	//				}
	case 3:		if (GET_INT(ch) >= dice(3, roll) ) {
       				critical = Learn;
				}
				// break;	// Yes, we want the fallthrough
	case 4:		if (highertheory)	critical = LearnByLesson;	break;
	case 17:	if (GET_INT(ch) >= dice(3, 6))	critical = LearnByBotch;
				else							critical = LoseByBotch;
				break;
	}

	switch (critical) {
	case LearnByBotch: 
		LearnSkill(skillnum, i, pts + 1);
		ch->Sendf("&cW&bbThat didn't work!  "
					"You'll not botch %s like that again!&b0&c0\r\n",
					Skill::Name(skillnum));
		break;
	case LearnByLesson:
		LearnSkill(skillnum, i, pts + 1);		 // Critical success.	Learn from it!
		ch->Sendf("&cW&bbBrilliant move!  "
					"Your lessons in %s are paying off!&b0&c0\r\n",
					Skill::Name(skillnum));
		break;
	case Learn:
		LearnSkill(skillnum, i, pts + 1);		 // Critical success.	Learn from it!
		ch->Sendf("&cW&bbBrilliant move!  "
					"Your skill in %s is increasing!&b0&c0\r\n",
					Skill::Name(skillnum));
		break;
	case LoseByBotch:
		LearnSkill(skillnum, i, -1); // Uh oh, they lost confidence.
		ch->Sendf("&cY&brThat didn't work!  "
				"You feel less confident in your skill in %s!&b0&c0\r\n",
				Skill::Name(skillnum));
		break;
	default:
		break;
	}
}


// Uses the above functions to return the difference between a skill chance and a
// 3d6 roll.	A negative number is a failure.	Returns SKILL_NOCHANCE if the
// character just can't perform the skill.
int SkillStore::RollSkill(Character *ch, SInt16 skillnum)
{
	SInt8 chance = SKILL_NOCHANCE, mod = 0;
	SInt16 defaultnum;

	if (VALID_SKILL(skillnum) && !SKL_IS_STAT(skillnum) && 
		 !((1 << GET_POS(ch)) & Skills[skillnum]->min_position)) {
		return (-1);
	}

	FindBestRoll(ch, skillnum, defaultnum, chance, mod);

	if (chance == SKILL_NOCHANCE)		// No go.	They don't have the skill
		return (chance);

	if (!LEARNABLE_SKILL(skillnum))
		skillnum = defaultnum;

	return DiceRoll(ch, skillnum, chance);

}


int SkillStore::RollSkill(Character *ch, SInt16 skillnum, 
													SInt8 chance)
{
	if (VALID_SKILL(skillnum) && !SKL_IS_STAT(skillnum) && 
		 !((1 << GET_POS(ch)) & Skills[skillnum]->min_position)) {
		return (-1);
	}

	if (chance == SKILL_NOCHANCE)		// No go.	They don't have the skill
		return (chance);

	return DiceRoll(ch, skillnum, chance);

}


// This one is used for spells.
int SkillStore::RollSkill(Character *ch, SInt16 skillnum, SInt16 defaultnum,
													SInt8 chance)
{
	if (VALID_SKILL(skillnum) && !SKL_IS_STAT(skillnum) && 
		 !((1 << GET_POS(ch)) & Skills[skillnum]->min_position)) {
		return (-1);
	}

	if (chance == SKILL_NOCHANCE)		// No go.	They don't have the skill
		return (chance);

	return DiceRoll(ch, defaultnum, chance);

}


int SkillStore::RollSkill(Character *ch, SkillKnown *i)
{
	SInt16 skillnum;
	SInt8 chance;

	if (i == NULL)
		return (SKILL_NOCHANCE);

	if (VALID_SKILL(skillnum) && !SKL_IS_STAT(skillnum) && 
		 !((1 << GET_POS(ch)) & Skills[skillnum]->min_position)) {
		return (-1);
	}

	skillnum = i->skillnum;
	chance = GetSkillChance(ch, skillnum, i, TRUE);

	if (chance == SKILL_NOCHANCE)		// No go.	They don't have the skill
		return (chance);

	return DiceRoll(ch, skillnum, chance);

}


// A contest of skill between two characters.	This routine renders quicker
// resolutions using GURPS rules for highly skilled characters.	It returns
// 1 if offense succeeds, 0 if defense succeeds, and uses references for the
// success of both characters.
int SkillStore::SkillContest(Character *ch, SInt16 cskillnum, SInt8 cmod,
				 Character *victim, SInt16 vskillnum, SInt8 vmod,
				 SInt8 &offense, SInt8 &defense)
{
	SInt16 cdefaultnum = 0, vdefaultnum = 0;
	SInt8 scaledown = 0, cchance = SKILL_NOCHANCE, vchance = SKILL_NOCHANCE; // Used if both skills are over 14.

	offense = -100;
	defense = -100;

	// In cases of both characters having chances over 14 or more on the dice,
	// we speed things up by getting the difference between the highest skill and
	// 14, and reducing both skills by the difference.
	if (VALID_SKILL(cskillnum))
		FindBestRoll(ch, cskillnum, cdefaultnum, cchance, cmod);

	// The defender may not have a chance to roll a skill.	Doublecheck.
	if (VALID_SKILL(vskillnum))
		FindBestRoll(victim, vskillnum, vdefaultnum, vchance, vmod);
	
	// If both skill chances are 15 or more, this kicks in to lower the numbers
	// for a faster contest.
	if ((cchance > 14) && (vchance > 14)) {
		if (cchance > vchance)			 
			scaledown = cchance - 14; 			 
		else					 
			scaledown = vchance - 14;
	}

	if (cchance != SKILL_NOCHANCE)
		offense = DiceRoll(ch, cskillnum, cchance) + cmod - scaledown;

	if (vchance != SKILL_NOCHANCE)
		// Either the victim has no chance or has no available skill for defense.
		defense = DiceRoll(victim, vskillnum, vchance) + vmod - scaledown;

	if (offense == defense) {	// Equal success rolls mean failure for both.
		offense = -1;
		defense = -1;
	}

	if (offense > 0)	 // Return 1 if offense succeeds.	We'll figure defense
		return 1;				 // success in the calling routine.
	else
		return 0;
}


// Adds a skill to the character's list.	Returns 1 if successful, 0 if not.
// The newly created skill is returned in i.
int SkillStore::AddSkill(SInt16 skillnum, SkillKnown **i = NULL, UInt8 pts = 1, UInt8 theory = 0)
{
	SkillKnown *newskill = NULL, *prev = NULL, *j = NULL;
	int n = NUM_SKILLS + 1;
	
	if (pts <= 0)
		return 0;	// What's the point??

	if (!LEARNABLE_SKILL(skillnum))
		return 0;	// Skill can't be learned!
		
	// First get the skill pointer set.	Since skills are only added
	// to the list in ascending order of skillnums, we can break
	// if the skillnum exceeds the num we're searching for.

	CREATE(newskill, SkillKnown, 1);
	newskill->skillnum = skillnum;
	newskill->proficient = pts;
	newskill->theory = ((theory > 0) ? theory : pts);
	newskill->next = NULL;

	if (skills == NULL)	// This is the player's first skill.
		skills = newskill;
	else if (skills->skillnum > skillnum) {	// This is lower than the lowest skillnum
		newskill->next = skills;			// on the player's list.
		skills = newskill;
	} else {
		for (j = skills; j && (j->skillnum < skillnum) && n; prev = j, j = j->next, n--);
		
		if (!n)
			return 0;	// Uh oh.	We just barely avoided an infinite loop.

		if (j == NULL) {			// We're at the end of the list...
			prev->next = newskill;
			*i = j;
			return 1;
		}	

		if (j->skillnum == skillnum) {	// They've got this skill already!
			FREE(newskill);
			LearnSkill(skillnum, j, pts);
		} else {
			newskill->next = j;
			if (prev)
				prev->next = newskill;
		}

		if (j->skillnum != skillnum)
			return 0;
	}
	
	*i = j;

	return 1;
}


// Adds to a skill rating.	Returns 3 if skill maxes out, 2 if skill hits 0, 
// 1 if successful, 0 if not.
int SkillStore::LearnSkill(SInt16 skillnum, SkillKnown *i, SInt16 pts = 1)
{
	int change = 0;
	// First let's be sure we have a pointer to the skill.

	if (pts == 0)
		return 0;

	if ((!i) && (LEARNABLE_SKILL(skillnum)))
		FindSkill(skillnum, &i);
		
	if (!i) {
		if (pts > 0) {
			if (!AddSkill(skillnum, &i, pts))
				return 0;	// WHOA.	This shouldn't happen!
		} else {
			return 0;		// Can't lose points on a skill you don't have!
		}
	} else {
		if ((i->proficient + pts) > 255) {
			change = MAX(0, 255 - i->proficient);
			i->theory = i->proficient = 255;
			return (change);		// Maximum skill reached
		}
		if ((i->proficient + pts) < 0) {
			pts = i->proficient - 1;
            i->proficient = 1;
			return (-pts);		// Minimum skill reached, maybe the skill should be removed
		}
	
		i->proficient += pts;
		i->theory = (i->proficient > i->theory) ? i->proficient : i->theory;
	}

	return (pts);

}


// Returns the proficient point change, if any.
int SkillStore::LearnTheory(SInt16 skillnum, SkillKnown *i, SInt16 pts = 1)
{
	// First let's be sure we have a pointer to the skill.

	if (pts == 0)
		return 0;

	if ((!i) && (LEARNABLE_SKILL(skillnum)))
		FindSkill(skillnum, &i);
		
	if (!i) {
		if (pts > 0) {
			if (!AddSkill(skillnum, &i, 1, pts))
				return 1;	// WHOA.	This shouldn't happen!
		} else {
			return 0;		// Can't lose points on a skill you don't have!
		}
	} else {
		if ((i->theory + pts) > 255) {
			i->theory = 255;
			return (0);		// Maximum skill reached
		}
		if ((i->theory + pts) < 0) {
			pts = i->theory;
			return (0);		// Minimum skill reached, maybe the skill should be removed
		}
	
		i->theory += pts;
	}

	return (0);

}


int SkillStore::LearnSkill(Character *ch, SInt16 skillnum, 
													 SkillKnown *i, SInt16 pts = 1)
{
	int add;
	add = LearnSkill(skillnum, i, pts);
	return add;
}


int SkillStore::RemoveSkill(SInt16 skillnum)
{
	SkillKnown *j = NULL, *prev = NULL;
	int n = NUM_SKILLS + 1;

	if (!VALID_SKILL(skillnum))
		return 0;	// Skill does not exist!
	
	if (skills == NULL)
		return 0;
		
	// First get the skill pointer set.	Since skills are only added
	// to the list in ascending order of skillnums, we can break
	// if the skillnum exceeds the num we're searching for.

	if (skills->skillnum == skillnum) {
		j = skills->next;
		FREE(skills);
		skills = j;
		j = NULL;
	} else {
		for (j = skills; j && (j->skillnum < skillnum) && n; prev = j, j = j->next, n--);
		
		if (!n)
			return 0;	// Uh oh.	We just barely avoided an infinite loop.

		if (j == NULL)			// We're at the end of the list...
			return 0;

		if (j->skillnum == skillnum) {	// They've got this skill already!
			prev->next = j->next;
			FREE(j);
		} else {
			return 0;
		}
	}
	
	return 1;
}


int SkillStore::SetSkill(SInt16 skillnum, SkillKnown *i, UInt8 pts, UInt8 theory = 0)
{
	if (!LEARNABLE_SKILL(skillnum))
		return 0;

	// Let's be sure we have a pointer to the skill.
	if (!i)
		FindSkill(skillnum, &i);
		
	if (!i) {
		if (pts > 0) {
			if (!AddSkill(skillnum, &i, pts, theory))
				return 0;
			if (i) { // i might have a value now, thanks to AddSkill.
				i->proficient = pts;
				i->theory = (pts > theory) ? pts : theory ;
				return (pts);
			} else {
				return 0;
			}
		} else {
			return 0;		// Can't zero skill you don't have!
		}
	} else {
		if (pts == 0) {
			pts = -(i->proficient);
			RemoveSkill(skillnum);
		} else {
			int diff = pts - i->proficient;
			i->proficient = pts;
			i->theory = (pts > theory) ? pts : theory ;
			return (diff);
		}
	}
	
	return (pts);
}


int SkillStore::SetSkill(Character *ch, SInt16 skillnum, 
						SkillKnown *i, UInt8 pts, UInt8 theory = 0)
{
	int add;
	add = SetSkill(skillnum, i, pts, theory);
	return add;
}


// SInt8 skill_pts_to_mod(SInt16 &skillnum, UInt8 &pts)
SInt8 skill_pts_to_mod(SInt16 skillnum, UInt8 pts)
{
	SInt16 mod = Skills[skillnum]->startmod;
	UInt16 cost = 1;
	
	if (!pts)
		return (-50);

	while ((cost += MIN(cost, MAX(Skills[skillnum]->maxincr, 1))) <= pts)
		mod = MIN(++mod, 120);

	return (SInt8) mod;
}


// UInt8 skill_mod_to_pts(SInt16 &skillnum, SInt8 &mod)
UInt8 skill_mod_to_pts(SInt16 skillnum, SInt8 mod)
{
	SInt8 i;
	UInt8 pts = 1;
	
	for(i = Skills[skillnum]->startmod; i < mod; ++i)
		pts += MIN(pts, Skills[skillnum]->maxincr);
		
	return (pts);
}


// Get the points needed to raise a skill modifier, taking defaults into account.
UInt8 skill_lvl_to_pts(Character *ch, SInt16 skillnum)
{
	SInt16 i, chance = 0;
	UInt8 pts = 1;

	chance = CHAR_SKILL(ch).GetSkillChance(ch, skillnum, NULL, 1);
	
	for(i = Skills[skillnum]->startmod; i < chance; i++)
		pts += MIN(pts, Skills[skillnum]->maxincr);
		
	return (pts);
}


int SkillStore::TotalPoints(Character *ch)
{
	SkillKnown *tskill = NULL;
	int sum = 0;

	for (tskill = skills; tskill; tskill = tskill->next) {
		// This discounts generic skills
		if (SKL_IS_COMBATSKILL(tskill->skillnum) || 
				!SKL_IS_SKILL(tskill->skillnum)) {
			sum += tskill->proficient;
			if (IS_NPC(ch))
				sum -= gen_mob_skillpoints(ch, tskill->skillnum);
		}
	}

	return sum;
}


int get_skill_pts(Character *ch, SInt16 skillnum, SkillKnown *i)
{
	int pts = 0;

	if ((!i) && (LEARNABLE_SKILL(skillnum)))
		CHAR_SKILL(ch).FindSkill(skillnum, &i);

	if (i)
		pts = i->proficient;
	else if (IS_NPC(ch))
		pts = gen_mob_skillpoints(ch, skillnum);

	return pts;
}


Ptr	Skill::GetPointer(int arg)
{
	switch (arg) {
	case 0:					return ((Ptr) (&name));			break;
	case 1:					return ((Ptr) (&delay));		break;
	case 2:					return ((Ptr) (&lag));			break;
	case 3:					return ((Ptr) (&wearoff));		break;
	case 4:					return ((Ptr) (&wearoffroom));	break;
	case 5:					return ((Ptr) (&types));		break;
	case 6:					return ((Ptr) (&stats));		break;
	case 7:					return ((Ptr) (&min_position));	break;
	case 8:					return ((Ptr) (&target));		break;
	case 9:					return ((Ptr) (&routines));		break;
	case 10:				return ((Ptr) (&mana));			break;
	case 11:				return ((Ptr) (&startmod));		break;
	case 12:				return ((Ptr) (&maxincr));		break;
	case 13:				return ((Ptr) (&violent));		break;
	case 14:				return ((Ptr) (&learnable));		break;
	case 15:				return ((Ptr) (&defaults));			break;
	case 16:				return ((Ptr) (&prerequisites));	break;
	case 17:				return ((Ptr) (&anrequisites));		break;
	}

	return NULL;
}


const struct olc_fields * Skill::Constraints(int arg)
{
	static const struct olc_fields constraints[] = 
	{
		{ "name", 			0, 			Datatypes::CStringLine,	0, 0, 0, NULL	},
		{ "delay", 			EDT_SINT32,	Datatypes::Integer,		0, 0, 20, NULL	},
		{ "lag", 			EDT_SINT32,	Datatypes::Integer,		0, 0, 20, NULL	},
		{ "wearoff", 		0, 			Datatypes::CStringLine,	0, 0, 0, NULL	},
		{ "wearoffroom", 	0, 			Datatypes::CStringLine,	0, 0, 0, NULL	},
		{ "types", 			EDT_UINT32,	Datatypes::Bitvector,	0, 0, 0, (const char **) namedtypes	},
		{ "stats", 			EDT_UINT32,	Datatypes::Bitvector,	0, 0, 0, (const char **) named_bases	},
		{ "position", 		EDT_UINT32,	Datatypes::Bitvector,	0, 0, 0, (const char **) position_block	},
		{ "target", 		EDT_UINT32,	Datatypes::Bitvector,	0, 0, 0, (const char **) targeting	},
		{ "routines", 		EDT_UINT32,	Datatypes::Bitvector,	0, 0, 0, (const char **) spell_routines	},
		{ "mana", 			EDT_SINT16,	Datatypes::Integer,		0, 0, 10000, NULL	},
		{ "startmod", 		EDT_SINT8,	Datatypes::Integer,		0, -50, 10, NULL	},
		{ "maxincr", 		EDT_SINT8,	Datatypes::Integer,		0, 1, 32, NULL	},
		{ "violent", 		0, 			Datatypes::Bool,		0, 0, 0, NULL	},
		{ "learnable", 		0, 			Datatypes::Bool,		0, 0, 0, NULL	},
// 		{ "default", 		0, 			Datatypes::SkillRef + Datatypes::Vector,		0, 0, 0, NULL	},
// 		{ "prereq", 		0, 			Datatypes::SkillRef + Datatypes::Vector,		0, 0, 0, NULL	},
// 		{ "anreq", 			0, 			Datatypes::SkillRef + Datatypes::Vector,		0, 0, 0, NULL	},
// 		{ "properties", 	0, 			Datatypes::Property + Datatypes::Vector + Datatypes::Pointer,		
//										0, 0, 0, NULL	},
		{ "\n", 0, 0, 0, 0, 0, NULL }
	};

	if (arg >= 0 && arg < 14)
		return &(constraints[arg]);

		return NULL;
}


void Skill::SaveDisk(FILE *fp, int indent = 1)
{
	int				i;
	char			*ptr = NULL,
					*buf = get_buffer(MAX_STRING_LENGTH),
					*name = get_buffer(MAX_STRING_LENGTH),
					*indentation = get_buffer(indent + 2);

	for (i = 0; i < indent; ++i)
		strcat(indentation, "\t");

	::Editable::SaveDisk(fp, indent);

	for (i = 0; i < defaults.Count(); ++i) {
		strcpy(name, Skill::Name(defaults[i].skillnum));
		while ((ptr = strchr(name, ' ')) != NULL)
			*ptr = '_';
		fprintf(fp, "%sdefault = %s %d;\n", indentation, name, defaults[i].chance);
	}
	for (i = 0; i < prerequisites.Count(); ++i) {
		strcpy(name, Skill::Name(prerequisites[i].skillnum));
		while ((ptr = strchr(name, ' ')) != NULL)
			*ptr = '_';
		fprintf(fp, "%sprereq = %s %d;\n", indentation, name, prerequisites[i].chance);
	}
	for (i = 0; i < anrequisites.Count(); ++i) {
		strcpy(name, Skill::Name(anrequisites[i].skillnum));
		while ((ptr = strchr(name, ' ')) != NULL)
			*ptr = '_';
		fprintf(fp, "%sanreq = %s %d;\n", indentation, name, anrequisites[i].chance);
	}
	properties.SaveDisk(fp);
	
	release_buffer(buf);
	release_buffer(name);
	release_buffer(indentation);
}
