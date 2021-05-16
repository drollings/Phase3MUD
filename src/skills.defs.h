/* ************************************************************************
*  File: skills.h										  Part of TLO Phase ][	  *
*																								 *
*  Usage: Contains routines related to the SkillStore class,		  *
*	  which contains a linked list of the player's skills.		  *
*  This code is Copyright (C) 1997 by Daniel Rollings AKA Daniel Houghton.			  *
*																								 *
*  $Author: sfn $
*  $Date: 2001/02/04 15:57:58 $
*  $Revision: 1.5 $
************************************************************************ */


#ifndef _SKILLS_DEFS_H_
#define _SKILLS_DEFS_H_

extern char *spells[];

#define IS_WEAPONSKILL(skillnum)	(((skillnum) >= SKILL_WEAPON_R) && \
  					((skillnum) <= SKILL_WEAPON_4))

#define LEARNABLE_SKILL(sklnum)	(!IS_SET(Skills[sklnum]->types, Skill::DEFAULT | Skill::STAT | Skill::MESSAGE))
#define VALID_SKILL(sklnum)		((sklnum < Skills.Count()) && Skills[sklnum]->types)

#define SKL_IS_RUNE(sklnum)			(IS_SET(Skills[sklnum]->types, Skill::ROOT))
#define SKL_IS_METHOD(sklnum)		(IS_SET(Skills[sklnum]->types, Skill::METHOD))
#define SKL_IS_CSPHERE(sklnum)		(IS_SET(Skills[sklnum]->types, Skill::SPHERE))
#define SKL_IS_ORDINATION(sklnum)	(IS_SET(Skills[sklnum]->types, Skill::ORDINATION))
#define SKL_IS_SKILL(sklnum)		(IS_SET(Skills[sklnum]->types, Skill::GENERAL))
#define SKL_IS_WEAPONPROF(sklnum)	(IS_SET(Skills[sklnum]->types, Skill::WEAPON))
// This is horrible.  As soon as I eliminate dependencies on skillnums for
// skills, this can go.
#define SKL_IS_COMBATSKILL(sklnum)	(IS_SET(Skills[sklnum]->types, Skill::COMBAT))
#define SKL_IS_SPELL(sklnum)		(IS_SET(Skills[sklnum]->types, Skill::SPELL))
#define SKL_IS_PRAYER(sklnum)		(IS_SET(Skills[sklnum]->types, Skill::PRAYER))
#define SKL_IS_STAT(sklnum)			(IS_SET(Skills[sklnum]->types, Skill::STAT))
#define SKL_IS_COMBATMSG(sklnum)	(IS_SET(Skills[sklnum]->types, Skill::MESSAGE))

/* PLAYER SPELLS -- Numbered from 1 to MAX_SPELLS */


#define TYPE_UNDEFINED			-1

#ifndef FIRST_SKILLO
#define FIRST_SKILLO			1
#endif

#define FIRST_RUNE			1
#define FIRST_SPHERE			1
#define SPHERE_AIR			1
#define SPHERE_WIND			2
#define SPHERE_LIGHTNING		3
#define SPHERE_SOUND			4

#define SPHERE_FIRE			5
#define SPHERE_LIGHT			6
#define SPHERE_HEAT			7

#define SPHERE_WATER			8
#define SPHERE_MIST			9
#define SPHERE_ICE			10

#define SPHERE_EARTH			11
#define SPHERE_STONE			12
#define SPHERE_METAL			13

#define SPHERE_CHI			14
#define SPHERE_LIFE			15
#define SPHERE_NECRO			16
#define SPHERE_FLORA			17
#define SPHERE_FAUNA			18

#define SPHERE_PSYCHE			19
#define SPHERE_SPIRIT			20
#define SPHERE_ILLUSION			21

#define SPHERE_VIM			22
#define LAST_SPHERE			22

#define FIRST_TECHNIQUE			23
#define TECH_CREATOR			23
#define TECH_NEGATOR			24
#define TECH_BUILDER			25
#define TECH_DESTROYER			26
#define TECH_ATTRACT			27
#define TECH_REPEL			28
#define TECH_CONTROL			29
#define TECH_AURA			30
#define TECH_INFUSE			31
#define TECH_SCRY			32
#define TECH_TRANSMUTE			33
#define TECH_RADIATE			34
#define TECH_CONSUME			35
#define TECH_WARD			36
#define LAST_TECHNIQUE			36
#define LAST_RUNE			36


#define FIRST_METHOD			40
#define METHOD_CHANT			40
#define METHOD_SOMATIC			41
#define METHOD_THOUGHT			42
#define METHOD_RUNE				43
#define LAST_METHOD				44

#define FIRST_ORDINATION		60
#define ORDINATION_PELENDRA		60
#define ORDINATION_TALVINDAS		61
#define ORDINATION_DYMORDIAN		62
#define ORDINATION_KHARSUS		63
#define ORDINATION_NYMRIS		64
#define ORDINATION_VAADH		65
#define ORDINATION_SHESTU		66
#define LAST_ORDINATION			66

#define FIRST_CSPHERE			80
#define CSPHERE_HEALING			80
#define CSPHERE_DIVINATION		81
#define CSPHERE_NATURE			82
#define CSPHERE_PROTECTION		83
#define CSPHERE_WAR			84
#define CSPHERE_HEAVENS			85
#define CSPHERE_NECRO			86
#define CSPHERE_CREATION		87
#define LAST_CSPHERE			87

/* Insert new spells here, up to MAX_SPELLS */
#define FIRST_SKILL			100

/* PLAYER SKILLS - Numbered from MAX_SPELLS+1 to MAX_SKILLS */
#define FIRST_COMBATSKILL1		101
#define SKILL_BACKSTAB			101 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_BASH			102 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_HIDE			103 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_KICK			104 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_PICK_LOCK			105 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_PUNCH			106 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_RESCUE			107 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_SNEAK			108 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_STEAL			109 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_BERSERK			110
#define SKILL_SEARCH			111
#define SKILL_DISARM			112
#define SKILL_RIDE			113
#define SKILL_FLIP			114
#define SKILL_HEADBUTT			115
#define SKILL_CAMOFLAUGE		116
#define SKILL_IRON_FIST			117
#define SKILL_STRANGLE			118
#define SKILL_CIRCLE			119
#define SKILL_SHIELD			120
#define SKILL_BRAWLING			121
#define SKILL_FAST_DRAW			123
#define SKILL_JUDO				124
#define SKILL_KARATE			125
#define LAST_COMBATSKILL1		125

#define SKILL_SWIMMING			126
#define SKILL_BARD			127
#define SKILL_ANIMAL_HANDLING		128
#define SKILL_FALCONRY			129
#define SKILL_BLACKSMITH		131
#define SKILL_CARPENTRY			132
#define SKILL_COOKING			133
#define SKILL_LEATHERWORKING		135
#define SKILL_ELVEN_LANGUAGE		137
#define SKILL_ELVEN_WLANGUAGE		138
#define SKILL_ELVEN_LORE		139
#define SKILL_HYUMANN_LANGUAGE		140
#define SKILL_HYUMANN_WLANGUAGE		141
#define SKILL_HYUMANN_LORE		142
#define SKILL_LAORI_LANGUAGE		143
#define SKILL_LAORI_WLANGUAGE		144
#define SKILL_LAORI_LORE		145
#define SKILL_DWOMAX_LANGUAGE		146
#define SKILL_DWOMAX_WLANGUAGE		147
#define SKILL_DWOMAX_LORE		148
#define SKILL_MURRASH_LANGUAGE		149
#define SKILL_MURRASH_WLANGUAGE		150
#define SKILL_MURRASH_LORE		151
#define SKILL_SHASTICK_LANGUAGE		152
#define SKILL_SHASTICK_WLANGUAGE	153
#define SKILL_SHASTICK_LORE		154
#define SKILL_THADYRI_LANGUAGE		155
#define SKILL_THADYRI_WLANGUAGE		156
#define SKILL_THADYRI_LORE		157
#define SKILL_TERRAN_LANGUAGE		158
#define SKILL_TERRAN_WLANGUAGE		159
#define SKILL_TERRAN_LORE		160
#define SKILL_TRULOR_LANGUAGE		161
#define SKILL_TRULOR_WLANGUAGE		162
#define SKILL_TRULOR_LORE		163
#define SKILL_SIGN_LANGUAGE		164
#define SKILL_FIRST_AID			165
#define SKILL_BOATING			166
#define SKILL_CLIMBING			167
#define SKILL_TRACKING			178
#define SKILL_COMPUTER_OPS		179
#define SKILL_ELECTRONICS_OPS		180
#define SKILL_FASTTALK			183
#define SKILL_LEADERSHIP		185
#define SKILL_MERCHANT			186
#define SKILL_SEX_APPEAL		188
#define SKILL_TEACHING			189
#define SKILL_DEMOLITION		190
#define SKILL_DISGUISE			192
#define SKILL_ESCAPE			193
#define SKILL_FORGERY			194
#define SKILL_LIP_READING		196
#define SKILL_STREETWISE		197
#define SKILL_MIX_POISONS		198
#define SKILL_SHADOWING			199
#define SKILL_TRAPS				200
#define SKILL_ALCHEMY			201
#define SKILL_TERRAN_POWEREDARMOR	202
#define SKILL_TRULOR_POWEREDARMOR	203
#define SKILL_TERRAN_PILOTING		204
#define SKILL_TRULOR_PILOTING		205

#define FIRST_COMBATSKILL2		206
#define SKILL_BLADESONG			206
#define SKILL_SHIELD_BASH		207
#define SKILL_BITE			208
#define SKILL_UPPERCUT			209
#define SKILL_KARATE_CHOP		210
#define SKILL_KARATE_KICK		211
#define SKILL_FEINT			212
#define SKILL_RIPOSTE			213
#define SKILL_DUAL_WIELD		214
#define SKILL_COMBAT_TECHNIQUE		215
#define LAST_COMBATSKILL2		216

#define FIRST_WEAPON_PROFICIENCY	251
#define PROFICIENCY_BOW			251
#define PROFICIENCY_CROSSBOW		252
#define PROFICIENCY_SLING		253
#define PROFICIENCY_THROW		254
#define PROFICIENCY_MACE		255
#define PROFICIENCY_AXE			256
#define PROFICIENCY_AXETHROW		257
#define PROFICIENCY_BROADSWORD		258
#define PROFICIENCY_FENCING		259
#define PROFICIENCY_FLAIL		260
#define PROFICIENCY_KNIFE		261
#define PROFICIENCY_KNIFETHROW		262
#define PROFICIENCY_LANCE		263
#define PROFICIENCY_POLEARM		264
#define PROFICIENCY_SHORTSWORD		265
#define PROFICIENCY_SPEAR		266
#define PROFICIENCY_SPEARTHROW		267
#define PROFICIENCY_STAFF		268
#define PROFICIENCY_2H_MACE		269
#define PROFICIENCY_2H_AXE		270
#define PROFICIENCY_2H_SWORD		271
#define PROFICIENCY_WHIP		272
#define PROFICIENCY_LASSO		273
#define PROFICIENCY_NET			274
#define PROFICIENCY_BOLA		275
#define PROFICIENCY_GUN			276
#define PROFICIENCY_BEAMGUN		277
#define PROFICIENCY_BEAMSWORD		278
#define LAST_WEAPON_PROFICIENCY		278
#define LAST_SKILL			278

/* New skills may be added here up to MAX_SKILLS (300) */

/*
 *  NON-PLAYER AND OBJECT SPELLS AND SKILLS
 *  The practice levels for the spells and skills below are _not_ recorded
 *  in the playerfile; therefore, the intended use is for spells and skills
 *  associated with objects or non-players (such as NPC-only spells).
 */

#define SPELL_FIRE_BREATH		301
#define SPELL_GAS_BREATH		302
#define SPELL_FROST_BREATH		303
#define SPELL_ACID_BREATH		304
#define SPELL_LIGHTNING_BREATH		305


/* NEW NPC/OBJECT SPELLS can be inserted here up to 399 */

// Beyond here, we have skillnums that can be rolled against for their
// defaults, but not learned per se.


/* WEAPON ATTACK TYPES */

#define FIRST_COMBAT_MESSAGE		400
#define TYPE_HIT			400
#define TYPE_STING			401
#define TYPE_WHIP			402
#define TYPE_SLASH			403
#define TYPE_BITE			404
#define TYPE_BLUDGEON		405
#define TYPE_CRUSH			406
#define TYPE_POUND			407
#define TYPE_CLAW			408
#define TYPE_MAUL			409
#define TYPE_THRASH			410
#define TYPE_PIERCE			411
#define TYPE_BLAST			412
#define TYPE_STAB			413
#define TYPE_GARROTE		414
#define TYPE_ARROW			415
#define TYPE_BOLT			416
#define TYPE_ROCK			417

// Dummy messages for combat profile
#define SKILL_DODGE			450
#define SKILL_PARRY			451

/* new attack types can be added here - up to TYPE_SUFFERING */
#define TYPE_SUFFERING		499
#define LAST_COMBAT_MESSAGE	499

#define FIRST_SPELL				500
#define SPELL_ARMOR				500
#define SPELL_TELEPORT			501
#define SPELL_BLESS				502
#define SPELL_BLINDNESS			503
#define SPELL_BURNING_HANDS		504
#define SPELL_CALL_LIGHTNING	505
#define SPELL_CHARM				506
#define SPELL_CHILL_TOUCH		507
#define SPELL_CLONE				508 // Unfamiliar.  But I don't care yet.
#define SPELL_COLOR_SPRAY		509
#define SPELL_CONTROL_WEATHER	510
#define SPELL_CREATE_FOOD		511
#define SPELL_CREATE_WATER		512
#define SPELL_CURE_BLIND		513 // No effect.
#define SPELL_CURE_CRITIC		514
#define SPELL_CURE_LIGHT		515
#define SPELL_CURSE				516
#define SPELL_DETECT_ALIGN		517
#define SPELL_DETECT_INVIS		518
#define SPELL_DETECT_MAGIC		519
#define SPELL_DETECT_POISON		520
#define SPELL_DISPEL_EVIL		521
#define SPELL_EARTHQUAKE		522
#define SPELL_ENCHANT_WEAPON	523
#define SPELL_ENERGY_DRAIN		524
#define SPELL_FIREBALL			525
#define SPELL_HARM				526
#define SPELL_HEAL				527
#define SPELL_INVISIBLE			528
#define SPELL_LIGHTNING_BOLT	529
#define SPELL_LOCATE_OBJECT		530
#define SPELL_MAGIC_MISSILE		531
#define SPELL_POISON			532
#define SPELL_PROT_FROM_EVIL	533
#define SPELL_REMOVE_CURSE		534
#define SPELL_MAGE_SHIELD		535
#define SPELL_SHOCKING_GRASP	536
#define SPELL_SLEEP				537
#define SPELL_STRENGTH			538
#define SPELL_SUMMON			539
#define SPELL_VENTRILOQUATE		540 // Doesn't work.
#define SPELL_WORD_OF_RECALL	541
#define SPELL_REMOVE_POISON		542 // Didn't work.
#define SPELL_SENSE_LIFE		543
#define SPELL_ANIMATE_DEAD		544
#define SPELL_DISPEL_GOOD		545
#define SPELL_GROUP_ARMOR		546
#define SPELL_GROUP_HEAL		547
#define SPELL_GROUP_RECALL		548
#define SPELL_INFRAVISION		549
#define SPELL_FLY				550
#define SPELL_IDENTIFY			551
#define SPELL_EARTHSPIKE		552
#define SPELL_SILENCE			553
#define SPELL_LEVITATE			554
#define SPELL_REFRESH			555
#define SPELL_FAERIE_FIRE		556
#define SPELL_STONESKIN			557
#define SPELL_HASTE				558
#define SPELL_BLADE_BARRIER		559
#define SPELL_FIRE_SHIELD		560
#define SPELL_CREATE_LIGHT		561
#define SPELL_CAUSE_LIGHT		562
#define SPELL_CAUSE_CRITIC		563
#define SPELL_HOLY_WORD			564
#define SPELL_ICE_STORM			565
#define SPELL_INFERNO			566
#define SPELL_RECHARGE			567
#define SPELL_CREATE_FEAST		568
#define SPELL_PHANTOM_FIST		569
#define SPELL_HOLY_ARMOR		570
#define SPELL_ACID_RAIN			571
#define SPELL_PSYCHIC_ATTACK		572
#define SPELL_HORRIFIC_ILLUSION		573
#define SPELL_REGENERATE		574
#define SPELL_FLAME_DAGGER		575
#define SPELL_POULTICE			576
#define SPELL_GRENADE			577
#define SPELL_POISON_GAS		578
#define SPELL_FREEZE_FOE		579
#define SPELL_WALL_OF_FOG		580
#define SPELL_RELOCATE			581 // Doesn't work.  Can't find target.
#define SPELL_PEACE				582 // Code change
#define SPELL_PLANESHIFT		583
#define SPELL_GROUP_PLANESHIFT		584 // Code change
#define SPELL_VAMP_REGEN		585
#define SPELL_SANCTUARY			586
#define SPELL_PORTAL			587
#define SPELL_BIND				588
#define SPELL_RAISE_DEAD		589
#define SPELL_REINCARNATE		590
#define SPELL_CONVERSE_WITH_EARTH	591
#define SPELL_ENTANGLE			592
#define SPELL_EARTHSEIZE		593
#define SPELL_CALTROPS			594
#define SPELL_SCORCHED_EARTH		595
#define SPELL_FLAME_AFFINITY		596
#define SPELL_FLAMMABILITY		597
#define SPELL_TORNADO			598
#define SPELL_TREMOR			599
#define SPELL_ICE_WALL			600
#define SPELL_CONTROLLED_ILLUSION	601
#define SPELL_FIRE_ELEMENTAL		602
#define SPELL_EARTH_ELEMENTAL		603
#define SPELL_AIR_ELEMENTAL			604
#define SPELL_WATER_ELEMENTAL		605




// HARD REFERENCES TO STATS - used for GURPS-style skills -DH
#define FIRST_STAT			650
#define STAT_STR			650
#define STAT_INT			651
#define STAT_WIS			652
#define STAT_DEX			653
#define STAT_CON			654
#define STAT_CHA			655
#define SKILL_WEAPON			656
#define STAT_VISION			657
#define STAT_HEARING			658
#define STAT_MOVEMENT			659
#define SKILL_WEAPON_R			660	// These are for combat moves
#define SKILL_WEAPON_L			661	// which specify which hand
#define SKILL_WEAPON_3			662	// is used.
#define SKILL_WEAPON_4			663
#define LAST_STAT			663

#ifndef TOP_SKILLO_DEFINE
#define TOP_SKILLO_DEFINE		700
#define NUM_SKILLS				TOP_SKILLO_DEFINE
#endif

#define SAVING_PARA	0
#define SAVING_ROD	 1
#define SAVING_PETRI  2
#define SAVING_BREATH 3
#define SAVING_SPELL  4


#define TAR_IGNORE	  (1 << 0)
#define TAR_CHAR_ROOM  (1 << 1)
#define TAR_CHAR_WORLD (1 << 2)
#define TAR_FIGHT_SELF (1 << 3)
#define TAR_FIGHT_VICT (1 << 4)
#define TAR_SELF_ONLY  (1 << 5) /* Only a check, use with i.e. TAR_CHAR_ROOM */
#define TAR_NOT_SELF	(1 << 6) /* Only a check, use with i.e. TAR_CHAR_ROOM */
#define TAR_OBJ_INV	 (1 << 7)
#define TAR_OBJ_ROOM	(1 << 8)
#define TAR_OBJ_WORLD  (1 << 9)
#define TAR_OBJ_EQUIP  (1 << 10)
#define TAR_OBJ_DEST	(1 << 11)
#define TAR_CHAR_OBJ	(1 << 12)
#define TAR_IG_CHAR	 (1 << 13)
#define TAR_IG_OBJ	  (1 << 14)
#define TAR_MELEE		(1 << 15)

#define SKLBASE_STR	 (1 << 0)
#define SKLBASE_INT	 (1 << 1)
#define SKLBASE_WIS	 (1 << 2)
#define SKLBASE_DEX	 (1 << 3)
#define SKLBASE_CON	 (1 << 4)
#define SKLBASE_CHA	 (1 << 5)
#define SKLBASE_VISION (1 << 6)
#define SKLBASE_HEARING (1 << 7)
#define SKLBASE_MOVE	(1 << 8)


#define SKILL_CRIT_SUCCESS		100
#define SKILL_CRIT_FAILURE		-100
#define SKILL_NOCHANCE			 -120


enum Result {
	Ignored = -1,
	Blunder,
	AFailure,
	Failure,
	PSuccess,
	NSuccess,
	Success,
	ASuccess
};


#define TAR_IGNORE	  (1 << 0)
#define TAR_CHAR_ROOM  (1 << 1)
#define TAR_CHAR_WORLD (1 << 2)
#define TAR_FIGHT_SELF (1 << 3)
#define TAR_FIGHT_VICT (1 << 4)
#define TAR_SELF_ONLY  (1 << 5) /* Only a check, use with i.e. TAR_CHAR_ROOM */
#define TAR_NOT_SELF	(1 << 6) /* Only a check, use with i.e. TAR_CHAR_ROOM */
#define TAR_OBJ_INV	 (1 << 7)
#define TAR_OBJ_ROOM	(1 << 8)
#define TAR_OBJ_WORLD  (1 << 9)
#define TAR_OBJ_EQUIP  (1 << 10)
#define TAR_OBJ_DEST	(1 << 11)
#define TAR_CHAR_OBJ	(1 << 12)
#define TAR_IG_CHAR	 (1 << 13)
#define TAR_IG_OBJ	  (1 << 14)
#define TAR_MELEE		(1 << 15)

#endif


