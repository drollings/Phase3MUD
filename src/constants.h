#ifndef __CONSTANTS_H__
#define __CONSTANTS_H__

#include "character.defs.h"
#include "object.defs.h"
#include "room.defs.h"

//	Misc
extern const char *weekdays[];
extern const char *month_name[];
extern const char *connected_types[];
extern const char circlemud_version[];
extern const char *color_liquid[];
extern const char *fullness[];
extern const char *bool_conditions[];
extern const char *position_block[];

//	Characters
extern const struct attack_hit_type attack_hit_text[];
extern const char *genders[];
extern const char *affected_bits[];
extern const char *apply_types[];
extern const char *action_bits[];
extern const char *player_bits[];
extern const char *preference_bits[];
extern const char *position_types[];
extern const char *connected_types[];
extern const char *staff_bits[];
extern const char *race_abbrevs[];
extern const char *affects[];
extern const char relation_colors[];
extern const char *positions[];
extern const char *race_types[];
extern const char *class_types[];
extern const char *class_abbrevs[];
extern const char *mortality_types[];

extern const char *relations[];
extern const char *advantages[];
extern const struct advantage_dat advantage_data[];
extern const char *disadvantages[];
extern const struct advantage_dat disadvantage_data[];
extern const char *named_advantages[];
extern const char *named_disadvantages[];
extern const char *trig_attach_types[];
extern const char *encumbrance_shift[][2];
extern const SInt32 max_stat_table[NUM_RACES][6];
extern const SInt16 stat_cost_table[];
extern const SInt32 mod_stat_table[NUM_RACES][6];
extern const char *bodytypes[MAX_BODYTYPE];
extern const Bodytype race_bodytype[NUM_RACES];
extern const char *already_wearing[MAX_ALREADY_WEARING];
extern const struct body_wear_slots wear_slots[MAX_BODYTYPE][NUM_WEARS];
extern const char *mobile_types[];
extern const char *group_positions[];
extern const char *lawfulness[];
extern const char *alignment[];
extern const char *datatype_strings[];
//	Menus and messages
extern const char *ANSI;
extern const char *MENU;
extern const char *WELC_MESSG;
extern const char *START_MESSG;
extern const char *GREETINGS;
extern const char *PLAINMENU;
extern const char *PLAINGREETINGS;


extern const char *race_menu[NUM_RACES];

//	Objects
extern const char *item_types[];
extern const char *wear_bits[];
extern const char *extra_bits[];
extern const char *container_bits[];
extern const char *drinks[];
extern const char *equipment_types[];
extern const char * ammo_types[];
extern const char *where[];

extern const char *material_types[];
extern const char *trust_names[];
extern const struct wear_on_message wear_slot_messages[MAX_HANDWEAR_MSG];
extern const char *damage_types[];
extern const struct oedit_objval_tab obj_values_tab[NUM_ITEM_TYPES + 1][NUM_OBJ_VALUES + 1];

//	Rooms
extern const char *room_bits[];
extern const char *exit_bits[];
extern const char *sector_types[];

//	Directions
extern const int rev_dir[];
extern const int movement_loss[];
extern char *dir_text[];
extern const char *dirs[];
extern const char *from_dir[];
extern const char *dir_text_to[];
extern const char *dir_text_to_of[];

extern const char * distance_upper[MAX_DISTANCE + 1];
extern const char * distance_lower[MAX_DISTANCE + 1];

//	Scripts
extern const char *mtrig_types[];
extern const char *otrig_types[];
extern const char *wtrig_types[];

//	Shops
extern const char *trade_letters[];
extern const char *shop_bits[];


extern const char *datatype_namestrings[];
extern const char *encumbrance_shift[][2];

extern const char *named_stats[];
extern const char *named_bases[];
extern const char *spell_routines[];

extern const char *colornames[];
extern const char *colorcodes[];
extern const char *gods[];

#endif

