/* ************************************************************************
*   File: constants.c                                   Part of CircleMUD *
*  Usage: Numeric and string contants used by the MUD                     *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */



#include "structs.h"
#include "constants.h"
#include "skills.defs.h"

const char circlemud_version[] = {
"Struggle for Nyrilis, based on the LexiMUD engine,\r\n"
"compiled on " __DATE__ " at " __TIME__ "\r\n"};

/* strings corresponding to ordinals/bitvectors in structs.h ***********/


/* (Note: strings for class definitions in class.c instead of here) */


const char *bool_conditions[] = {
	"false",
	"true",
	"\n"
};


/* cardinal directions */
const char *dirs[] = {
	"north",
	"east",
	"south",
	"west",
	"up",
	"down",
	"\n"
};


const char *from_dir[] = {
	"the south",
	"the west",
	"the north",
	"the east",
	"below",
	"above",
	"\n"
};


/* cardinal directions */
const char *thedirs[] =
{
	"the north",
	"the east",
	"the south",
	"the west",
	"above",
	"below",
	"\n"
};


/* ROOM_x */
const char *room_bits[] = {
	"DARK",
	"DEATH",
	"!MOB",
	"INDOORS",
	"PEACEFUL",
	"SOUNDPROOF",
	"!TRACK",
	"PARSER",
	"TUNNEL",
	"PRIVATE",
	"STAFF-ONLY",
	"HOUSE",
	"HCRSH",
	"!MAGIC",
	"Unused14",
	"*",				/* BFS MARK */
	"!VEHICLES",
	"SRSTAFF-ONLY",
	"GRAVITY",
	"MAP",
	"VACUUM",					/* 20 */
	"JAIL",
	"Unused22",
	"Unused23",
	"Unused24",
	"Unused25",				/* 25 */
	"Unused26",
	"Unused27",
	"Unused28",
	"Unused29",
	"Unused30",				/* 30 */
	"DELETED",
	"\n"
};


/* EX_x */
const char *exit_bits[] = {
	"CLOSEABLE",
	"CLOSED",
	"LOCKED",
	"PICKPROOF",
	"HIDDEN",
	"NOSHOOT",
	"VISTA",
	"NOMOB",
	"NOVEHICLES",
	"SEETHRU",
	"AUTOMATIC",
	"\n"
};


/* SECT_ */
const char *sector_types[] = {
	"Inside",
	"City",
	"Field",
	"Forest",
	"Hills",
	"Mountains",
	"Water (Swim)",
	"Water (No Swim)",
	"Underwater",
	"Flight",
	"Road",
	"Desert",
	"Jungle",
	"Swamp",
	"Arctic",
	"High Mountains",
	"Underground",
	"Vacuum",
	"Space",
	"DeepSpace",
	"\n"
};


/* SEX_x */
const char *genders[] = {
	"Neutral",
	"Male",
	"Female",
	"\n"
};


/* POS_x */
const char *position_types[] = {
	"Dead",
	"Mortally wounded",
	"Incapacitated",
	"Stunned",
	"Sleeping",
	"Resting",
	"Sitting",
	"Fighting",
	"Standing",
	"\n"
};


/* POS_x */
const char *positions[] = {
	"dead",
	"mortally wounded",
	"incapacitated",
	"stunned",
	"sleeping",
	"resting",
	"sitting",
	"fighting",
	"standing",
	"\n"
};

/* POS_x */
const char *position_block[] = {
	"DEAD",
	"MORTAL",
	"INCAP",
	"STUN",
	"SLEEP",
	"REST",
	"SIT",
	"FIGHT",
	"STAND",
	"\n"
};


/* PLR_x */
const char *player_bits[] = {
	"KILLER",
	"THIEF",
	"FROZEN",
	"DONTSET",
	"WRITING",
	"MAILING",
	"CRASH",
	"SITEOK",
	"NOSHOUT",
	"NOTITLE",
	"UNJUST",
	"DELETED",
	"LOADROOM",
	"NODELETE",
	"INVSTART",
	"CRYO",
	"AFK",
	"TELEPATHY",
	"",
	"NOWIZLIST",
	"DOZY",
	"JAILED",
	"\n"
};


/* MOB_x */
const char *action_bits[] = {
	"SPEC",
	"SENTINEL",
	"SCAVENGER",
	"ISNPC",
	"AWARE",
	"AGGRESSIVE",
	"STAY-ZONE",
	"WIMPY",
	"AGGR_GOOD",
	"AGGR_NEUTRAL",
	"AGGR_EVIL",
	"AGGR_ALL",
	"MEMORY",
	"HELPER",
	"!CHARM",
	"!SUMMON",
	"!SLEEP",
	"!BASH",
	"!BLIND",
	"!SHOOT",
	"HELPRACE",
	"HELPALIGN",
	"HUNTER",
	"SEEKER",
	"MERCIFUL",
	"STAY-SECTOR",
	"Unused26",
	"Unused27",
	"Unused28",
	"APPROVED",
	"KILLER",
	"DELETED",
	"\n"
};


/* PRF_x */
const char *preference_bits[] = {
	"BRIEF",			//	0
	"COMPACT",
	"STAFFINVIS",
	"ADMININVIS",
	"AUTOEXIT",
	"!HASS",
	"!SUMMON",
	"!REPEAT",
	"HOLYLIGHT",
	"COLOR",
	"LOG1",
	"LOG2",
	"ROOMFLAGS",
	"INVULN",
	"AUTOLOOT",
	"AUTOSPLIT",
	"MERCY",
	"AUTOASSIST",
	"AUTOSWITCH",
	"SOUND",
	"QUEST",
	"\n"
};


const char *channel_bits[] = {
	"!SHOUT",
	"!TELL",
	"!CHAT",
	"MISSION",
	"!MUSIC",
	"!GRATZ",
	"!INFO",
	"!WIZ",
	"!RACE",
	"!CLAN",
	"\n"
};


/* AFF_x */
const char *affected_bits[] = {
	"BLIND",
	"INVIS",
	"IMMOBILIZED",
	"DET-INVIS",
	"DET-MAGIC",
	"SENSELIFE",
	"FLY",
	"SANCT",
	"GROUP",
	"MAGESHIELD",
	"INFRA",
	"PROT-EVIL",
	"DET-ALIGN",
	"SLEEP",
	"NOTRACK",
	"LIGHT",
	"REGEN",
	"SNEAK",
	"HIDE",
	"VACUUM-OK",
	"CHARM",
	"SILENCE",
	"HASTE",
	"BLADE",
	"FIRESHIELD",
	"TRACKING",
	"POISON",
	"CAMO",
	"VAMPREGEN",
	"BERSERK",
	"\n"
};


/* CON_x */
const char *connected_types[] = {
	"Playing",
	"Disconnecting",
	"Get name",
	"Confirm name",
	"Get password",
	"Get new PW",
	"Conf. new PW",
	"Enter SDesc",
	"Select sex",
	"Select race",
	"Reading MOTD",
	"Main Menu",
	"Get descript.",
	"Chg PW 1",
	"Chg PW 2",
	"Chg PW 3",
	"Self-Delete 1",
	"Self-Delete 2",
	"New Message",
	"Object edit",
	"Room edit",
	"Zone edit",
	"Mobile edit",
	"Shop edit",
	"Action edit",
	"Help edit",
	"Skill edit",
	"Clan edit",
	"Text Editor",
	"Script edit",
	"Disconnect",
	"Ident",
	"Get Email",
	"CG Main",
	"CG Save",
	"CG Str",
	"CG Int",
	"CG Wis",
	"CG Dex",
	"CG Con",
	"CG Cha",
	"CG Align",
	"CG Lawful",
	"CG Hair",
	"CG Eyes",
	"CG Adv",
	"CG Disadv",
	"QANSI",
	"CG Class",
	"\n"
};


#if 0
/* WEAR_x - for eq list */
const char *where[] = {
	"<worn on finger>     ",
	"<worn on finger>     ",
	"<worn around neck>   ",
	"<worn on body>       ",
	"<worn on head>       ",
	"<worn on legs>       ",
	"<worn on feet>       ",
	"<worn on hands>      ",
	"<worn on arms>       ",
	"<worn about body>    ",
	"<worn about waist>   ",
	"<worn around wrist>  ",
	"<worn around wrist>  ",
	"<worn over eyes>     ",
	"ERROR - PLEASE REPORT",
	"ERROR - PLEASE REPORT",
	"<wielded two-handed> ",
	"<held two-handed>    ",
	"<wielded>            ",
	"<wielded off-hand>   ",
	"<held as light>      ",
	"<held>               "
};
#endif


/* WEAR_x - for stat */
const char *equipment_types[] = {
	"right ring finger",
	"left ring finger",
	"third ring finger",
	"fourth ring finger",
	"neck",
	"neck",
	"body",
	"head",
	"face",
	"eyes",
	"right ear",
	"left ear",
	"legs",
	"feet",
	"upper hands",
	"lower hands",
	"upper arms",
	"lower arms",
	"body",
	"back",
	"waist",
	"waist",
	"right wrist",
	"left wrist",
	"third wrist",
	"fourth wrist",
	"right hand",
	"left hand",
	"third hand",
	"fourth hand",
	"\n"
};


/* ITEM_x (ordinal object types) */
const char *item_types[] = {
	"UNDEFINED",		//	0
	"LIGHT",
	"SCROLL",
	"WAND",
	"STAFF",
	"WEAPON",
	"FIREWEAPON",
	"MISSILE",
	"TREASURE",
	"ARMOR",
	"POTION",
	"REAGENT",
	"TALISMAN",
	"SPELLBOOK",
	"TRAP",
	"CONTAINER",
	"NOTE",
	"DRINKCON",
	"KEY",
	"FOOD",
	"MONEY",
	"PEN",
	"BOAT",
	"FOUNTAIN",
	"GRENADE",
	"WEAPONCON",
	"ATTACHMENT",
	"BOARD",
	"FURNITURE",
	"VEHICLE",
	"V_HATCH",
	"V_CONTROLS",
	"V_WINDOW",
//	"V_WEAPON",
	"CURRENCY",
	"\n"
};


/* ITEM_WEAR_ (wear bitvector) */
const char *wear_bits[] = {
	"TAKE",
	"FINGER",
	"NECK",
	"BODY",
	"HEAD",
	"LEGS",
	"FEET",
	"HANDS",
	"ARMS",
	"SHIELD",
	"ABOUT",
	"WAIST",
	"WRIST",
	"WIELD",
	"HOLD",
	"BACK",
	"ONBELT",
	"FACE",
	"EYES",
	"EARS",
	"SHOULDER",
	"SYMBIOT",
	"\n"
};


/* ITEM_x (extra bits) */
const char *extra_bits[] = {
	"HUM",
	"!RENT",
	"!DONATE",
	"!INVIS",
	"INVISIBLE",
	"MAGIC",
	"!DROP",
	"BLESS",
	"!GOOD",
	"!EVIL",
	"!NEUTRAL",
	"!SELL",
	"!PURGE",
	"2-HAND",
	"HIDDEN",
	"PROXIMITY",
	"GLOW",
	"MOVEABLE",
	"NOLOSE",
	"Unused19",
	"Unused20",				/* 20 */
	"Unused21",
	"Unused22",
	"Unused23",
	"Unused24",
	"Unused25",				/* 25 */
	"Unused26",
	"Unused27",
	"Unused28",
	"Unused29",
	"APPROVED",				/* 30 */
	"DELETED",
	"\n"
};


/* APPLY_x */
const char *apply_types[] = {
	"NONE",
	"STR",
	"INT",
	"WIS",
	"DEX",
	"CON",
	"CHA",
	"WEIGHT",
	"HEIGHT",
	"AGE",
	"MAXHIT",
	"MAXMANA",
	"MAXMOVE",
	"PDEFENSE",
	"DAMRESIST",
	"GOLD",
	"HITROLL",
	"DAMROLL",
	"SAVEFIRE",
	"SAVEELECTRICITY",
	"SAVEACID",
	"SAVEFREEZING",
	"SAVEPOISON",
	"SAVEPSIONIC",
	"SAVEVIM",
	"SAVESPELL",
	"SAVEPETR",
	"\n"
};


/* CONT_x */
const char *container_bits[] = {
	"CLOSEABLE",
	"PICKPROOF",
	"CLOSED",
	"LOCKED",
	"\n",
};


/* LIQ_x */
const char *drinks[] =
{
	"water",
	"beer",
	"wine",
	"ale",
	"dark ale",
	"whisky",
	"lemonade",
	"firebreather",
	"local speciality",
	"slime mold juice",
	"milk",
	"tea",
	"coffee",
	"blood",
	"salt water",
	"clear water",
	"fey wine",
	"mountain dew",
	"\n"
};


/* other constants for liquids ******************************************/


char *drinknames[] = {
	"water",
	"beer",
	"wine",
	"ale",
	"darkale",
	"whisky",
	"lemonade",
	"firebreather",
	"speciality",
	"slimemold",
	"milk",
	"tea",
	"coffee",
	"blood",
	"saltwater",
	"clearwater",
	"feywine",
	"mountaindew",
	"\n"
};


/* effect of drinks on hunger, thirst, and drunkenness -- see values.doc */
int drink_aff[][3] = {
	{	1, 10, 0	},	// water
	{	2, 5, 3		},	 // beer
	{	2, 5, 5		},	 // wine
	{	2, 5, 2		},	 // ale
	{	2, 5, 1		},	 // ale
	{	1, 4, 6		},	 // whisky
	{	1, 8, 0		},	 // lemonade
	{	0, 0, 10	},	// firebreather
	{	3, 3, 3		},	 // local
	{	4, -8, 0	},	// juice
	{	3, 6, 0		},	 // milk
	{	1, 6, 0		},	 // tea
	{	1, 6, 0		},	 // coffee
	{	2, -1, 0	},	// blood
	{	1, -2, 0	},	// salt
	{	0, 13, 0	},	// water
	{	0, 3, 12	},	// feywine
	{	0, 2, 0		}  // dew
};


/* color of the various drinks */
const char *color_liquid[] =
{
	"clear",
	"brown",
	"clear",
	"brown",
	"dark",
	"golden",
	"red",
	"green",
	"clear",
	"light green",
	"white",
	"brown",
	"black",
	"red",
	"clear",
	"crystal clear",
	"glowing lavender",
	"fizzy yellow-green",
	"\n"
};


/* level of fullness for drink containers */
const char *fullness[] = {
	"less than half ",
	"about half ",
	"more than half ",
	""
};


const int rev_dir[] = {
	2,
	3,
	0,
	1,
	5,
	4
};


const int movement_loss[] =
{
	1,	/* Inside		 */
	1,	/* City			 */
	2,	/* Field			*/
	3,	/* Forest		 */
	4,	/* Hills			*/
	6,	/* Mountains	*/
	4,	/* Swimming	 */
	4,	/* Unswimable */
	5,	/* Underwater */
	1,	/* Flying		 */
	1,	/* Road */
	3,	/* Desert */
	4,	/* Jungle */
	4,	/* Swamp */
	3,	/* Arctic */
	3,	/* Impassable Mountain */
	2,	/* Underground */
	0,	/* Vacuum */
	0,	/* Space */
	0	/* Deep Space */
};


const char *weekdays[] = {
	"Sunday",
	"Monday",
	"Tuesday",
	"Wednesday",
	"Thursday",
	"Friday",
	"Saturday"
};


const char *month_name[] = {
	"January",		/* 0 */
	"February",
	"March",
	"April",
	"May",
	"June",
	"July",			/* 6 */
	"August",
	"September",
	"October",
	"November",
	"December",		/* 11 */
};


char * level_string[1] = {
	"           "
//	"Terraformer",
//	"Security/PR",
//	"   Admin   ",
//	"   Coder   "
};


char * dir_text[NUM_OF_DIRS] = {
	"the north",
	"the east",
	"the south",
	"the west",
	"above",
	"below",
};


const char * dir_text_to[NUM_OF_DIRS] = {
	"to the north",
	"to the east",
	"to the south",
	"to the west",
	"above",
	"below"
};


const char * dir_text_to_of[NUM_OF_DIRS] = {
	"to the north of",
	"to the east of",
	"to the south of",
	"to the west of",
	"above",
	"below"
};


const char relation_colors[] = "nyr";


const char *relations[] = {
	"friend",
	"neutral",
	"enemy",
	"\n"
};


const char *staff_bits[] = {
	"GENERAL",
	"ADMIN",
	"SECURITY",
	"GAME",
	"HOUSES",
	"CHARS",
	"CLANS",
	"MOBS",
	"OBJECTS",
	"ROOMS",
	"OLCADMIN",
	"SOCIALS",
	"HELP",
	"SHOPS",
	"SCRIPTS",
	"IMC",
	"CODER",
	"WIZNET",
	"INVULN",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"\n"
};


// Various strings associated with equipping an object.  I put these
// in as an array of structs to make adding new elements easier. -DH
const struct wear_on_message wear_slot_messages[MAX_HANDWEAR_MSG] = {

  { "worn on", "slides $p on to", "slide $p on to" },
  { "worn around", "wears $p around", "wear $p around" },
  { "worn about",  "wears $p about", "wear $p about" },
  { "worn on", "wears $p on", "wear $p on" },
  { "worn over", "wears $p over", "wear $p over" },
  { "worn from", "wears $p from", "wear $p from" },
  { "worn on", "puts $p on", "put $p on" },
  { "engulfed", "engulf $p in", "engulfs $p in" },
  { "held in", "grabs $p with", "grab $p with" },
  { "used as light", "lights $p and holds it in", "light $p and hold it in" },
  { "slung over", "slings $p over", "sling $p over" },
  { "attached to", "attaches $p to", "attach $p to" },
  { "wielded in", "wields $p in", "wield $p in" },
  { "strapped to", "straps $p to", "strap $p to" }
};


// Echoed to character along with a part name.  Made separate from
// wear_slot_messages for extra flexibility.
const char *already_wearing[MAX_ALREADY_WEARING] = {
  "You can't wear anything else around your",
  "You're already wearing something around your",
  "You're already wearing something over your",
  "You're already wearing something on your",
  "You're already wearing something on both of your",
  "You're already wearing something on all of your",
  "You're already wearing something about your",
  "You already have something on your",
  "You already have something around your",
  "You already have something attached to your",
  "You're already holding something in your",
  "You're already holding something in both"
};


// Note:  For all slots WEAR_HAND_R and above, the keyword is echoed before
// the partname on equipment lists and wear strings.  Hence, we have
// "hand", "primary" and "claw", "fourth".

// Format:    2d6 hit table location,
//            Damage multiplication - random number from 1 to X
//            Intrinsic damage resistance
//            Damage maximum for slot
//	      messages used from wear_on_message,
//	      messages used from already_wearing,
//	      item type flags wearable,
//	      partname,
//	      keyword

const struct body_wear_slots wear_slots[MAX_BODYTYPE][NUM_WEARS] = {

  { // Humanoid Body
    {  0, 1, 0, 20, WEAR_MSG_SLIDEONTO, WEARING_SOMETHING_AROUND, ITEM_WEAR_FINGER, "right ring finger", "r-finger" },
    {  0, 1, 0, 20, WEAR_MSG_SLIDEONTO, WEARING_SOMETHING_AROUND, ITEM_WEAR_FINGER, "left ring finger", "l-finger" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    {  2, 4, 0, 100, WEAR_MSG_WEARAROUND, WEAR_ANYTHING_ELSE, ITEM_WEAR_NECK, "neck", "neck" },
    {  2, 4, 0, 100, WEAR_MSG_WEARAROUND, WEAR_ANYTHING_ELSE, ITEM_WEAR_NECK, "neck", "neck" },
    {  8, 3, 0, 100, WEAR_MSG_WEARON, WEARING_SOMETHING_ON, ITEM_WEAR_BODY, "body", "body" },
    {  3, 2, 2, 100, WEAR_MSG_WEARON, WEARING_SOMETHING_ON, ITEM_WEAR_HEAD, "head", "head" },
    {  3, 2, 0, 100, WEAR_MSG_WEARON, WEARING_SOMETHING_ON, ITEM_WEAR_FACE, "face", "face" },
    {  3, 4, 0, 100, WEAR_MSG_WEAROVER, WEARING_SOMETHING_OVER, ITEM_WEAR_EYES, "eyes", "eyes" },
    {  0, 1, 0, 20, WEAR_MSG_PUTON, WEARING_SOMETHING_ON, ITEM_WEAR_EAR, "right ear", "r-ear" },
    {  0, 1, 0, 20, WEAR_MSG_PUTON, WEARING_SOMETHING_ON, ITEM_WEAR_EAR, "left ear", "l-ear" },
    { 11, 1, 0, 60, WEAR_MSG_WEARON, WEARING_SOMETHING_ON, ITEM_WEAR_LEGS, "legs", "legs" },
    { 12, 1, 0, 40, WEAR_MSG_WEARON, WEARING_SOMETHING_ON, ITEM_WEAR_FEET, "feet", "feet" },
    {  5, 1, 0, 30, WEAR_MSG_WEARON, WEARING_SOMETHING_ON, ITEM_WEAR_HANDS, "hands", "hands" },
    {  0, 3, 0, 100, WEAR_MSG_SLUNGOVER, WEARING_SOMETHING_OVER, ITEM_WEAR_SHOULDER, "shoulder", "shoulder" },
    {  4, 1, 0, 40, WEAR_MSG_WEARON, WEARING_SOMETHING_ON, ITEM_WEAR_ARMS, "arms", "arms" },
    { 10, 3, 0, 100, WEAR_MSG_WEARABOUT, WEARING_SOMETHING_AROUND, ITEM_WEAR_WAIST, "waist", "waist" },
    {  7, 3, 0, 100, WEAR_MSG_WEARABOUT, WEARING_SOMETHING_ABOUT, ITEM_WEAR_ABOUT, "body", "about" },
    {  0, 3, 0, 100, WEAR_MSG_STRAPTO, WEARING_SOMETHING_ON, ITEM_WEAR_BACK, "back", "back" },
    {  0, 2, 0, 100, WEAR_MSG_WEARFROM, WEARING_SOMETHING_ON, ITEM_WEAR_ONBELT, "waist", "fromwaist" },
    {  0, 2, 0, 100, WEAR_MSG_WEARFROM, WEARING_SOMETHING_ON, ITEM_WEAR_ONBELT, "waist", "fromwaist" },
    {  6, 1, 0, 50, WEAR_MSG_WEARAROUND, WEARING_SOMETHING_AROUND, ITEM_WEAR_WRIST, "right wrist", "r-wrist" },
    {  9, 1, 0, 50, WEAR_MSG_WEARAROUND, WEARING_SOMETHING_AROUND, ITEM_WEAR_WRIST, "left wrist", "l-wrist" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    {  0, 1, 0, 60, WEAR_MSG_HELDIN, HOLDING_SOMETHING_IN, (ITEM_WEAR_TAKE | ITEM_WEAR_SHIELD | ITEM_WEAR_WIELD), "hand", "primary" },
    {  0, 1, 0, 60, WEAR_MSG_HELDIN, HOLDING_SOMETHING_IN, (ITEM_WEAR_TAKE | ITEM_WEAR_SHIELD | ITEM_WEAR_WIELD), "hand", "off" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" }
  },
  { // Insectoid Body
    {  0, 1, 0, 100, WEAR_MSG_SLIDEONTO, WEARING_SOMETHING_AROUND, ITEM_WEAR_FINGER, "first finger", "1-finger" },
    {  0, 1, 0, 100, WEAR_MSG_SLIDEONTO, WEARING_SOMETHING_AROUND, ITEM_WEAR_FINGER, "second finger", "2-finger" },
    {  0, 1, 0, 100, WEAR_MSG_SLIDEONTO, WEARING_SOMETHING_AROUND, ITEM_WEAR_FINGER, "third finger", "3-finger" },
    {  0, 1, 0, 100, WEAR_MSG_SLIDEONTO, WEARING_SOMETHING_AROUND, ITEM_WEAR_FINGER, "fourth finger", "4-finger" },
    {  2, 1, 0, 100, WEAR_MSG_WEARAROUND, WEAR_ANYTHING_ELSE, ITEM_WEAR_NECK, "neck", "neck" },
    {  2, 1, 0, 100, WEAR_MSG_WEARAROUND, WEAR_ANYTHING_ELSE, ITEM_WEAR_NECK, "neck", "neck" },
    {  9, 1, 0, 100, 0, 0, 0, "thorax", "thorax" },
    {  3, 1, 0, 100, 0, 0, 0, "head", "head" },
    {  3, 1, 0, 100, WEAR_MSG_WEARON, WEARING_SOMETHING_ON, ITEM_WEAR_FACE, "face", "face" },
    {  3, 1, 0, 100, WEAR_MSG_WEAROVER, WEARING_SOMETHING_OVER, ITEM_WEAR_EYES, "eyes", "eyes" },
    {  0, 1, 0, 100, WEAR_MSG_PUTON, WEARING_SOMETHING_ON, ITEM_WEAR_EAR, "right antenna", "r-antenna" },
    {  0, 1, 0, 100, WEAR_MSG_PUTON, WEARING_SOMETHING_ON, ITEM_WEAR_EAR, "left antenna", "l-antenna" },
    { 11, 1, 0, 100, 0, 0, 0, "legs", "legs" },
    { 12, 1, 0, 100, 0, 0, 0, "feet", "feet" },
    {  5, 1, 0, 100, 0, 0, 0, "claws", "u-claws" },
    {  5, 1, 0, 100, 0, 0, 0, "claws", "l-claws" },
    {  4, 1, 0, 100, 0, 0, 0, "arms", "u-arms" },
    {  4, 1, 0, 100, 0, 0, 0, "arms", "l-arms" },
    {  7, 1, 0, 100, WEAR_MSG_WEARABOUT, WEARING_SOMETHING_ABOUT, ITEM_WEAR_ABOUT, "thorax", "about" },
    {  0, 1, 0, 100, WEAR_MSG_STRAPTO, WEARING_SOMETHING_ON, ITEM_WEAR_BACK, "back carapace", "back" },
    { 10, 1, 0, 100, WEAR_MSG_WEARABOUT, WEARING_SOMETHING_AROUND, ITEM_WEAR_WAIST, "waist", "waist" },
    {  0, 1, 0, 100, WEAR_MSG_WEARFROM, WEARING_SOMETHING_ON, ITEM_WEAR_ONBELT, "waist", "fromwaist" },
    {  6, 1, 0, 100, WEAR_MSG_WEARAROUND, WEARING_SOMETHING_AROUND, ITEM_WEAR_WRIST, "first wrist", "ur-wrist" },
    {  8, 1, 0, 100, WEAR_MSG_WEARAROUND, WEARING_SOMETHING_AROUND, ITEM_WEAR_WRIST, "second wrist", "ul-wrist" },
    {  6, 1, 0, 100, WEAR_MSG_WEARAROUND, WEARING_SOMETHING_AROUND, ITEM_WEAR_WRIST, "third wrist", "lr-wrist" },
    {  8, 1, 0, 100, WEAR_MSG_WEARAROUND, WEARING_SOMETHING_AROUND, ITEM_WEAR_WRIST, "fourth wrist", "ll-wrist" },
    {  6, 1, 0, 100, WEAR_MSG_HELDIN, HOLDING_SOMETHING_IN, (ITEM_WEAR_TAKE | ITEM_WEAR_SHIELD | ITEM_WEAR_WIELD), "claw", "first" },
    {  8, 1, 0, 100, WEAR_MSG_HELDIN, HOLDING_SOMETHING_IN, (ITEM_WEAR_TAKE | ITEM_WEAR_SHIELD | ITEM_WEAR_WIELD), "claw", "second" },
    {  6, 1, 0, 100, WEAR_MSG_HELDIN, HOLDING_SOMETHING_IN, (ITEM_WEAR_TAKE | ITEM_WEAR_SHIELD | ITEM_WEAR_WIELD), "claw", "third" },
    {  8, 1, 0, 100, WEAR_MSG_HELDIN, HOLDING_SOMETHING_IN, (ITEM_WEAR_TAKE | ITEM_WEAR_SHIELD | ITEM_WEAR_WIELD), "claw", "fourth" }
  },
  { // Lizardman Body
    {  0, 1, 0, 20, WEAR_MSG_SLIDEONTO, WEARING_SOMETHING_AROUND, ITEM_WEAR_FINGER, "right ring claw", "r-claw" },
    {  0, 1, 0, 20, WEAR_MSG_SLIDEONTO, WEARING_SOMETHING_AROUND, ITEM_WEAR_FINGER, "left ring claw", "l-claw" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    {  2, 3, 0, 100, WEAR_MSG_WEARAROUND, WEAR_ANYTHING_ELSE, ITEM_WEAR_NECK, "neck", "neck" },
    {  2, 3, 0, 100, WEAR_MSG_WEARAROUND, WEAR_ANYTHING_ELSE, ITEM_WEAR_NECK, "neck", "neck" },
    {  8, 2, 0, 100, WEAR_MSG_WEARON, WEARING_SOMETHING_ON, ITEM_WEAR_BODY, "body", "body" },
    {  3, 2, 2, 100, WEAR_MSG_WEARON, WEARING_SOMETHING_ON, ITEM_WEAR_HEAD, "head", "head" },
    {  3, 2, 0, 100, WEAR_MSG_WEARON, WEARING_SOMETHING_ON, ITEM_WEAR_FACE, "face", "face" },
    {  3, 3, 0, 100, WEAR_MSG_WEAROVER, WEARING_SOMETHING_OVER, ITEM_WEAR_EYES, "eyes", "eyes" },
    {  0, 1, 0, 30, WEAR_MSG_PUTON, WEARING_SOMETHING_ON, ITEM_WEAR_EAR, "right ear", "r-ear" },
    {  0, 1, 0, 30, WEAR_MSG_PUTON, WEARING_SOMETHING_ON, ITEM_WEAR_EAR, "left ear", "l-ear" },
    { 11, 1, 0, 40, WEAR_MSG_WEARON, WEARING_SOMETHING_ON, ITEM_WEAR_LEGS, "legs", "legs" },
    { 12, 1, 0, 40, WEAR_MSG_WEARON, WEARING_SOMETHING_ON, ITEM_WEAR_FEET, "feet", "feet" },
    {  5, 1, 0, 30, WEAR_MSG_WEARON, WEARING_SOMETHING_ON, ITEM_WEAR_HANDS, "hands", "hands" },
    {  0, 3, 0, 100, WEAR_MSG_SLUNGOVER, WEARING_SOMETHING_OVER, ITEM_WEAR_SHOULDER, "shoulder", "shoulder" },
    {  4, 1, 0, 40, WEAR_MSG_WEARON, WEARING_SOMETHING_ON, ITEM_WEAR_ARMS, "arms", "arms" },
    { 10, 2, 0, 100, WEAR_MSG_WEARABOUT, WEARING_SOMETHING_AROUND, ITEM_WEAR_WAIST, "waist", "waist" },
    {  7, 2, 0, 100, WEAR_MSG_WEARABOUT, WEARING_SOMETHING_ABOUT, ITEM_WEAR_ABOUT, "body", "about" },
    {  0, 2, 0, 100, WEAR_MSG_STRAPTO, WEARING_SOMETHING_ON, ITEM_WEAR_BACK, "back", "back" },
    {  0, 2, 0, 100, WEAR_MSG_WEARFROM, WEARING_SOMETHING_ON, ITEM_WEAR_ONBELT, "waist", "fromwaist" },
    {  0, 2, 0, 100, WEAR_MSG_WEARFROM, WEARING_SOMETHING_ON, ITEM_WEAR_ONBELT, "waist", "fromwaist" },
    {  6, 1, 0, 30, WEAR_MSG_WEARAROUND, WEARING_SOMETHING_AROUND, ITEM_WEAR_WRIST, "right wrist", "r-wrist" },
    {  9, 1, 0, 30, WEAR_MSG_WEARAROUND, WEARING_SOMETHING_AROUND, ITEM_WEAR_WRIST, "left wrist", "l-wrist" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    {  0, 1, 0, 100, WEAR_MSG_HELDIN, HOLDING_SOMETHING_IN, (ITEM_WEAR_TAKE | ITEM_WEAR_SHIELD | ITEM_WEAR_WIELD), "hand", "primary" },
    {  0, 1, 0, 100, WEAR_MSG_HELDIN, HOLDING_SOMETHING_IN, (ITEM_WEAR_TAKE | ITEM_WEAR_SHIELD | ITEM_WEAR_WIELD), "hand", "off" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    {  4, 1, 0, 100, WEAR_MSG_HELDIN, HOLDING_SOMETHING_IN, (ITEM_WEAR_TAKE | ITEM_WEAR_WIELD), "tail", "prehensile" }
  },
  { // Draconian Body
    {  0, 1, 0, 100, WEAR_MSG_SLIDEONTO, WEARING_SOMETHING_AROUND, ITEM_WEAR_WRIST, "right ring finger", "r-finger" },
    {  0, 1, 0, 100, WEAR_MSG_SLIDEONTO, WEARING_SOMETHING_AROUND, ITEM_WEAR_WRIST, "left ring finger", "l-finger" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    {  2, 1, 0, 100, 0, 0, 0, "neck", "neck" },
    {  2, 1, 0, 100, 0, 0, 0, "neck", "neck" },
    {  9, 1, 0, 100, WEAR_MSG_WEARON, WEARING_SOMETHING_ON, ITEM_WEAR_BODY, "body", "body" },
    {  3, 1, 0, 100, 0, 0, 0, "head", "head" },
    {  3, 1, 0, 100, WEAR_MSG_WEARON, WEARING_SOMETHING_ON, ITEM_WEAR_FACE, "face", "face" },
    {  3, 1, 0, 100, WEAR_MSG_WEAROVER, WEARING_SOMETHING_OVER, ITEM_WEAR_EYES, "eyes", "eyes" },
    {  0, 1, 0, 100, WEAR_MSG_PUTON, WEARING_SOMETHING_ON, ITEM_WEAR_EAR, "right ear", "r-ear" },
    {  0, 1, 0, 100, WEAR_MSG_PUTON, WEARING_SOMETHING_ON, ITEM_WEAR_EAR, "left ear", "l-ear" },
    { 11, 1, 0, 100, 0, 0, 0, "rear legs", "r-legs" },
    { 12, 1, 0, 100, 0, 0, 0, "rear claws", "r-claws" },
    {  5, 1, 0, 100, 0, 0, 0, "fore claws", "f-claws" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    {  4, 1, 0, 100, 0, 0, 0, "fore legs", "forelegs" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    {  7, 1, 0, 100, 0, 0, 0, "body", "about" },
    {  0, 1, 0, 100, WEAR_MSG_STRAPTO, WEARING_SOMETHING_ON, ITEM_WEAR_BACK, "back", "back" },
    { 10, 1, 0, 100, 0, 0, 0, "waist", "waist" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    {  6, 1, 0, 100, WEAR_MSG_WEARAROUND, WEARING_SOMETHING_AROUND, ITEM_WEAR_WAIST, "right wrist", "r-wrist" },
    {  8, 1, 0, 100, WEAR_MSG_WEARAROUND, WEARING_SOMETHING_AROUND, ITEM_WEAR_WAIST, "left wrist", "l-wrist" },
    { 10, 2, 0,   0, 0, 0, 0, "wings", "wings" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    {  0, 1, 0, 100, WEAR_MSG_HELDIN, HOLDING_SOMETHING_IN, ITEM_WEAR_TAKE, "foreclaw", "right" },
    {  0, 1, 0, 100, WEAR_MSG_HELDIN, HOLDING_SOMETHING_IN, ITEM_WEAR_TAKE, "foreclaw", "left" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" }
  },
  { // Avian Body
    {  0, 1, 0,  20, WEAR_MSG_SLIDEONTO, WEARING_SOMETHING_AROUND, ITEM_WEAR_FINGER, "right ring finger", "r-finger" },
    {  0, 1, 0,  20, WEAR_MSG_SLIDEONTO, WEARING_SOMETHING_AROUND, ITEM_WEAR_FINGER, "left ring finger", "l-finger" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    {  2, 3, 0, 100, WEAR_MSG_WEARAROUND, WEAR_ANYTHING_ELSE, ITEM_WEAR_NECK, "neck", "neck" },
    {  2, 3, 0, 100, WEAR_MSG_WEARAROUND, WEAR_ANYTHING_ELSE, ITEM_WEAR_NECK, "neck", "neck" },
    {  8, 2, 0, 100, WEAR_MSG_WEARON, WEARING_SOMETHING_ON, ITEM_WEAR_BODY, "body", "body" },
    {  3, 2, 2, 100, WEAR_MSG_WEARON, WEARING_SOMETHING_ON, ITEM_WEAR_HEAD, "head", "head" },
    {  3, 2, 0, 100, WEAR_MSG_WEARON, WEARING_SOMETHING_ON, ITEM_WEAR_FACE, "face", "face" },
    {  3, 3, 0, 100, WEAR_MSG_WEAROVER, WEARING_SOMETHING_OVER, ITEM_WEAR_EYES, "eyes", "eyes" },
    {  0, 1, 0,  30, WEAR_MSG_PUTON, WEARING_SOMETHING_ON, ITEM_WEAR_EAR, "right ear", "r-ear" },
    {  0, 1, 0,  30, WEAR_MSG_PUTON, WEARING_SOMETHING_ON, ITEM_WEAR_EAR, "left ear", "l-ear" },
    { 11, 1, 0,  40, WEAR_MSG_WEARON, WEARING_SOMETHING_ON, ITEM_WEAR_LEGS, "legs", "legs" },
    { 12, 1, 0,  40, WEAR_MSG_WEARON, WEARING_SOMETHING_ON, ITEM_WEAR_FEET, "feet", "feet" },
    {  5, 1, 0,  30, WEAR_MSG_WEARON, WEARING_SOMETHING_ON, ITEM_WEAR_HANDS, "hands", "hands" },
    {  0, 3, 0, 100, WEAR_MSG_SLUNGOVER, WEARING_SOMETHING_OVER, ITEM_WEAR_SHOULDER, "shoulder", "shoulder" },
    {  4, 1, 0,  40, WEAR_MSG_WEARON, WEARING_SOMETHING_ON, ITEM_WEAR_ARMS, "arms", "arms" },
    {  8, 2, 0,  60, 0, 0, 0, "wings", "wings" },
    {  7, 2, 0, 100, WEAR_MSG_WEARABOUT, WEARING_SOMETHING_ABOUT, ITEM_WEAR_ABOUT, "body", "about" },
    {  0, 2, 0, 100, WEAR_MSG_STRAPTO, WEARING_SOMETHING_ON, ITEM_WEAR_BACK, "back", "back" },
    { 10, 2, 0, 100, WEAR_MSG_WEARABOUT, WEARING_SOMETHING_AROUND, ITEM_WEAR_WAIST, "waist", "waist" },
    {  0, 2, 0, 100, WEAR_MSG_WEARFROM, WEARING_SOMETHING_ON, ITEM_WEAR_ONBELT, "waist", "fromwaist" },
    {  0, 2, 0, 100, WEAR_MSG_WEARFROM, WEARING_SOMETHING_ON, ITEM_WEAR_ONBELT, "waist", "fromwaist" },
    {  6, 1, 0,  30, WEAR_MSG_WEARAROUND, WEARING_SOMETHING_AROUND, ITEM_WEAR_WRIST, "right wrist", "r-wrist" },
    {  9, 1, 0,  30, WEAR_MSG_WEARAROUND, WEARING_SOMETHING_AROUND, ITEM_WEAR_WRIST, "left wrist", "l-wrist" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    {  0, 1, 0,  30, WEAR_MSG_HELDIN, HOLDING_SOMETHING_IN, (ITEM_WEAR_TAKE | ITEM_WEAR_SHIELD | ITEM_WEAR_WIELD), "hand", "primary" },
    {  0, 1, 0,  30, WEAR_MSG_HELDIN, HOLDING_SOMETHING_IN, (ITEM_WEAR_TAKE | ITEM_WEAR_SHIELD | ITEM_WEAR_WIELD), "hand", "off" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" }
  },
  { // Amorphous Body
    {  0, 1, 0, 100, WEAR_MSG_SLIDEONTO, WEARING_SOMETHING_AROUND, ITEM_WEAR_FINGER, "right ring finger", "r-finger" },
    {  0, 1, 0, 100, WEAR_MSG_SLIDEONTO, WEARING_SOMETHING_AROUND, ITEM_WEAR_FINGER, "left ring finger", "l-finger" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    {  2, 2, 0, 100, WEAR_MSG_WEARAROUND, WEAR_ANYTHING_ELSE, ITEM_WEAR_NECK, "neck", "neck" },
    {  2, 2, 0, 100, WEAR_MSG_WEARAROUND, WEAR_ANYTHING_ELSE, ITEM_WEAR_NECK, "neck", "neck" },
    {  9, 1, 0, 100, WEAR_MSG_WEARON, WEARING_SOMETHING_ON, ITEM_WEAR_BODY, "body", "body" },
    {  3, 1, 0, 100, WEAR_MSG_WEARON, WEARING_SOMETHING_ON, ITEM_WEAR_HEAD, "head", "head" },
    {  3, 1, 0, 100, WEAR_MSG_WEARON, WEARING_SOMETHING_ON, ITEM_WEAR_FACE, "face", "face" },
    {  3, 2, 0, 100, WEAR_MSG_WEAROVER, WEARING_SOMETHING_OVER, ITEM_WEAR_EYES, "eyes", "eyes" },
    {  0, 1, 0, 100, WEAR_MSG_PUTON, WEARING_SOMETHING_ON, ITEM_WEAR_EAR, "right ear", "r-ear" },
    {  0, 1, 0, 100, WEAR_MSG_PUTON, WEARING_SOMETHING_ON, ITEM_WEAR_EAR, "left ear", "l-ear" },
    { 11, 1, 0, 100, WEAR_MSG_WEARON, WEARING_SOMETHING_ON, ITEM_WEAR_LEGS, "legs", "legs" },
    { 12, 1, 0, 100, WEAR_MSG_WEARON, WEARING_SOMETHING_ON, ITEM_WEAR_FEET, "feet", "feet" },
    {  5, 1, 0, 100, WEAR_MSG_WEARON, WEARING_SOMETHING_ON, ITEM_WEAR_HANDS, "hands", "hands" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    {  4, 1, 0, 100, WEAR_MSG_WEARON, WEARING_SOMETHING_ON, ITEM_WEAR_ARMS, "arms", "arms" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    {  7, 1, 0, 100, WEAR_MSG_WEARABOUT, WEARING_SOMETHING_ABOUT, ITEM_WEAR_ABOUT, "body", "about" },
    {  0, 1, 0, 100, WEAR_MSG_STRAPTO, WEARING_SOMETHING_ON, ITEM_WEAR_BACK, "back", "back" },
    { 10, 1, 0, 100, WEAR_MSG_WEARABOUT, WEARING_SOMETHING_AROUND, ITEM_WEAR_WAIST, "waist", "waist" },
    {  0, 1, 0, 100, WEAR_MSG_WEARFROM, WEARING_SOMETHING_ON, ITEM_WEAR_ONBELT, "waist", "fromwaist" },
    {  6, 1, 0, 100, WEAR_MSG_WEARAROUND, WEARING_SOMETHING_AROUND, ITEM_WEAR_WRIST, "right wrist", "r-wrist" },
    {  8, 1, 0, 100, WEAR_MSG_WEARAROUND, WEARING_SOMETHING_AROUND, ITEM_WEAR_WRIST, "left wrist", "l-wrist" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    {  0, 1, 0, 100, WEAR_MSG_HELDIN, HOLDING_SOMETHING_IN, (ITEM_WEAR_TAKE | ITEM_WEAR_SHIELD | ITEM_WEAR_WIELD), "primary hand", "hand" },
    {  0, 1, 0, 100, WEAR_MSG_HELDIN, HOLDING_SOMETHING_IN, (ITEM_WEAR_TAKE | ITEM_WEAR_SHIELD | ITEM_WEAR_WIELD), "off hand", "offhand" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" }
  },
  { // Hooved Quadruped Body
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    {  2, 1, 0, 100, WEAR_MSG_WEARAROUND, WEAR_ANYTHING_ELSE, ITEM_WEAR_NECK, "neck", "neck" },
    {  2, 1, 0, 100, WEAR_MSG_WEARAROUND, WEAR_ANYTHING_ELSE, ITEM_WEAR_NECK, "neck", "neck" },
    {  9, 1, 0, 100, WEAR_MSG_WEARON, WEARING_SOMETHING_ON, ITEM_WEAR_BODY, "body", "body" },
    {  3, 1, 0, 100, WEAR_MSG_WEARON, WEARING_SOMETHING_ON, ITEM_WEAR_HEAD, "head", "head" },
    {  3, 1, 0, 100, WEAR_MSG_WEARON, WEARING_SOMETHING_ON, 0, "face", "face" },
    {  3, 1, 0, 100, WEAR_MSG_WEAROVER, WEARING_SOMETHING_OVER, ITEM_WEAR_EYES, "eyes", "eyes" },
    {  0, 1, 0, 100, WEAR_MSG_PUTON, WEARING_SOMETHING_ON, ITEM_WEAR_EAR, "right ear", "r-ear" },
    {  0, 1, 0, 100, WEAR_MSG_PUTON, WEARING_SOMETHING_ON, ITEM_WEAR_EAR, "left ear", "l-ear" },
    { 11, 1, 0, 100, WEAR_MSG_WEARON, WEARING_SOMETHING_ON, ITEM_WEAR_LEGS, "rear legs", "back legs" },
    { 12, 1, 0, 100, WEAR_MSG_WEARON, WEARING_SOMETHING_ON, ITEM_WEAR_FEET, "rear hooves", "rear hooves" },
    {  5, 1, 0, 100, WEAR_MSG_WEARON, WEARING_SOMETHING_ON, ITEM_WEAR_FEET, "front hooves", "front hooves" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    {  4, 1, 0, 100, WEAR_MSG_WEARON, WEARING_SOMETHING_ON, ITEM_WEAR_LEGS, "forelegs", "forelegs" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    {  7, 1, 0, 100, WEAR_MSG_WEARABOUT, WEARING_SOMETHING_ABOUT, 0, "body", "about" },
    {  0, 1, 0, 100, WEAR_MSG_STRAPTO, WEARING_SOMETHING_ON, ITEM_WEAR_BACK, "back", "back" },
    { 10, 1, 0, 100, WEAR_MSG_WEARABOUT, WEARING_SOMETHING_AROUND, 0, "waist", "waist" },
    {  0, 1, 0, 100, WEAR_MSG_WEARFROM, WEARING_SOMETHING_ON, 0, "girth", "fromgirth" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    { -1, 1, 0, 100, WEAR_MSG_HELDIN, HOLDING_SOMETHING_IN, (ITEM_WEAR_TAKE | ITEM_WEAR_SHIELD | ITEM_WEAR_WIELD), "mouth", "mouth" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" }
  },
  { // Pawed Quadruped Body
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    {  2, 1, 0, 100, WEAR_MSG_WEARAROUND, WEAR_ANYTHING_ELSE, ITEM_WEAR_NECK, "neck", "neck" },
    {  2, 1, 0, 100, WEAR_MSG_WEARAROUND, WEAR_ANYTHING_ELSE, ITEM_WEAR_NECK, "neck", "neck" },
    {  9, 1, 0, 100, WEAR_MSG_WEARON, WEARING_SOMETHING_ON, ITEM_WEAR_BODY, "body", "body" },
    {  3, 1, 0, 100, WEAR_MSG_WEARON, WEARING_SOMETHING_ON, ITEM_WEAR_HEAD, "head", "head" },
    {  3, 1, 0, 100, WEAR_MSG_WEARON, WEARING_SOMETHING_ON, 0, "face", "face" },
    {  3, 1, 0, 100, WEAR_MSG_WEAROVER, WEARING_SOMETHING_OVER, ITEM_WEAR_EYES, "eyes", "eyes" },
    {  0, 1, 0, 100, WEAR_MSG_PUTON, WEARING_SOMETHING_ON, ITEM_WEAR_EAR, "right ear", "r-ear" },
    {  0, 1, 0, 100, WEAR_MSG_PUTON, WEARING_SOMETHING_ON, ITEM_WEAR_EAR, "left ear", "l-ear" },
    { 11, 1, 0, 100, WEAR_MSG_WEARON, WEARING_SOMETHING_ON, ITEM_WEAR_LEGS, "rear legs", "back legs" },
    { 12, 1, 0, 100, WEAR_MSG_WEARON, WEARING_SOMETHING_ON, ITEM_WEAR_FEET, "rear paws", "rear paws" },
    {  5, 1, 0, 100, WEAR_MSG_WEARON, WEARING_SOMETHING_ON, ITEM_WEAR_FEET, "front paws", "front paws" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    {  4, 1, 0, 100, WEAR_MSG_WEARON, WEARING_SOMETHING_ON, ITEM_WEAR_LEGS, "forelegs", "forelegs" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    {  7, 1, 0, 100, WEAR_MSG_WEARABOUT, WEARING_SOMETHING_ABOUT, 0, "body", "about" },
    {  0, 1, 0, 100, WEAR_MSG_STRAPTO, WEARING_SOMETHING_ON, ITEM_WEAR_BACK, "back", "back" },
    { 10, 1, 0, 100, WEAR_MSG_WEARABOUT, WEARING_SOMETHING_AROUND, 0, "waist", "waist" },
    {  0, 1, 0, 100, WEAR_MSG_WEARFROM, WEARING_SOMETHING_ON, 0, "girth", "fromgirth" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    { -1, 1, 0, 100, WEAR_MSG_HELDIN, HOLDING_SOMETHING_IN, (ITEM_WEAR_TAKE | ITEM_WEAR_SHIELD | ITEM_WEAR_WIELD), "mouth", "mouth" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" }
  },
  { // Shastick Body
    {  0, 1, 0, 20, WEAR_MSG_SLIDEONTO, WEARING_SOMETHING_AROUND, ITEM_WEAR_FINGER, "right major digit", "r-digit" },
    {  0, 1, 0, 20, WEAR_MSG_SLIDEONTO, WEARING_SOMETHING_AROUND, ITEM_WEAR_FINGER, "left major digit", "l-digit" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    {  2, 4, 0, 100, WEAR_MSG_WEARAROUND, WEAR_ANYTHING_ELSE, ITEM_WEAR_NECK, "neck", "neck" },
    {  2, 4, 0, 100, WEAR_MSG_ATTACH, HAVE_SOMETHING_ATTACHED, (ITEM_WEAR_NECK | ITEM_WEAR_SYMBIOT), "neck", "neck" },
    {  8, 3, 0, 100, WEAR_MSG_WEARON, WEARING_SOMETHING_ON, ITEM_WEAR_BODY, "body", "body" },
    {  3, 2, 2, 100, WEAR_MSG_WEARON, WEARING_SOMETHING_ON, ITEM_WEAR_HEAD, "head", "head" },
    {  3, 2, 0, 100, WEAR_MSG_WEARON, WEARING_SOMETHING_ON, ITEM_WEAR_FACE, "face", "face" },
    {  3, 4, 0, 100, WEAR_MSG_WEAROVER, WEARING_SOMETHING_OVER, ITEM_WEAR_EYES, "eyes", "eyes" },
    {  3, 2, 2, 100, WEAR_MSG_ATTACH, HAVE_SOMETHING_ATTACHED, ITEM_WEAR_SYMBIOT, "head", "head" },
    {  3, 2, 0, 100, WEAR_MSG_ATTACH, HAVE_SOMETHING_ATTACHED, ITEM_WEAR_SYMBIOT, "face", "face" },
    { 11, 1, 0, 60, WEAR_MSG_WEARON, WEARING_SOMETHING_ON, ITEM_WEAR_LEGS, "legs", "legs" },
    { 12, 1, 0, 40, WEAR_MSG_WEARON, WEARING_SOMETHING_ON, ITEM_WEAR_FEET, "feet", "feet" },
    {  5, 1, 0, 30, WEAR_MSG_WEARON, WEARING_SOMETHING_ON, ITEM_WEAR_HANDS, "hands", "hands" },
    {  0, 3, 0, 100, WEAR_MSG_ATTACH, HAVE_SOMETHING_ATTACHED, (ITEM_WEAR_SHOULDER | ITEM_WEAR_SYMBIOT), "shoulder", "shoulder" },
    {  4, 1, 0, 40, WEAR_MSG_WEARON, WEARING_SOMETHING_ON, ITEM_WEAR_ARMS, "arms", "arms" },
    { 10, 3, 0, 100, WEAR_MSG_WEARABOUT, WEARING_SOMETHING_AROUND, ITEM_WEAR_WAIST, "waist", "waist" },
    {  7, 3, 0, 100, WEAR_MSG_WEARABOUT, WEARING_SOMETHING_ABOUT, ITEM_WEAR_ABOUT, "body", "about" },
    {  0, 3, 0, 100, WEAR_MSG_STRAPTO, WEARING_SOMETHING_ON, ITEM_WEAR_BACK, "back", "back" },
    {  0, 2, 0, 100, WEAR_MSG_WEARFROM, WEARING_SOMETHING_ON, ITEM_WEAR_ONBELT, "waist", "fromwaist" },
    {  0, 2, 0, 100, WEAR_MSG_WEARFROM, WEARING_SOMETHING_ON, ITEM_WEAR_ONBELT, "waist", "fromwaist" },
    {  6, 1, 0, 50, WEAR_MSG_ATTACH, HAVE_SOMETHING_ATTACHED, (ITEM_WEAR_SYMBIOT | ITEM_WEAR_SYMBIOT), "right wrist", "r-wrist" },
    {  9, 1, 0, 50, WEAR_MSG_ATTACH, HAVE_SOMETHING_ATTACHED, (ITEM_WEAR_SYMBIOT | ITEM_WEAR_SYMBIOT), "left wrist", "l-wrist" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    {  0, 1, 0, 60, WEAR_MSG_HELDIN, HOLDING_SOMETHING_IN, (ITEM_WEAR_TAKE | ITEM_WEAR_SHIELD | ITEM_WEAR_WIELD), "hand", "primary" },
    {  0, 1, 0, 60, WEAR_MSG_HELDIN, HOLDING_SOMETHING_IN, (ITEM_WEAR_TAKE | ITEM_WEAR_SHIELD | ITEM_WEAR_WIELD), "hand", "off" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" }
  },
  { // Terran Armor
    {  0, 1, 0, 100, WEAR_MSG_SLIDEONTO, WEARING_SOMETHING_AROUND, ITEM_WEAR_FINGER, "right ring finger", "r-finger" },
    {  0, 1, 0, 100, WEAR_MSG_SLIDEONTO, WEARING_SOMETHING_AROUND, ITEM_WEAR_FINGER, "left ring finger", "l-finger" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    {  2, 1, 0, 100, WEAR_MSG_WEARAROUND, WEAR_ANYTHING_ELSE, ITEM_WEAR_NECK, "neck", "neck" },
    {  2, 1, 0, 100, WEAR_MSG_WEARAROUND, WEAR_ANYTHING_ELSE, ITEM_WEAR_NECK, "neck", "neck" },
    {  9, 1, 0, 100, WEAR_MSG_WEARON, WEARING_SOMETHING_ON, ITEM_WEAR_BODY, "body", "body" },
    {  3, 1, 0, 100, WEAR_MSG_WEARON, WEARING_SOMETHING_ON, ITEM_WEAR_HEAD, "head", "head" },
    {  3, 1, 0, 100, WEAR_MSG_WEARON, WEARING_SOMETHING_ON, ITEM_WEAR_FACE, "face", "face" },
    {  3, 1, 0, 100, WEAR_MSG_WEAROVER, WEARING_SOMETHING_OVER, ITEM_WEAR_EYES, "eyes", "eyes" },
    {  0, 1, 0, 100, WEAR_MSG_PUTON, WEARING_SOMETHING_ON, ITEM_WEAR_EAR, "right ear", "r-ear" },
    {  0, 1, 0, 100, WEAR_MSG_PUTON, WEARING_SOMETHING_ON, ITEM_WEAR_EAR, "left ear", "l-ear" },
    { 11, 1, 0, 100, WEAR_MSG_WEARON, WEARING_SOMETHING_ON, ITEM_WEAR_LEGS, "legs", "legs" },
    { 12, 1, 0, 100, WEAR_MSG_WEARON, WEARING_SOMETHING_ON, ITEM_WEAR_FEET, "feet", "feet" },
    {  5, 1, 0, 100, WEAR_MSG_WEARON, WEARING_SOMETHING_ON, ITEM_WEAR_HANDS, "hands", "hands" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    {  4, 1, 0, 100, WEAR_MSG_WEARON, WEARING_SOMETHING_ON, ITEM_WEAR_ARMS, "arms", "arms" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    {  7, 1, 0, 100, WEAR_MSG_WEARABOUT, WEARING_SOMETHING_ABOUT, ITEM_WEAR_ABOUT, "body", "about" },
    {  0, 1, 0, 100, WEAR_MSG_STRAPTO, WEARING_SOMETHING_ON, ITEM_WEAR_BACK, "back", "back" },
    { 10, 1, 0, 100, WEAR_MSG_WEARABOUT, WEARING_SOMETHING_AROUND, ITEM_WEAR_WAIST, "waist", "waist" },
    {  0, 1, 0, 100, WEAR_MSG_WEARFROM, WEARING_SOMETHING_ON, ITEM_WEAR_ONBELT, "waist", "fromwaist" },
    {  6, 1, 0, 100, WEAR_MSG_WEARAROUND, WEARING_SOMETHING_AROUND, ITEM_WEAR_WRIST, "right wrist", "r-wrist" },
    {  8, 1, 0, 100, WEAR_MSG_WEARAROUND, WEARING_SOMETHING_AROUND, ITEM_WEAR_WRIST, "left wrist", "l-wrist" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    {  0, 1, 0, 100, WEAR_MSG_HELDIN, HOLDING_SOMETHING_IN, (ITEM_WEAR_TAKE | ITEM_WEAR_SHIELD | ITEM_WEAR_WIELD), "primary hand", "hand" },
    {  0, 1, 0, 100, WEAR_MSG_HELDIN, HOLDING_SOMETHING_IN, (ITEM_WEAR_TAKE | ITEM_WEAR_SHIELD | ITEM_WEAR_WIELD), "off hand", "offhand" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" }
  },
  { // Trulor Armor
    {  0, 1, 0, 100, WEAR_MSG_SLIDEONTO, WEARING_SOMETHING_AROUND, ITEM_WEAR_WRIST, "right ring finger", "r-finger" },
    {  0, 1, 0, 100, WEAR_MSG_SLIDEONTO, WEARING_SOMETHING_AROUND, ITEM_WEAR_WRIST, "left ring finger", "l-finger" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    {  2, 1, 0, 100, 0, 0, 0, "neck", "neck" },
    {  2, 1, 0, 100, 0, 0, 0, "neck", "neck" },
    {  9, 1, 0, 100, WEAR_MSG_WEARON, WEARING_SOMETHING_ON, ITEM_WEAR_BODY, "body", "body" },
    {  3, 1, 0, 100, 0, 0, 0, "head", "head" },
    {  3, 1, 0, 100, WEAR_MSG_WEARON, WEARING_SOMETHING_ON, ITEM_WEAR_FACE, "face", "face" },
    {  3, 1, 0, 100, WEAR_MSG_WEAROVER, WEARING_SOMETHING_OVER, ITEM_WEAR_EYES, "eyes", "eyes" },
    {  0, 1, 0, 100, WEAR_MSG_PUTON, WEARING_SOMETHING_ON, ITEM_WEAR_EAR, "right ear", "r-ear" },
    {  0, 1, 0, 100, WEAR_MSG_PUTON, WEARING_SOMETHING_ON, ITEM_WEAR_EAR, "left ear", "l-ear" },
    { 11, 1, 0, 100, 0, 0, 0, "rear legs", "r-legs" },
    { 12, 1, 0, 100, 0, 0, 0, "rear claws", "r-claws" },
    {  5, 1, 0, 100, 0, 0, 0, "fore claws", "f-claws" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    {  4, 1, 0, 100, 0, 0, 0, "fore legs", "forelegs" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    {  7, 1, 0, 100, 0, 0, 0, "body", "about" },
    {  0, 1, 0, 100, WEAR_MSG_STRAPTO, WEARING_SOMETHING_ON, ITEM_WEAR_BACK, "back", "back" },
    { 10, 1, 0, 100, 0, 0, 0, "waist", "waist" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    {  6, 1, 0, 100, WEAR_MSG_WEARAROUND, WEARING_SOMETHING_AROUND, ITEM_WEAR_WAIST, "right wrist", "r-wrist" },
    {  8, 1, 0, 100, WEAR_MSG_WEARAROUND, WEARING_SOMETHING_AROUND, ITEM_WEAR_WAIST, "left wrist", "l-wrist" },
    {  10,3,  0,0, 100,  0, 0, "wings", "wings" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    {  0, 1, 0, 100, WEAR_MSG_HELDIN, HOLDING_SOMETHING_IN, (ITEM_WEAR_TAKE | ITEM_WEAR_SHIELD | ITEM_WEAR_WIELD), "hand", "primary" },
    {  0, 1, 0, 100, WEAR_MSG_HELDIN, HOLDING_SOMETHING_IN, (ITEM_WEAR_TAKE | ITEM_WEAR_SHIELD | ITEM_WEAR_WIELD), "hand", "off" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" },
    { -1, 1, 0, 100, 0, 0, 0, "", "" }
  }
};


const char *skillbases[] = {
  "STR",
  "INT",
  "WIS",
  "DEX",
  "CON",
  "CHA",
  "VISION",
  "HEARING",
  "MOVE",
  "\n"
};


const char *trig_attach_types[] = {
  "Mobiles",
  "Objects",
  "Rooms",
  "\n"
};


const char *ocmd_place_vector[] = {
  "Equipped",
  "Inventory",
  "Room",
  "\n"
};


const char *encumbrance_levels[] = {
  "You are unencumbered.",
  "Your load is light.",
  "You are moderately encumbered.",
  "Your load feels heavy!",
  "Your load feels REALLY HEAVY!",
  "You can barely manage to carry all this stuff!",
  "You are too weighed down to move!",
  "\n"
};


const char *encumbrance_shift[][2] = {
  {
    "BUG: UNDERWEIGHTED ENCUMBRANCE.  PLEASE REPORT.\r\n",
    "You are now unencumbered.\r\n",
  },
  {
    "You feel a little weighed down.\r\n",
    "Your load feels light now.\r\n",
  },
  {
    "You feel moderately weighed down.\r\n",
    "Whew!  You feel only moderately burdened now.\r\n",
  },
  {
    "You shift your weight, struggling with your burden.\r\n",
    "You shift your balance as you shed some of your burden.\r\n",
  },
  {
    "You are really taxed carrying this stuff!\r\n",
    "You still struggle beneath your heavy load.\r\n",
  },
  {
    "You are barely able to move with this load!\r\n",
    "You are able to move now, barely...\r\n",
  },
  {
    "Your load is IMPOSSIBLY heavy!  You can't move!\r\n",
    "BUG: OVERWEIGHTED ENCUMBRANCE.  PLEASE REPORT.\r\n",
  }
};


// ASCII load of skill parameters by Daniel Rollings AKA Daniel Houghton

// A quick lookup table for rolling against stats.
const char *named_stats[] = {
  "strength",
  "intelligence",
  "wisdom",
  "dexterity",
  "constitution",
  "charisma",
  "weapon",
  "vision",
  "hearing",
  "movement",
  "right weapon",
  "left weapon",
  "third weapon",
  "fourth weapon",
  "\n"
};


// A table for roll bases.
const char *named_bases[] = {
	"STR",
	"INT",
	"WIS",
	"DEX",
	"CON",
	"CHA",
	"WEAPON",
	"SIGHT",
	"HEAR",
	"MOVE",
	"CLERIC",
	"MAGERY",
	"\n"
};


const char *colornames[] =
{
	"off",
	"UNUSED",
	"black",
	"red",
	"green",
	"yellow",
	"blue",
	"magenta",
	"cyan",
	"white",
	"UNUSED",
	"grey",
	"bred",
	"bgreen",
	"byellow",
	"bblue",
	"bmagenta",
	"bcyan",
	"bwhite",
	"\n"
};



const char *colorcodes[] =
{
	"",
	"",
	"&cl",
	"&cr",
	"&cg",
	"&cy",
	"&cb",
	"&cm",
	"&cc",
	"&cw",
	"",
	"&cL",
	"&cR",
	"&cG",
	"&cY",
	"&cB",
	"&cM",
	"&cC",
	"&cW",
	"\n"
};


const char *gods[] =
{
	"None",
	"Pelendra",
	"Tal-Vindas",
	"Dymordian",
	"Kharsus",
	"Nymris",
	"Vaadh",
	"Shestu",
	"\n"
};


const char *class_abbrevs[] = {
	"Ma",
	"Cl",
	"Th",
	"Wa",
	"Ra",
	"As",
	"Al",
	"Va",
	"Ps",
	"Mb",
	"Un",
	"\n"
};

const char * mortality_types[NUM_MORTALITIES + 1] = {
	"Mortal",
	"Immortal",
	"Deity",
	"Staff",
	"\n"
};

const char *spell_routines[] = {
  "DAMAGE",
  "AFFECTS",
  "UNAFFECTS",
  "POINTS",
  "ALTER_OBJS",
  "GROUPS",
  "MASSES",
  "AREAS",
  "SUMMONS",
  "CREATIONS",
  "MANUAL",
  "MATERIALS",
  "ROOM",
  "HITGROUPS",
  "SCRIPT",
  "\n"
};


const char *damage_types[] = {
	"undefined",
	"basic",
	"slashing",
	"piercing",
	"fire",
	"electricity",
	"acid",
	"freezing",
	"poison",
	"psionic",
	"vim",
	"explosion",
	"fall",
	"misc",
	"suffering",
	"\n"
};


// Character strings to identify body types
const char *bodytypes[] =
{
	"humanoid",
	"insectoid",
	"lizardman",
	"dragon",
	"avian",
	"amorphous",
	"hooved quad",
	"pawed quad",
	"shastick",
	"\n"
};


const char *vehicle_bits[] = {
	"ground",
	"air",
	"space",
	"deepspace",
	"water",
	"underwater",
	"\n"
};


const struct oedit_objval_tab obj_values_tab[NUM_ITEM_TYPES + 1][NUM_OBJ_VALUES + 1] = {
  { // NULL
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
  },
  { // Light
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "Number of hours (0 = burnt, -1 is infinite)", -1, 32000, NULL, Datatypes::Integer },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
  },
  { // Scroll
    { "Spell level", 1, 1000, NULL, Datatypes::Integer },
    { "Spell", FIRST_SPELL, FIRST_STAT - 1, (const char **)spells, Datatypes::SpellValue },
    { "Spell", FIRST_SPELL, FIRST_STAT - 1, (const char **)spells, Datatypes::SpellValue },
    { "Spell", FIRST_SPELL, FIRST_STAT - 1, (const char **)spells, Datatypes::SpellValue },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
  },
  { // Wand
    { "Spell level", 1, 1000, NULL, Datatypes::Integer },
    { "Max number of charges", -1, 1000, NULL, Datatypes::Integer },
    { "Number of charges remaining", -1, 1000, NULL, Datatypes::Integer },
    { "Spell", FIRST_SPELL, FIRST_STAT - 1, (const char **)spells, Datatypes::SpellValue },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
  },
  { // Staff
    { "Spell level", 1, 1000, NULL, Datatypes::Integer },
    { "Max number of charges", 0, 0, NULL, Datatypes::Integer },
    { "Number of charges remaining", 0, 0, NULL, Datatypes::Integer },
    { "Spell", FIRST_SPELL, FIRST_STAT - 1, (const char **)spells, Datatypes::SpellValue },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
  },
  { // Weapon
    { "Proficiency", FIRST_WEAPON_PROFICIENCY, LAST_WEAPON_PROFICIENCY, (const char **) spells, Datatypes::Value },
    { "Swing bonus", -100, 100, NULL, Datatypes::Integer },
    { "Swing Damage Type (0 if cannot be swung)", -1, 0, (const char **) damage_types, Datatypes::Value },
    { "Swing Message", FIRST_COMBAT_MESSAGE, TYPE_GARROTE, (const char **) spells, Datatypes::DiffValue },
    { "Thrust bonus", 0-100, 100, NULL, Datatypes::Integer },
    { "Thrust Damage Type (0 if cannot be thrusted)",  -1, 0, (const char **) damage_types, Datatypes::Value },
    { "Thrust Message", FIRST_COMBAT_MESSAGE, TYPE_GARROTE, (const char **) spells, Datatypes::DiffValue },
    { "Speed", 1, 50, NULL, Datatypes::Integer },
  },
  { // Fireweapon
    { "Proficiency", FIRST_WEAPON_PROFICIENCY, LAST_WEAPON_PROFICIENCY, (const char **) spells, Datatypes::Value },
    { "To-hit modifier", 20, 20, NULL, 0 },
    { "Damage bonus", -20, 100, NULL, 0 },
    { "Damage Type (0 if cannot be thrusted)",  -1, 0, (const char **) damage_types, Datatypes::Value },
    { "Holds ammo (0 if not reloaded)", 0, 100, NULL, 0 },
    { "Contains ammo, (0 if not reloaded)", 0, 100, NULL, 0 },
    { "Range", 0, 3, NULL, Datatypes::Integer },
    { "Speed", 2, 15, NULL, Datatypes::Integer },
  },
  { // Missile
    { "Proficiency", FIRST_WEAPON_PROFICIENCY, LAST_WEAPON_PROFICIENCY, (const char **) spells, Datatypes::Value },
    { "To-hit modifier", 20, 20, NULL, 0 },
    { "Damage bonus", -20, 100, NULL, 0 },
    { "Damage type", 0, 0, (const char **) damage_types, Datatypes::Value },
    { "Ammo capacity (0 if not used with reload)", 0, 100, NULL, 0 },
    { "Ammo contained (0 if not used with reload)", 0, 100, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
  },
  { // Treasure
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
  },
  { // Armor
    { "Damage bonus", -100, 100, NULL, Datatypes::Integer },
    { "Damage type", 0, 0, (const char **) damage_types, Datatypes::Value },
    { "Shielding bonus", -100, 100, NULL, Datatypes::Integer },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
  },
  { // Potion
    { "Spell level", 1, 1000, NULL, Datatypes::Integer },
    { "Spell", FIRST_SPELL, FIRST_STAT - 1, (const char **)spells, Datatypes::SpellValue },
    { "Spell", FIRST_SPELL, FIRST_STAT - 1, (const char **)spells, Datatypes::SpellValue },
    { "Spell", FIRST_SPELL, FIRST_STAT - 1, (const char **)spells, Datatypes::SpellValue },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
  },
  { // Reagent
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
  },
  { // Talisman
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
  },
  { // Spellbook
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
  },
  { // Trap
    { "Spell", FIRST_SPELL, FIRST_STAT - 1, (const char **)spells, Datatypes::SpellValue },
    { "Number of damage dice", 0, 200, NULL, Datatypes::Integer },
    { "Size of damage dice", 0, 200, NULL, Datatypes::Integer },
    { "Max number of charges", -1, 1000, NULL, Datatypes::Integer },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
  },
  { // Container
    { "Capacity", 0, 32000, NULL, Datatypes::Integer },
    { "Flags", 0, 0, (const char **) container_bits, Datatypes::Bitvector },
    { "Vnum of key to open container (-1 for no key)", -1, 32099, NULL, Datatypes::Integer },
    { "Corpse of (enter mob vnum, 0 if not corpse)", -32099, 32099, NULL, Datatypes::Integer },
    { "Pick modifier", -10, 10, NULL, Datatypes::Integer },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
  },
  { // Note
    { "Language", SKILL_ELVEN_LANGUAGE, SKILL_SIGN_LANGUAGE, (const char **)spells, Datatypes::Value },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
  },
  { // Drinkcon
    { "Max drink units", -1, 1000, NULL, Datatypes::Integer },
    { "Initial drink units", -1, 1000, NULL, Datatypes::Integer },
    { "Liquid type", 0, 0, (const char **) drinks, Datatypes::Value },
    { "Poisoned (0 = not poison)", 0, 1, NULL, Datatypes::Integer },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
  },
  { // Key
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
  },
  { // Food
    { "Hours to fill stomach", 0, 24, NULL, Datatypes::Integer },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "Poisoned (0 = not poison)", 0, 1000, NULL, Datatypes::Integer },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
  },
  { // Money
    { "Number of gold coins", 1, 2000000000, NULL, Datatypes::Integer },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
  },
  { // Pen
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
  },
  { // Boat
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
  },
  { // Fountain
    { "Max drink units", -1, 10000, NULL, Datatypes::Integer },
    { "Initial drink units", -1, 10000, NULL, Datatypes::Integer },
    { "Liquid type", 0, 0, (const char **) drinks, Datatypes::Value },
    { "Poisoned (0 = not poison)", 0, 1000, NULL, Datatypes::Integer },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
  },
  { // Grenade
    { "Seconds to countdown to explosion", 0, 2000000000, NULL, Datatypes::Integer },
    { "Number of damage dice", 0, 200, NULL, Datatypes::Integer },
    { "Size of damage dice", 0, 200, NULL, Datatypes::Integer },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
  },
  { // Weaponcon
    { "\n", 0, 0, NULL, 0 },
    { "Contents concealed?  0 = No, 1 = YES", 0, 1, NULL, Datatypes::Integer },
    { "\n", 0, 0, NULL, 0 },
    { "Weapon type stored", FIRST_WEAPON_PROFICIENCY, LAST_WEAPON_PROFICIENCY, (const char **) spells, Datatypes::Value },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
  },
  { // Attachment
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
  },
  { // Board
    { "Read Access", 0, MAX_TRUST, NULL, Datatypes::Integer },
    { "Write Access", 0, MAX_TRUST, NULL, Datatypes::Integer },
    { "Remove Access", 0, MAX_TRUST, NULL, Datatypes::Integer },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
  },
  { // Furniture
    { "Capacity", 0, 32000, NULL, Datatypes::Integer },
    { "Position", POS_RESTING, POS_STANDING, (const char **) positions, Datatypes::Value },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
  },
  { // Vehicle
    { "Entry Room", 0, 32099, NULL, Datatypes::Integer },
    { "Flags", 0, 0, (const char **) vehicle_bits, Datatypes::Bitvector },
    { "Lowest Room", 0, 32099, NULL, Datatypes::Integer },
    { "Highest Room", 0, 32099, NULL, Datatypes::Integer },
    { "Size in cubic meters", 0, 1000, NULL, Datatypes::Integer },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
  },
  { // Vehicle Hatch
    { "Vehicle VNum", 0, 32099, NULL, Datatypes::Integer },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
  },
  { // Vehicle Controls
    { "Vehicle VNum", 0, 32099, NULL, Datatypes::Integer },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
  },
  { // Vehicle Window
    { "Vehicle VNum", 0, 32099, NULL, Datatypes::Integer },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
  },
  { // Vehicle Weapon
    { "Vehicle VNum", 0, 32099, NULL, Datatypes::Integer },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
    { "\n", 0, 0, NULL, 0 },
  }
};


const char *material_types[] = {
	"undefined",
	"liquid",
	"wax",
	"veggy",
	"flesh",
	"paper",
	"cloth",
	"leather",
	"wood",
	"bone",
	"dragon hide",
	"iron",
	"metal",
	"copper",
	"silver",
	"gold",
	"platinum",
	"mithril",
	"titanium",
	"plastic",
	"glass",
	"gemstone",
	"mineral",
	"shadow",
	"ether",
	"steel",
	"adamantite",
	"diamond",
	"porcelain",
	"ceramic",
	"jelly",
	"vapor",
	"dark steel",
	"\n"
};


const char *realms[] = {
	"Nyrilis",
	"Earth",
	"Olodris",
	"Space",
	"Astral Planes",
	"Shadow Planes",
	"Ethereal Planes",
	"\n"
};


const char * distance_upper[MAX_DISTANCE + 1] = {
	"SHOULD NOT SEE!  REPORT IMMEDIATELY!",
//	"Immediately",
	"Nearby",
//	"A little ways away",
	"A ways away",
//	"Off in the distance",
	"Far away"
};


const char * distance_lower[MAX_DISTANCE + 1] = {
	"SHOULD NOT SEE!  REPORT IMMEDIATELY!",
//	"immediately",
	"nearby",
//	"a little ways away",
	"a ways away",
//	"off in the distance",
	"far away"
};

