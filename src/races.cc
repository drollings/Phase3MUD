


#include "structs.h"
#include "characters.h"
#include "utils.h"
#include "skills.h"

#include "constants.h"

/*
 * This file attempts to concentrate most of the code which must be changed
 * in order for new races to be added.  If you're adding a new race,
 * you should go through this entire file from beginning to end and add
 * the appropriate new special cases for your new race.
 */

#define Y   TRUE
#define N   FALSE

/* Names first */

const char *race_abbrevs[] = {
	"Anm",
	"Hyu",
	"Elf",
	"Dwo",
	"Sha",
	"Mur",
	"Lao",
	"Thd",
	"Ter",
	"Tru",
	"Drg",
	"Dev",
	"Sol",
	"Bug",
	"Fsh",
	"Amp",
	"Fel",
	"Can",
	"Equ",
	"Brd",
	"Gob",
	"Orc",
	"Gth",
	"\n"
};


const char *race_types[] = {
	"Animal",
	"HyuMann",
	"Elf",
	"Dwomax",
	"Shastick",
	"Murrash",
	"Laori",
	"Thadyri",
	"Terran",
	"Trulor",
	"Dragon",
	"Deva",
	"Solar",
	"Insect",
	"Fish",
	"Amphibian",
	"Feline",
	"Canine",
	"Equine",
	"Bird",
	"Goblin",
	"Orc",
	"Githyanki",
	"\n"
};


/* The menu for choosing a race in interpreter.c: */

const char *race_menu[NUM_RACES] =
{
  "-) Animal  ",
  "a) Hyu-Mann",
  "b) Elf     ",
  "c) Dwomax  ",
  "d) Shastick",
  "e) Murrash ",
  "f) Laori   ",
  "g) Thadyri ",
  "h) Terran  ",
  "i) Trulor  ",
  "j) Dragon  ",
  "k) Deva    ",
  "l) Solar   "
};


/*
 * The code to interpret a race letter (used in interpreter.c when a
 * new character is selecting a race).
 */

int parse_race(char *arg)
{
	if (is_abbrev(arg, "animal"))
		return RACE_ANIMAL;
	if (is_abbrev(arg, "hyumann"))
		return RACE_HYUMANN;
	if (is_abbrev(arg, "elf"))
		return RACE_ELF;
	if (is_abbrev(arg, "dwomax"))
		return RACE_DWOMAX;
	if (is_abbrev(arg, "shastick"))
		return RACE_SHASTICK;
	if (is_abbrev(arg, "murrash"))
		return RACE_MURRASH;
	if (is_abbrev(arg, "laori"))
		return RACE_LAORI;
	if (is_abbrev(arg, "thadyri"))
		return RACE_THADYRI;
	if (is_abbrev(arg, "terran"))
		return RACE_TERRAN;
	if (is_abbrev(arg, "trulor"))
		return RACE_TRULOR;
	if (is_abbrev(arg, "dragon"))
		return RACE_DRAGON;
	if (is_abbrev(arg, "deva"))
		return RACE_DEVA;
	if (is_abbrev(arg, "solar"))
		return RACE_SOLAR;
	return RACE_ANIMAL;
}


Flags find_race_bitvector(char arg)
{
  arg = LOWER(arg);

  switch (arg) {
    case '-':
      break;
    case 'a':
      return 1;
      break;
    case 'b':
      return 2;
      break;
    case 'c':
      return 4;
      break;
    case 'd':
      return 8;
      break;
    case 'e':
      return 16;
      break;
    case 'f':
      return 32;
      break;
    case 'g':
      return 64;
      break;
    case 'h':
      return 128;
      break;
    case 'i':
      return 256;
      break;
    case 'j':
      return 512;
      break;
    case 'k':
      return 1024;
      break;
    case 'l':
      return 2048;
      break;
    default:
      break;
  }

  return 0;
}


#ifdef GOT_RID_OF_IT
void remove_racial_affects(Character *ch)
{
	Affect *af = NULL, *temp = NULL;
	int type;

	af = AFFECTS(ch);
	while (af)
	{
		temp = af->next;
		if (!(af->affect_event))
			affect_remove(ch, af);
		af = temp;
	}
}

void apply_racial_affects(Character *ch)
{
	Affect af;
	
	af.bitvector = 0;
	af.modifier = 0;
	af.location = APPLY_NONE;
	af.level = 126;
	af.type = 0;
	af.affect_event = NULL;

	switch(GET_RACE(ch))
	{
	case RACE_ANIMAL:
	case RACE_HYUMANN:
	case RACE_ELF:
	case RACE_DWOMAX:
		break;
	case RACE_SHASTICK:
		af.type = SPELL_SENSE_LIFE;
		af.bitvector = AffectBit::SenseLife;
		affect_to_char(ch, &af, 0);
		break;
	case RACE_MURRASH:
	case RACE_LAORI:
	case RACE_THADYRI:
	case RACE_TERRAN:
	case RACE_TRULOR:
	case RACE_DRAGON:
	case RACE_DEVA:
	case RACE_SOLAR:
	default:
		break;
	}
}
#endif


void roll_height_weight(Character *ch)
{
  switch (GET_RACE(ch)) {
  case RACE_ELF:
    if (GET_SEX(ch) == Male) {
      GET_WEIGHT(ch) = Number(100, 160);
      GET_HEIGHT(ch) = Number(160, 200);
    } else {
      GET_WEIGHT(ch) = Number(90, 130);
      GET_HEIGHT(ch) = Number(150, 180);
    }
    break;

  case RACE_DWOMAX:
    GET_WEIGHT(ch) = Number(100, 160);
    GET_HEIGHT(ch) = Number(140, 170);
    break;

  case RACE_LAORI:
    GET_WEIGHT(ch) = Number(70, 120);
    GET_HEIGHT(ch) = Number(140, 170);
    break;

  case RACE_HYUMANN:
  default:
    if (GET_SEX(ch) == Male) {
      GET_WEIGHT(ch) = Number(120, 180);
      GET_HEIGHT(ch) = Number(160, 200);
    } else {
      GET_WEIGHT(ch) = Number(100, 160);
      GET_HEIGHT(ch) = Number(150, 180);
    }
    break;
  }
}


// Array used to match a race to a bodytype.
const SInt8 race_select_costs[NUM_RACES] =
{
      0, 	// Animal
      0,	// Hyu-Mann
      35,	// Elf
      30,	// Dwomax
      50,	// Shastick
      30,	// Murrash
      35,	// Laori
      50,	// Thadyri
      0,	// Terran
      20,	// Trulor
      80,	// Dragon
      100,	// Deva
      200,	// Solar
      0, 	// Animal
      0, 	// Animal
      0, 	// Animal
      0, 	// Animal
      0, 	// Animal
      0, 	// Animal
      0, 	// Animal
      0, 	// Animal
      0, 	// Animal
      0 	// Animal
};


// Array used to set default race advantages
const Flags race_advantages[NUM_RACES] =
{
      0,				 	// Animal
      0,					// Hyu-Mann
      (Advantages::Infravision | Advantages::FastManaRegen | Advantages::Sapient),	// Elf
      (Advantages::Infravision | Advantages::FastHpRegen),	// Dwomax
      0,					// Shastick
      Advantages::SlowMetabolism,			// Murrash
      (Advantages::NatureRecharge | Advantages::Flight),	// Laori
      (Advantages::Ambidextrous | Advantages::GroupMagicBonus | Advantages::RuneAware),	// Thadyri
      0,	// Terran
      0,	// Trulor
      (Advantages::Flight | Advantages::Infravision),	// Dragon
      (Advantages::Flight | Advantages::FastManaRegen),	// Deva
      (Advantages::Flight | Advantages::FastManaRegen),	// Solar
      0,				 	// Animal
      0,				 	// Animal
      0,				 	// Animal
      0,				 	// Animal
      0,				 	// Animal
      0,				 	// Animal
      0,				 	// Animal
      0,				 	// Animal
      0,				 	// Animal
      0					 	// Animal
};


// Array used to set default race disadvantages
const Flags race_disadvantages[NUM_RACES] =
{
      0,				 	// Animal
      0,					// Hyu-Mann
      Disadvantages::SlowHpRegen,			// Elf
      0,					// Dwomax
      Disadvantages::NecroDestroysFlesh,		// Shastick
      Disadvantages::ColdBlooded,			// Murrash
      0,					// Laori
      0,					// Thadyri
      Disadvantages::NoMagic,						// Terran
      Disadvantages::NoMagic | Disadvantages::Atheist, // Disadvantages::SUSCEPTIBLEDISEASE,		// Trulor
      0,					// Dragon
      0,					// Deva
      0,					// Solar
      0,				 	// Animal
      0,				 	// Animal
      0,				 	// Animal
      0,				 	// Animal
      0,				 	// Animal
      0,				 	// Animal
      0,				 	// Animal
      0,				 	// Animal
      0,				 	// Animal
      0					 	// Animal
};


const int max_stat_table[NUM_RACES][6] = {
/* str int wis dex con cha */
{  15, 15, 15, 15, 15, 15  },   /* Animal   */
{  18, 18, 18, 18, 18, 18  },   /* HyuMann    */
{  18, 18, 18, 19, 17, 18  },   /* Elf      */
{  19, 18, 19, 18, 19, 15  },   /* Dwomax   */
{  20, 20, 20, 20, 20, 20  },   /* Shastick */
{  24, 16, 18, 19, 20, 13  },   /* Murrash  */
{  16, 18, 19, 22, 15, 20  },   /* Laori    */
{  18, 18, 18, 18, 18, 18  },   /* Thadyri  */
{  18, 18, 18, 18, 18, 18  },   /* Terran   */
{  14, 22, 22, 16, 15, 19  },   /* Trulor   */
{  50, 20, 22, 15, 30, 22  },   /* Dragon   */
{  25, 20, 25, 25, 25, 25  },   /* Deva     */
{  30, 30, 30, 30, 30, 30  },   /* Solar    */
{  15, 15, 15, 15, 15, 15  },   /* Animal   */
{  15, 15, 15, 15, 15, 15  },   /* Animal   */
{  15, 15, 15, 15, 15, 15  },   /* Animal   */
{  15, 15, 15, 15, 15, 15  },   /* Animal   */
{  15, 15, 15, 15, 15, 15  },   /* Animal   */
{  15, 15, 15, 15, 15, 15  },   /* Animal   */
{  15, 15, 15, 15, 15, 15  },   /* Animal   */
{  15, 15, 15, 15, 15, 15  },   /* Animal   */
{  15, 15, 15, 15, 15, 15  },   /* Animal   */
{  15, 15, 15, 15, 15, 15  }    /* Animal   */

};


const int mod_stat_table[NUM_RACES][6] = {
	/* str int wis dex con cha */
	{   6,  -7, -8, 6,  5, -2  },   /* Animal   */
	{   0,  0,  0,  0,  0,  0  },   /* HyuMann  */
	{   0,  0,  0,  1, -1,  0  },   /* Elf      */
	{   0,  0,  1,  0,  1,  0  },   /* Dwomax   */
	{  -2,  0,  0, -2, -2, -2  },   /* Shastick */
	{   2, -2, -2,  0,  2, -2  },   /* Murrash  */
	{  -2,  0,  1,  2, -3,  1  },   /* Laori    */
	{   0,  0,  0,  0,  0,  0  },   /* Thadyri  */
	{   0,  0,  0,  0,  0,  0  },   /* Terran   */
	{  -4,  4,  4, -2, -3,  1  },   /* Trulor   */
	{   4,  0,  2, -5,  0, -1  },   /* Dragon   */
	{   0,  0,  0,  0,  0,  0  },   /* Deva     */
	{   0,  0,  0,  0,  0,  0  },   /* Solar    */
	{   6,  -7, -8, 6,  5, -2  },   /* Animal   */
	{   6,  -7, -8, 6,  5, -2  },   /* Animal   */
	{   6,  -7, -8, 6,  5, -2  },   /* Animal   */
	{   6,  -7, -8, 6,  5, -2  },   /* Animal   */
	{   6,  -7, -8, 6,  5, -2  },   /* Animal   */
	{   6,  -7, -8, 6,  5, -2  },   /* Animal   */
	{   6,  -7, -8, 6,  5, -2  },   /* Animal   */
	{   6,  -7, -8, 6,  5, -2  },   /* Animal   */
	{   6,  -7, -8, 6,  5, -2  },   /* Animal   */
	{   6,  -7, -8, 6,  5, -2  }   /* Animal   */
};

