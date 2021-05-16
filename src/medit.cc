/************************************************************************
 * OasisOLC - medit.c                                              v1.5 *
 * Copyright 1996 Harvey Gilpin.                                        *
 ************************************************************************/


#include "characters.h"
#include "descriptors.h"
#include "comm.h"
#include "utils.h"
#include "scripts.h"
#include "db.h"
#include "shop.h"
#include "olc.h"
#include "interpreter.h"
#include "clans.h"
#include "buffer.h"
#include "opinion.h"
#include "constants.h"
#include "skills.h"
#include "medit.h"
#include "combat.h"

//	External variable declarations.
extern struct zone_data *zone_table;
extern int top_of_zone_table;
extern PlayerData dummy_player;
extern int stat_min[NUM_RACES + 1][5];
extern int stat_max[NUM_RACES + 1][5];


//	External functions.
extern int olc_edit_bitvector(Descriptor *d, Flags *result, char *argument,
		       Flags old_bv, char *name, const char **values, Flags mask);

//	Handy internal macros.
#define GET_NDD(themob)			((themob)->mob->damage.number)
#define GET_SDD(themob)			((themob)->mob->damage.size)
#define GET_ATTACK(themob)		((themob)->mob->attack_type)
#define S_KEEPER(shop)			((shop)->keeper)

/*
 * Function prototypes.
 */
void init_mobile(Character *mob);
void copy_mobile(Character *tmob, Character *fmob);


/*-------------------------------------------------------------------*\
  utility functions 
\*-------------------------------------------------------------------*/


MEdit::MEdit(Descriptor *d, VNum newVNum) : Editor(d), 
		opinion(NULL) {
	mob = new Character;
	
	init_mobile(mob);
	
	//	Set up some default strings.
	mob->SetKeywords("mob unfinished");
	mob->SetName("the unfinished mob");
	mob->SetSDesc("An unfinished mob stands here.\r\n");
	mob->SetLDesc("It looks unfinished.\r\n");

	mob->Virtual(number = newVNum);
	value = 0;		/* Has changed flag. (It hasn't so far, we just made it.) */
	save = OLC_SAVE_NO;
  
	Menu();
}


/*-------------------------------------------------------------------*/

MEdit::MEdit(Descriptor *d, VNum newVNum, IndexData<Character> &mobEntry) : Editor(d), 
		opinion(NULL) {
	mob = new Character(*mobEntry.proto);

	if (mob->Virtual() != newVNum) {
		if (Character::Find(newVNum))	save = OLC_SAVE_OVERWRITE;
		else							save = OLC_SAVE_COPY;
	}

	copy_mobile(mob, mobEntry.proto);
	mob->Virtual(number = newVNum);
	
	triggers = mobEntry.triggers;

	Menu();
}

/*-------------------------------------------------------------------*/
/*
 * Copy one mob struct to another.
 */


void copy_mobile(Character *tmob, Character *fmob) {
	tmob->SetKeywords(fmob->SSKeywords() ? fmob->Keywords() : "undefined");
	tmob->SetName(fmob->SSName() ? fmob->Name() : "undefined");
	tmob->SetSDesc(fmob->SSSDesc() ? fmob->SDesc() : "undefined");
	tmob->SetLDesc(fmob->SSLDesc() ? fmob->LDesc() : "undefined");
}



/*
 * Ideally, this function should be in db.c, but I'll put it here for
 * portability.
 */
void init_mobile(Character *mob) {
	mob->player = &dummy_player;
	mob->mob = new MobData;
	
	GET_RACE(mob) = RACE_HYUMANN;
	
	// GET_HIT(mob) = GET_MANA(mob) = 1;
	GET_MAX_MANA(mob) = GET_MAX_MOVE(mob) = 100;
	GET_NDD(mob) = GET_SDD(mob) = 1;

	SET_BIT(MOB_FLAGS(mob), MOB_ISNPC);
}

/*-------------------------------------------------------------------*/

#define ZCMD zone_table[zone].cmd[cmd_no]

/*
 * Save new/edited mob to memory.
 */
void MEdit::SaveInternally(void) {
//	int		zone, cmd_no, shop;
	bool	found = false;
	Character *live_mob; //, *proto;
//	Descriptor *dsc;
	LListIterator<Character *>	iter(Characters);
	
	REMOVE_BIT(MOB_FLAGS(mob), MOB_DELETED);

	if (IS_SIMPLE_MOB(mob))		render_simple_mob(mob);
	
	//	Mob exists? Just update it.
	if (Character::Find(number)) {
		delete Character::Index[number].proto;
		Character::Index[number].func = func;
		Character::Index[number].triggers = triggers;
		Character::Index[number].proto = mob;
	} else {
		Character::Index[number].vnum = number;
		Character::Index[number].number = 0;
		Character::Index[number].func = func;
		Character::Index[number].triggers = triggers;
		Character::Index[number].proto = mob;
	}

	olc_add_to_save_list(zone_table[zoneNum].number, OLC_MEDIT);
}


void MEdit::Finish(FinishMode mode) {
	if (mode == All)		delete mob;
	
	if (d->character) {
		STATE(d) = CON_PLAYING;
		act("$n stops using OLC.", TRUE, d->character, 0, 0, TO_ROOM);
	}
	Editor::Finish();
}


/**************************************************************************
 Menu functions 
 **************************************************************************/

/*-------------------------------------------------------------------*/

/*
 * Display positions. (sitting, standing, etc)
 */
void MEdit::DispPositions(void) {
#if defined(CLEAR_SCREEN)
	d->Write("\x1B[H\x1B[J");
#endif
	mob->Edit(d, "", mode, "");
	d->Write("&c0Enter position number: ");
}

/*-------------------------------------------------------------------*/

/*
 * Display the gender of the mobile.
 */
void MEdit::DispSex(void) {
#if defined(CLEAR_SCREEN)
	d->Write("\x1B[H\x1B[J");
#endif
	mob->Edit(d, "", mode, "");
	d->Write("&c0Enter gender number: ");
}

/*-------------------------------------------------------------------*/

/*
 * Display attack types menu.
 */
void MEdit::DispAttackTypes(void) {
#if defined(CLEAR_SCREEN)
	d->Write("\x1B[H\x1B[J");
#endif
	mob->Edit(d, "", mode, "");
	d->Write("&c0Enter attack type: ");
}
 
/*-------------------------------------------------------------------*/

/*
 * Display races menu.
 */
void MEdit::DispRaces(void) {
#if defined(CLEAR_SCREEN)
	d->Write("\x1B[H\x1B[J");
#endif
	mob->Edit(d, "", mode, "");
	d->Write("&c0Enter race: ");
}


/*-------------------------------------------------------------------*/

/*
 * Display mob-flags menu.
 */
void MEdit::DispMobFlags(void) {
#if defined(CLEAR_SCREEN)
	d->Write("\x1B[H\x1B[J");
#endif
	mob->Edit(d, "", mode, "");
}

/*-------------------------------------------------------------------*/

/*
 * Display affection flags menu.
 */
void MEdit::DispAffFlags(void) {
#if defined(CLEAR_SCREEN)
	d->Write("\x1B[H\x1B[J");
#endif
	mob->Edit(d, "", mode, "");
}
  
/*-------------------------------------------------------------------*/

/*
 * Display main menu.
 */
void MEdit::Menu(void) 
{
	sprintbit(MOB_FLAGS(mob), action_bits, buf1);
	sprintbit(AFF_FLAGS(mob), affected_bits, buf2);

	for(int i = 0; i < triggers.Count(); ++i) {
		sprintf(buf3 + strlen(buf3), "%d ", triggers[i]);
	}
	
	d->Writef(
#if defined(CLEAR_SCREEN)
				"\x1B[H\x1B[J"
#endif
		"-- Mob Number:  [&cC%d&c0]    %s%s\r\n"
	 	"&cG1&c0) Keywords: &cY%s\r\n"
	 	"&cG2&c0) Name: &cY%s\r\n"
	 	"&cG3&c0) S-Desc:-\r\n&cY%s"
	 	"&cG4&c0) L-Desc:-\r\n&cY%s"
		"&cG5&c0) Sex         : &cY%-15.15s"
		"&cGB&c0) Position    : &cY%-15.15s\r\n"
		"&cG6&c0) Race        : &cY%-15.15s"
		"&cGC&c0) Default     : &cY%-15.15s\r\n"
		"&cG7&c0) Class       : &cY%-15.15s"
		"&cGD&c0) Attack      : &cY%-15.15s\r\n"
		"&cG8&c0) Gold        : &cC%-15d"
		"&cGE&c0) Max Riders  : &cC%-15d\r\n"
		"&cG9&c0) Alignment   : &cC%-15d"
		"&cGF&c0) Disposition : &cC%-15d\r\n"
		"&cGA&c0) XP Bonus    : &cC%-15d"
		"&cGG&c0) Bodytype    : &cC%-15s\r\n"
		"&cGH&c0) NPC Flags : &cC%s\r\n"
		"&cGI&c0) AFF Flags : &cC%s\r\n"
		"&cGO&c0) Opinions Menu\r\n"
		"&cGP&c0) Special procedure : &cC%s\r\n",

	number,
    (save == OLC_SAVE_COPY) ? "&cW&bb[ Copy ]&c0" : "",
    (save == OLC_SAVE_OVERWRITE) ? "&cW&br[ OVERWRITING ]&c0" : "",
	mob->Keywords(),
	mob->Name(),
	mob->SDesc(),
	mob->LDesc(),

	genders[(int)GET_SEX(mob)],
	position_types[(int)GET_POS(mob)],
	race_types[(int)GET_RACE(mob)],
	position_types[(int)GET_DEFAULT_POS(mob)],
	class_types[(int)GET_CLASS(mob)],
	Skill::Name(MIN(MAX(GET_ATTACK(mob) + TYPE_HIT, 0), TOP_SKILLO_DEFINE)),
	GET_GOLD(mob),
	GET_MAX_RIDERS(mob), 
	GET_ALIGNMENT(mob),
	GET_DISPOSITION(mob), 
	GET_EXP_MOD(mob),
	bodytypes[GET_BODYTYPE(mob)],
	buf1,
	buf2,
	(func == NULL) ? "None" : spec_mob_name(func));


	*buf1 = '\0';
	for (int i = 0; i < triggers.Count(); ++i) {
		sprintf(buf1 + strlen(buf1), "%d ", triggers[i]);
	}

	d->Writef("&cGS&c0) Scripts     : &cC%s\r\n"
			  "&cGT&c0) Mobile type : &cY%-15s&c0",
			*buf1 ? buf1 : "None",
			mobile_types[MOB_TYPE(mob)]);

	if (IS_SIMPLE_MOB(mob)) {
		sprintf(buf, "&cGX&c0) Total Points : &cC%d\r\n", 
						GET_TOTALPTS(mob) + GET_EXP_MOD(mob));
	} else {
		// CHANGEPOINT:  A glitch in this and similar code causes drift in point values.

		GET_MAX_HIT(mob) = MAX(1, (mob->mob->numhitdice * mob->mob->sizehitdice / 2) + mob->mob->modhitdice);
		GET_MAX_MANA(mob) = GET_INT(mob) * 10;
		GET_MAX_MOVE(mob) = GET_STR(mob) * 10;

		GET_TOTALPTS(mob) = sum_char_pts(mob);
		sprintf(buf, "Total Points : %d\r\n"
									"&cGX&c0) Extended stats\r\n", 
		GET_TOTALPTS(mob));
	}

	strcat(buf, "&cGQ&c0) Quit\r\nEnter choice : ");

	d->Write(buf);

	mode = MainMenu;
}


void MEdit::DispExtendedMenu(void) 
{
		GET_MAX_HIT(mob) = MAX(1, (mob->mob->numhitdice * mob->mob->sizehitdice / 2) + mob->mob->modhitdice);
		GET_MAX_MANA(mob) = GET_INT(mob) * 10;
		GET_MAX_MOVE(mob) = GET_STR(mob) * 10;

		GET_TOTALPTS(mob) = sum_char_pts(mob);

#if defined(CLEAR_SCREEN)
				"\x1B[H\x1B[J"
#endif
	d->Writef(
		"&ccTotal Points : &cW%d&c0\r\n\n"
	 	"&cG1&c0) Strength:      [&cC%5d&c0]  &cG7&c0) Hitroll:       [&cC%5d&c0]\r\n"
		"&cG2&c0) Intelligence:  [&cC%5d&c0]  &cG8&c0) Damroll:       [&cC%5d&c0]\r\n"
		"&cG3&c0) Wisdom:        [&cC%5d&c0]  &cG9&c0) Dam Resist:    [&cC%5d&c0]\r\n"
		"&cG4&c0) Dexterity:     [&cC%5d&c0]  &cGA&c0) Num HP Dice:   [&cC%5d&c0]\r\n"
		"&cG5&c0) Constitution:  [&cC%5d&c0]  &cGB&c0) Size HP Dice:  [&cC%5d&c0]\r\n"
		"&cG6&c0) Charisma:      [&cC%5d&c0]  &cGC&c0) HP Bonus:      [&cC%5d&c0]\r\n\n"
		"&cGL&c0) Level:         [&cC%5d&c0]  &cGS&c0) Skills\r\n"
		"&cGQ&c0) Quit\r\n"
		"Enter choice : ",
		GET_TOTALPTS(mob),
		GET_STR(mob), GET_HITROLL(mob), 
		GET_INT(mob), GET_DAMROLL(mob), 
		GET_WIS(mob), GET_DR(mob),
		GET_DEX(mob), mob->mob->numhitdice,
		GET_CON(mob), mob->mob->sizehitdice,
		GET_CHA(mob), mob->mob->modhitdice,
		GET_LEVEL(mob));

	mode = ExtendedMenu;
}


/*-------------------------------------------------------------------*/

// Display the mob's skills - DH.

void MEdit::DispSkills(void)
{
	SkillKnown *skill = NULL;

#if defined(CLEAR_SCREEN)
	d->Write("[H[J");
#endif

	CHAR_SKILL(mob).GetTable(&skill);

	strcpy(buf, "\r\nSkills known:\r\n\r\n");
	
	if (skill == NULL) {
		strcat(buf, " - NONE!\r\n");
	} else {
		strcat(buf, "&cYSkill                     Points Chance\r\n");

		while (skill) {
			sprintf(buf + strlen(buf), "&cG%-25.25s    &cB%3d    %3d\r\n", Skill::Name(skill->skillnum),
					skill->proficient, SKILLCHANCE(mob, skill->skillnum, skill));
			skill = skill->next;
		}
	}

	strcat(buf, "\r\n&c0Enter a skill name to add or edit (0 quits) : ");
	
	d->Write(buf);
}


void MEdit::DispOpinion(Opinion *op, SInt32 num) {
	if (op->sex) {
		sprintbit(op->sex, genders, buf);
		d->Writef("   &cG%1d&c0) Sexes : %s\r\n", num, buf);
	} else
		d->Writef("   &cG%1d&c0) Sexes : <NONE>\r\n", num);
	
	if (op->race) {
		sprintbit(op->race, race_types, buf);
		d->Writef("   &cG%1d&c0) Races : %s\r\n", num+1, buf);
	} else
		d->Writef("   &cG%1d&c0) Races : <NONE>\r\n", num+1);
	
	if (op->vnum != NOBODY) {
		d->Writef("   &cG%1d&c0) VNum  : %d (%s)\r\n", num+2, op->vnum,
				Character::Find(op->vnum) ?
				(Character::Index[op->vnum].proto->SSName() ?
				Character::Index[op->vnum].proto->Name() :
				"Unnamed") : "Invalid VNUM");
	} else
		d->Writef("   &cG%1d&c0) Vnum  : <NONE>\r\n", num+2);
}


//	Opinions menus
void MEdit::DispOpinionMenu(void) {
	d->Write("-- Opinions :\r\n");
	d->Write("Hates:\r\n");	
	DispOpinion(mob->mob->hates, 1);
	d->Write("\r\n"
			"Fears:\r\n");	
	DispOpinion(mob->mob->fears, 4);
	d->Write("\r\n"
				"&cG0&c0) Exit\r\n"
				"&c0Enter choice:  ");
	mode = OpinionsMenu;
}


//	Display Opinion gender flags.
void MEdit::DispOpinionGenders(void) {
	olc_edit_bitvector(d, &(opinion->sex), "", opinion->sex, "genders", genders, 0);
}


//	Display Opinion races
void MEdit::DispOpinionRaces(void) {
	olc_edit_bitvector(d, &(opinion->race), "", opinion->race, "races", race_types, 0);
}


void MEdit::Parse(char *arg) {
	int i;
	
#ifdef GOT_RID_OF_IT
	if (mode > MobNumericalResponse) {
		if(*arg && !is_number(arg)) {
			d->Write("Field must be numerical, try again : ");
			return;
		}
	}
#endif

	switch (mode) {
	case ConfirmSave:
		SET_BIT(MOB_FLAGS(mob), MOB_ISNPC);
		switch (*arg) {
		case 'y':
		case 'Y':
			d->Write("Saving mobile to memory.\r\n");
			mudlogf(NRM, d->character, true, "OLC: %s edits mob %d", d->character->RealName(), number);
			SaveInternally();
			Finish(Structs);
			break;
		case 'n':
		case 'N':
			Finish(All);
			break;
		default:
			d->Write("Invalid choice!\r\n"
					"Do you wish to save the mobile? ");
		}
		return;

/*-------------------------------------------------------------------*/
	case MainMenu:
		i = 0;
		switch (*arg) {
		case 'q':
		case 'Q':
			if (save) {	//	Anything been changed?
				d->Write("Do you wish to save this mobile internally? (y/n) : ");
				mode = ConfirmSave;
			} else
				Finish(All);
			return;
		case '1':
			mode = Character::MobKeywords;
			--i;
			break;
		case '2':
			mode = Character::MobName;
			--i;
			break;
		case '3':
			mode = Character::MobSDesc;
			--i;
			break;
		case '4':
			mode = Character::MobLDesc;
			if (!save)	save = OLC_SAVE_YES;
			mob->Edit(d, "", mode, "");
			return;
		case '5':
			mode = Character::MobSex;
			DispSex();
			return;
		case '6':
			mode = Character::MobRace;
			DispRaces();
			++i;
			break;
		case '7':
			mode = Character::MobClass;
			mob->Edit(d, "", mode, "");
			d->Write("Enter choice : ");
			return;
		case '8':
			mode = Character::MobGold;
			++i;
			break;
		case '9':
			mode = Character::MobAlignment;
			++i;
			break;
		case 'a':
		case 'A':
			mode = Character::MobExpBonus;
			++i;
			break;
		case 'b':
		case 'B':
			mode = Character::MobPosition;
			DispPositions();
			return;
		case 'c':
		case 'C':
			mode = Character::MobDefPosition;
			DispPositions();
			return;
		case 'd':
		case 'D':
			mode = Character::MobAttackType;
			DispAttackTypes();
			return;
		case 'e':
		case 'E':
			mode = Character::MobMaxRiders;
			++i;
			break;
		case 'f':
		case 'F':
			mode = Character::MobDisposition;
			++i;
			break;
		case 'g':
		case 'G':
			mode = Character::MobBodytype;
			mob->Edit(d, "", mode, "");
			++i;
			break;
		case 'h':
		case 'H':
			mode = Character::MobNPCFlags;
			mob->Edit(d, "", mode, "");
			return;
		case 'i':
		case 'I':
			mode = Character::MobAffFlags;
			mob->Edit(d, "", mode, "");
			return;
		case 'p':
		case 'P':
			DispSpecProcMenu(d, mob);
			mode = SpecProcMenu;
			d->Write("Enter the name of the desired specproc, or \"none\" to clear: ");
			return;
		case 'o':
		case 'O':
			if (!mob->mob->hates)	mob->mob->hates = new Opinion();
			if (!mob->mob->fears)	mob->mob->fears = new Opinion();
			DispOpinionMenu();
			return;
		case 's':
		case 'S':
			DispScriptsMenu(d, triggers);
			return;
		case 't':
		case 'T':
			mode = Character::MobType;
			mob->Edit(d, "", mode, "");
			return;
		case 'x':
		case 'X':
			if (IS_SIMPLE_MOB(mob)) {
				++i;
				mode = Character::MobTotalPoints;
			} else {
				DispExtendedMenu();
				return;
			}
			break;
			
		default:
			Menu();
			return;
		}
		switch (i) {
		case -1:	d->Write("\r\nEnter new text :\r\n] ");		return;
		case 1:		d->Write("\r\nEnter new value : ");			return;
		default:												break;
		}
		
		break;

/*-------------------------------------------------------------------*/
	case Character::MobKeywords:
		mob->SetKeywords(*arg ? arg : "undefined"); 
		break;
/*-------------------------------------------------------------------*/
	case Character::MobName:
		mob->SetName(*arg ? arg : "undefined"); 
		break;
/*-------------------------------------------------------------------*/
	case Character::MobSDesc:
		if (*arg) {
			strcpy(buf, arg);
		} else {
			strcpy(buf, "undefined");
		}
		strcat(buf, "\r\n");
		mob->SetSDesc(buf);
		break;
/*-------------------------------------------------------------------*/
	case Character::MobLDesc:
		/*
		 * We should never get here.
		 */
//			cleanup_olc(d, CLEANUP_ALL);
		mudlog("SYSERR: medit_parse(): Reached D_DESC case!", BRF, NULL, true);
		d->Write("Oops...\r\n");
		Menu();
		return;
/*-------------------------------------------------------------------*/
// Generic bitvector editing.

	case Character::MobAffFlags:
	case Character::MobNPCFlags:
		switch (*arg && mob->Edit(d, "", mode, arg)) {
		case 0: 
			Menu(); 
			return;
		default: 
			if (!save)	save = OLC_SAVE_YES;
			mob->Edit(d, "", mode, "");
			return;
		}
		return;
/*-------------------------------------------------------------------*/
// Numerical responses

// On main menu
	case Character::MobExpBonus:
	case Character::MobGold:
	case Character::MobPosition:
	case Character::MobDefPosition:
	case Character::MobAttackType:
	case Character::MobAlignment:
	case Character::MobRace:
	case Character::MobClass:
	case Character::MobMaxRiders:
	case Character::MobType:
	case Character::MobBodytype:
	case Character::MobDisposition:
	case Character::MobSex:
	case Character::MobTotalPoints:
		switch (*arg && mob->Edit(d, "", mode, arg)) {
		case 0: break;
		default: 
			if (!save)	save = OLC_SAVE_YES;
		}
		Menu(); 
		return;

	case Character::MobStr: 	
	case Character::MobInt: 	
	case Character::MobWis: 	
	case Character::MobDex: 	
	case Character::MobCon: 	
	case Character::MobCha: 	
		switch (*arg && mob->Edit(d, "", mode, arg)) {
		case 0: break;
		default: 
			if (!save)	save = OLC_SAVE_YES;
			mob->real_abils = mob->aff_abils;
		}
		DispExtendedMenu();
		return;

	case Character::MobHitroll: 
	case Character::MobDamroll: 
	case Character::MobDamResist:
	case Character::MobNumHPDice: 
	case Character::MobSizeHPDice:
	case Character::MobAddHP:	  
	case Character::MobLevel:	
		switch (*arg && mob->Edit(d, "", mode, arg)) {
		case 0: break;
		default: 
			if (!save)	save = OLC_SAVE_YES;
		}
		DispExtendedMenu();
		return;

	case SkillMenu:
		if ((!*arg) || (*arg == '0')) {
			mode = ExtendedMenu;
			DispExtendedMenu();
			return;
		}
		value = Skill::Number(arg);
		if (!LEARNABLE_SKILL(value) || value == -1) {
			d->Write("That is not a learnable skill.	Try again.\r\n"
					"Enter a skill name to add or edit (0 quits) : ");
			return;
		}
		mode = SkillPoints;
		d->Write("Enter the points invested : ");
		return;

	case SkillPoints:
		SET_SKILL(mob, value, MIN(255, MAX(atoi(arg), 0)));
		mode = SkillMenu;
		value = 0;
		if (!save)	save = OLC_SAVE_YES;
		DispSkills();
		return;

	case SpecProcMenu:
		if (*arg) {
			ParseSpecProcMenu(d, arg, mob);
			Menu();
			save = OLC_SAVE_YES;
		} else {
			Menu();
		}
		return;

	case ExtendedMenu:
	// Extended menu
		switch(*arg) {
		case '1':	mode = Character::MobStr;			++i;	break;
		case '2':	mode = Character::MobInt;			++i;	break;
		case '3':	mode = Character::MobWis;			++i;	break;
		case '4':	mode = Character::MobDex;			++i;	break;
		case '5':	mode = Character::MobCon;			++i;	break;
		case '6':	mode = Character::MobCha;			++i;	break;
		case '7':	mode = Character::MobHitroll;		++i;	break;
		case '8':	mode = Character::MobDamroll;		++i;	break;
		case '9':	mode = Character::MobDamResist;		++i;	break;
		case 'a':
		case 'A':	mode = Character::MobNumHPDice;		++i;	break;
		case 'b':
		case 'B':	mode = Character::MobSizeHPDice;  	++i;	break;
		case 'c':
		case 'C':	mode = Character::MobAddHP;			++i;	break;
		case 'l':
		case 'L':	mode = Character::MobLevel; 		++i;	break;
		case 's':
		case 'S':	mode = SkillMenu;			DispSkills();	return;
		case 'q':
		case 'Q':	Menu();										return;
		default: 	DispExtendedMenu();							return;
		}
		return;


	case TriggersMenu:
	case TriggerAdd:
	case TriggerPurge:
		ParseScriptsMenu(d, arg, triggers);
		return;
		
	case MobClan:
		i = atoi(arg);
		if (Clan::Find(i))
			GET_CLAN(mob) = i;
		break;
	
	
//		Opinions Menu
	case OpinionsMenu:
		opinion = NULL;
		switch (*arg) {
			//	Hates
			case '1':	// Sex
				opinion = mob->mob->hates;
				DispOpinionGenders();
				mode = OpinionsHateSex;
				break;
			case '2':	// Race
				opinion = mob->mob->hates;
				DispOpinionRaces();
				mode = OpinionsHateRace;
				break;
			case '3':	// VNum
				opinion = mob->mob->hates;
				d->Write("VNum (-1 for none): ");
				mode = OpinionsHateVNum;
				break;
			
			//	Fears
			case '4':	// Sex
				opinion = mob->mob->fears;
				DispOpinionGenders();
				mode = OpinionsFearSex;
				break;
			case '5':	// Race
				opinion = mob->mob->fears;
				DispOpinionRaces();
				mode = OpinionsFearRace;
				break;
			case '6':	// VNum
				opinion = mob->mob->fears;
				d->Write("VNum (-1 for none): ");
				mode = OpinionsFearVNum;
				break;
				
			case '0':
				Menu();
				if (!mob->mob->hates->sex && !mob->mob->hates->race &&
						(mob->mob->hates->vnum == NOBODY)) {
					delete mob->mob->hates;
					mob->mob->hates = NULL;
				}
				if (!mob->mob->fears->sex && !mob->mob->fears->race &&
						(mob->mob->fears->vnum == NOBODY)) {
					delete mob->mob->fears;
					mob->mob->fears = NULL;
				}
				return;
		}
		return;
	
	case OpinionsHateSex:
	case OpinionsFearSex:
		switch (*arg && olc_edit_bitvector(d, &(opinion->sex), arg, opinion->sex, 
                     "genders", genders, 0)) {
		case 0:
			DispOpinionMenu();
			return;
		default: 
			if (!save)	save = OLC_SAVE_YES;
			DispOpinionGenders();
			return;
		}

#if 0
		if ((i = atoi(arg))) {
			if (i < NUM_GENDERS) {
				if (mode == OpinionsHateSex)
					TOGGLE_BIT(mob->mob->hates->sex, 1 << (i - 1));
				else
					TOGGLE_BIT(mob->mob->fears->sex, 1 << (i - 1));
			}
			DispOpinionGenders();
			if (!save)	save = OLC_SAVE_YES;
		} else
			DispOpinionMenu();
		return;
#endif
	
	case OpinionsHateRace:
	case OpinionsFearRace:
		switch (*arg && olc_edit_bitvector(d, &(opinion->race), arg, opinion->race, 
                     "races", race_types, 0)) {
		case 0:
			DispOpinionMenu();
			return;
		default: 
			if (!save)	save = OLC_SAVE_YES;
			DispOpinionRaces();
			return;
		}

#if 0
		if ((i = atoi(arg))) {
			if (i < (NUM_RACES + 1)) {
				if (mode == OpinionsHateRace)
					TOGGLE_BIT(mob->mob->hates->race, 1 << (i - 1));
				else
					TOGGLE_BIT(mob->mob->fears->race, 1 << (i - 1));
			}
			DispOpinionRaces();
			if (!save)	save = OLC_SAVE_YES;
		} else
			DispOpinionMenu();
		return;
#endif
	
	case OpinionsHateVNum:
	case OpinionsFearVNum:
		i = atoi(arg);
		if (Character::Find(i)) {
			opinion->vnum = i;
			if (!save)	save = OLC_SAVE_YES;
		}
		DispOpinionMenu();
		return;
	
	default:
		//	We should never get here.
//			cleanup_olc(d, CLEANUP_ALL);
		mudlogf(BRF, NULL, true, "SYSERR: MEdit::Parse(arg = %s): Reached default case!  Case: %d", arg, mode);
		d->Write("Oops...\r\n");
//			return;
		break;
}

	save = 1;
	Menu();
}
