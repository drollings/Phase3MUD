/* ************************************************************************
*   File: db.c                                          Part of CircleMUD *
*  Usage: Loading/saving chars, booting/resetting world, internal funcs   *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */


#include "structs.h"
#include "utils.h"
#include "buffer.h"
#include "scripts.h"
#include "db.h"
#include "rooms.h"
#include "characters.h"
#include "objects.h"
#include "comm.h"
#include "handler.h"
#include "find.h"
#include "mail.h"
#include "interpreter.h"
#include "house.h"
#include "boards.h"
#include "files.h"
#include "clans.h"
#include "ban.h"
#include "help.h"
#include "constants.h"
#include "combat.h"
#include "shop.h"
#include "worldparser.h"
#include "socials.h"


void CheckRegenRates(Character *ch);
extern void roll_height_weight(Character *ch);
void boot_methods(void);

/**************************************************************************
*  declarations of most of the 'global' variables                         *
************************************************************************ */

struct zone_data *zone_table;	/* zone table			 */
SInt32 top_of_zone_table = 0;	/* top element of zone tab	 */

IDNum top_idnum = 0;		/* highest idnum in use		 */

extern bool no_mail;		//	mail disabled?
bool mini_mud = false;		//	mini-mud mode?
time_t boot_time = 0;		//	time of mud boot
SInt32 circle_restrict = 0;		//	level of game restriction

char *credits = NULL;		//	game credits
char *news = NULL;			//	mud news
char *motd = NULL;			//	message of the day - mortals
char *imotd = NULL;			//	message of the day - immorts
char *help = NULL;			//	help screen
char *info = NULL;			//	info page
char *wizlist = NULL;		//	list of higher gods
char *immlist = NULL;		//	list of peon gods
char *background = NULL;	//	background story
char *handbook = NULL;		//	handbook for new immortals
char *policies = NULL;		//	policies page

struct TimeInfoData time_info;/* the infomation about the time    */
struct reset_q_type reset_q;	/* queue of zones to be reset	 */

IDNum max_id = 200000;		/* next unique id number to use  */

/* local functions */
void reboot_wizlists(void);
void boot_world(void);
int count_hash_records(FILE * fl);
void get_one_line(FILE *fl, char *buf);
ACMD(do_reboot);
void index_boot(int mode);
void load_zones(FILE * fl, char *zonename);
void assign_mobiles(void);
void assign_objects(void);
void assign_rooms(void);
void assign_the_shopkeepers(void);
void build_player_index(void);
int is_empty(int zone_nr);
void reset_zone(int zone);
int file_to_string(const char *name, char *buf);
int file_to_string_alloc(const char *name, char **buf);
void renum_zone_table(void);
void log_zone_error(int zone, int cmd_no, char *message);
void reset_time(void);

/* external functions */
void weather_and_time(int mode);
void create_command_list(void);
void sort_spells(void);
void sort_commands(void);
void boot_the_shops(FILE * shop_f, char *filename, int rec_count);
extern int find_first_step(VNum src, VNum target, Flags edgetype);
void strip_string(char *buffer);
struct TimeInfoData mud_time_passed(time_t t2, time_t t1);
int find_name(const char *name);
void make_maze(int zone);
void load_mtrigger(Character *mob);
void load_otrigger(Object *obj);
void reset_wtrigger(Room *room);

void weather_setup(void);

extern void boot_skills(void);

/* external vars */
extern int no_specials;

#define READ_SIZE 256



/*************************************************************************
*  routines for booting the system                                       *
*********************************************************************** */

/* this is necessary for the autowiz system */
void reboot_wizlists(void) {
	file_to_string_alloc(WIZLIST_FILE, &wizlist);
	file_to_string_alloc(IMMLIST_FILE, &immlist);
}


ACMD(do_reboot) {
	int i;
	char *arg = get_buffer(MAX_INPUT_LENGTH);
	
	one_argument(argument, arg);

	if (!str_cmp(arg, "all") || *arg == '*') {
		file_to_string_alloc(WIZLIST_FILE, &wizlist);
		file_to_string_alloc(IMMLIST_FILE, &immlist);
		file_to_string_alloc(NEWS_FILE, &news);
		file_to_string_alloc(CREDITS_FILE, &credits);
		file_to_string_alloc(MOTD_FILE, &motd);
		file_to_string_alloc(IMOTD_FILE, &imotd);
		file_to_string_alloc(HELP_PAGE_FILE, &help);
		file_to_string_alloc(INFO_FILE, &info);
		file_to_string_alloc(POLICIES_FILE, &policies);
		file_to_string_alloc(HANDBOOK_FILE, &handbook);
		file_to_string_alloc(BACKGROUND_FILE, &background);
	} else if (!str_cmp(arg, "wizlist"))
		file_to_string_alloc(WIZLIST_FILE, &wizlist);
	else if (!str_cmp(arg, "immlist"))
		file_to_string_alloc(IMMLIST_FILE, &immlist);
	else if (!str_cmp(arg, "news"))
		file_to_string_alloc(NEWS_FILE, &news);
	else if (!str_cmp(arg, "credits"))
		file_to_string_alloc(CREDITS_FILE, &credits);
	else if (!str_cmp(arg, "motd"))
		file_to_string_alloc(MOTD_FILE, &motd);
	else if (!str_cmp(arg, "imotd"))
		file_to_string_alloc(IMOTD_FILE, &imotd);
	else if (!str_cmp(arg, "help"))
		file_to_string_alloc(HELP_PAGE_FILE, &help);
	else if (!str_cmp(arg, "info"))
		file_to_string_alloc(INFO_FILE, &info);
	else if (!str_cmp(arg, "policy"))
		file_to_string_alloc(POLICIES_FILE, &policies);
	else if (!str_cmp(arg, "handbook"))
		file_to_string_alloc(HANDBOOK_FILE, &handbook);
	else if (!str_cmp(arg, "background"))
		file_to_string_alloc(BACKGROUND_FILE, &background);
	else if (!str_cmp(arg, "xhelp")) {
		if (Help::Table) {
			for (i = 0; i < Help::Top; ++i)
				Help::Table[i].Free();
			FREE(Help::Table);
		}
		Help::Top = 0;
		index_boot(DB_BOOT_HLP);

		if (Help::WizTable) {
			for (i = 0; i < Help::WizTop; ++i)
				Help::WizTable[i].Free();
			FREE(Help::WizTable);
		}
		Help::WizTop = 0;
		index_boot(DB_BOOT_WIZHLP);
	} else if (!str_cmp(arg, "skills")) {
		boot_skills();
	} else if (!str_cmp(arg, "methods")) {
		boot_methods();
	} else {
		ch->Send("Unknown reload option.\r\n");
		release_buffer(arg);
		return;
	}

	ch->Send(OK);
	release_buffer(arg);
}


void boot_world(void) {
	log("Loading zone table.");
	index_boot(DB_BOOT_ZON);

//	sleep(30);

	log("Loading trigs and generating index.");
	index_boot(DB_BOOT_TRG);
	
	log("Loading rooms.");
	index_boot(DB_BOOT_WLD);

//	log("Renumbering rooms.");
//	renum_world();

	log("Checking start rooms.");
	check_start_rooms();

	log("Loading mobs and generating index.");
	index_boot(DB_BOOT_MOB);

	log("Loading objs and generating index.");
	index_boot(DB_BOOT_OBJ);

	log("Loading clans and generating index.");
/*  index_boot(DB_BOOT_CLAN);*/
	Clan::Load();

	log("Renumbering zone table.");
	renum_zone_table();

	log("Loading boards.");
	Board::InitBoards();

	if (!no_specials) {
		log("Loading shops.");
		index_boot(DB_BOOT_SHP);
	}
}


void boot_skills(void)
{
	char 	*filename = get_buffer(256);
	char 	*block = get_buffer(MAX_STRING_LENGTH * 4);
	FILE 	*db_file;
	int 	i;

	Skills.Clear();
	Skills.SetSize(TOP_SKILLO_DEFINE + 1);
	Skills[0] = new Skill;
	Skills[0]->name = strdup("!UNUSED!");
	Skills[0]->wearoff = strdup("!UNUSED WEAROFF!");
	Skills[0]->wearoffroom = strdup("!UNUSED WEAROFFROOM!");

	for (i = 1; i < Skills.Count(); ++i) {
		Skills[i] = Skills[0];
	}

	strcpy(filename, SKILL_FILE);

	Parser::file = filename;

	if (!(db_file = fopen(filename, "r"))) {
		perror(filename);
		tar_restore_file(filename);
		exit(1);
	}

	while (1) {
		VNum	vnum = fread_block(block, db_file, "Error reading file...", filename);
		
		if (vnum == -1)	break;
		
		Skills[vnum] = new Skill;
		Skills[vnum]->number = Parser::number = vnum;
		Skills[vnum]->Parser(block);
		
		Parser::error = false;
	}
	
	fclose(db_file);
	release_buffer(filename);
	release_buffer(block);
}  


void boot_methods(void)
{
	char 	*filename = get_buffer(256);
	char 	*block = get_buffer(MAX_STRING_LENGTH * 4);
	FILE 	*db_file;
    Method  *method = NULL;
    int 	i = 0;

	strcpy(filename, METHOD_FILE);

	Parser::file = filename;

	if (!(db_file = fopen(filename, "r"))) {
		perror(filename);
		tar_restore_file(filename);
		exit(1);
	}

	Method::Methods.Clear();

	while (1) {
		VNum	vnum = fread_block(block, db_file, "Error reading file...", filename);
		
		if (vnum == -1)	break;
        
		method = new Method;

    	// The methods file was searched as a list beginning with one.
        // We're using a list that begins with element 0.  The setting of
        // the number separately is necessary to correct for this.
        method->number = i;

		Parser::number = ++i;
		method->Parser(block);
        
		Method::Methods.Add(*method);
		
		Parser::error = false;
	}
	
	fclose(db_file);
	release_buffer(filename);
	release_buffer(block);
}  


/* body of the booting system */
void boot_db(void) {
	int i;

	log("Boot db -- BEGIN.");

	log("Resetting the game time:");
	reset_time();

	log("Reading news, credits, help, bground, info & motds.");
	file_to_string_alloc(NEWS_FILE, &news);
	file_to_string_alloc(CREDITS_FILE, &credits);
	file_to_string_alloc(MOTD_FILE, &motd);
	file_to_string_alloc(IMOTD_FILE, &imotd);
	file_to_string_alloc(HELP_PAGE_FILE, &help);
	file_to_string_alloc(INFO_FILE, &info);
	file_to_string_alloc(WIZLIST_FILE, &wizlist);
	file_to_string_alloc(IMMLIST_FILE, &immlist);
	file_to_string_alloc(POLICIES_FILE, &policies);
	file_to_string_alloc(HANDBOOK_FILE, &handbook);
	file_to_string_alloc(BACKGROUND_FILE, &background);

	log("Generating player index.");
	build_player_index();
	
	log("Loading social messages.");
	Social::Boot();
	create_command_list(); /* aedit patch -- M. Scott */
  
	log("Loading skills.");
	boot_skills();

	log("Loading methods.");
	boot_methods();

	boot_world();

	log("Loading help entries.");
	index_boot(DB_BOOT_HLP);
	index_boot(DB_BOOT_WIZHLP);

	log("Loading fight messages.");
	CombatMessages::Load(MESS_FILE);
	
	log("Loading damage tables.");
	Combat::Load();

	log("Assigning function pointers:");

	if (!no_specials) {
		log("   Mobiles.");
		assign_mobiles();
		log("   Shopkeepers.");
		assign_the_shopkeepers();
		log("   Objects.");
		assign_objects();
		log("   Rooms.");
		assign_rooms();
	}

	log("Sorting command list and spells.");
	sort_commands();
	sort_spells();

	log("Booting mail system.");
	if (!scan_file()) {
		log("    Mail boot failed -- Mail system disabled");
		no_mail = true;
	}
	
	log("Reading banned site and invalid-name list.");
	Ban::Load();
	Ban::ReadInvalids();
	
	if (!mini_mud) {
		log("Booting houses.");
		House::Boot();
	}


	log("Setting up weather");
	weather_setup();
	
	for (i = 0; i <= top_of_zone_table; i++) {
		log("Resetting %s (rooms %d-%d).", zone_table[i].name, (i ? (zone_table[i - 1].top + 1) : 0),
				zone_table[i].top);
		reset_zone(i);
	}

	reset_q.head = reset_q.tail = NULL;

	boot_time = time(0);

	log("Boot db -- DONE.");
}


/* reset the time in the game from file */
void reset_time(void) {
	SInt32 beginning_of_time = 650336715;

	struct TimeInfoData mud_time_passed(time_t t2, time_t t1);

	time_info = mud_time_passed(time(0), beginning_of_time);

	time_info.year += 2000;

	if (time_info.hours <= 4)			sunlight = SUN_DARK;
	else if (time_info.hours == 5)		sunlight = SUN_RISE;
	else if (time_info.hours <= 20)		sunlight = SUN_LIGHT;
	else if (time_info.hours == 21)		sunlight = SUN_SET;
	else								sunlight = SUN_DARK;

	log("   Current Gametime: %dH %dD %dM %dY.", time_info.hours, time_info.day, time_info.month, time_info.year);
/*
	weather_info.pressure = 960;
	if ((time_info.month >= 7) && (time_info.month <= 12))
		weather_info.pressure += dice(1, 50);
	else
		weather_info.pressure += dice(1, 80);

	weather_info.change = 0;

	if (weather_info.pressure <= 980)			weather_info.sky = SKY_LIGHTNING;
	else if (weather_info.pressure <= 1000)		weather_info.sky = SKY_RAINING;
	else if (weather_info.pressure <= 1020)		weather_info.sky = SKY_CLOUDY;
	else										weather_info.sky = SKY_CLOUDLESS;
*/
}


/* new version to build the player index for the ascii pfiles */
/* generate index table for the player file */
void build_player_index(void) {
	FILE *plr_index;
	char	*index_name = get_buffer(40);
	char	*line, *arg1, *arg2;

	sprintf(index_name, "%s", PLR_INDEX_FILE);
	if(!(plr_index = fopen(index_name, "r"))) {
		if (errno != ENOENT) {
			perror("SYSERR: Error opening player index file pfiles/plr_index");
			system("touch ../.killscript");
			exit(1);
		} else {
			log("No player index file. Creating a new one.");
			touch(PLR_INDEX_FILE);
			if (!(plr_index = fopen(index_name, "r"))) {
				perror("SYSERR: Error opening player index file pfiles/plr_index");
				system("touch ../.killscript");
				exit(1);
			}
		}
	}
	
	release_buffer(index_name);
	line = get_buffer(256);
	
	arg1 = get_buffer(40);
	arg2 = get_buffer(80);
	PlayerIndex		temp;
	
	while (get_line(plr_index, line)) {
		two_arguments(line, arg1, arg2);
//		for (lowerCounter = 0; arg2[lowerCounter]; lowerCounter++)
//			arg2[lowerCounter] = LOWER(arg2[lowerCounter]);
//		CREATE(player_table[i].name, char, strlen(arg2) + 1);
//		strcpy(player_table[i].name, arg2);
		temp.name = str_dup(arg2);
		temp.id = atoi(arg1);
		
		player_table.Append(temp);
		top_idnum = MAX((SInt32) top_idnum, (SInt32) temp.id);
	}
  
	log("   %d players in database.", player_table.Count());
	release_buffer(arg1);
	release_buffer(arg2);
	release_buffer(line);
	fclose(plr_index);
}


 /* function to count how many hash-mark delimited records exist in a file */
int count_hash_records(FILE * fl) {
	char *buf = get_buffer(128);
	int count = 0;

	while (fgets(buf, 128, fl))
		if (*buf == '#')
			count++;

	release_buffer(buf);
	return count;
}


void index_boot(int mode) {
	const char *index_filename, *prefix;
	FILE *index, *db_file;
	int rec_count = 0;
	char *buf1;
	char *filename;

/*
 * XXX: Just in case, although probably not necessary.
 */
//	release_all_buffers();

	filename = get_buffer(256);
	buf1 = get_buffer(256);
	
	switch (mode) {
		case DB_BOOT_WLD:	prefix = WLD_PREFIX;	break;
		case DB_BOOT_MOB:	prefix = MOB_PREFIX;	break;
		case DB_BOOT_OBJ:	prefix = OBJ_PREFIX;	break;
		case DB_BOOT_ZON:	prefix = ZON_PREFIX;	break;
		case DB_BOOT_SHP:	prefix = SHP_PREFIX;	break;
		case DB_BOOT_WIZHLP:	prefix = WIZHLP_PREFIX;	break;
		case DB_BOOT_HLP:	prefix = HLP_PREFIX;	break;
		case DB_BOOT_TRG:	prefix = TRG_PREFIX;	break;
/*#ifdef CLANS_PATCH
		case DB_BOOT_CLAN:	prefix = CLAN_PREFIX;	break;
#endif*/
		default:
			log("SYSERR: Unknown subcommand %d to index_boot!", mode);
			exit(1);
			break;
	}

	if (mini_mud)
		index_filename = MINDEX_FILE;
	else
		index_filename = INDEX_FILE;
	
	sprintf(filename, "%s%s", prefix, index_filename);

	if (!(index = fopen(filename, "r"))) {
		sprintf(buf1, "SYSERR: Error opening index file '%s'", filename);
		perror(buf1);
		tar_restore_file(filename);
		exit(1);
	}

/* first, count the number of records in the file so we can malloc */
	fscanf(index, "%s\n", buf1);
	while (*buf1 != '$') {
		sprintf(filename, "%s%s", prefix, buf1);
		
		if (!(db_file = fopen(filename, "r"))) {
			perror(filename);
			log("SYSERR: File %s listed in %s/%s not found", filename, prefix, index_filename);
			tar_restore_file(filename);
			fclose(index);
			exit(1);
		} else {
			if (mode == DB_BOOT_ZON)
				rec_count++;
			else
				rec_count += count_hash_records(db_file);
		}

		fclose(db_file);
		fscanf(index, "%s\n", buf1);
	}

	/* Exit if 0 records, unless this is shops */
	if (!rec_count) {
		release_buffer(buf1);
		release_buffer(filename);
		if (mode == DB_BOOT_SHP) {
			fclose(index);
			return;
		}
		log("SYSERR: boot error - 0 records counted in %s/%s", prefix, index_filename);
		exit(1);
	}

	rec_count++;

	switch (mode) {
		case DB_BOOT_WLD:
//			CREATE(world, Room, rec_count);
//			world = new Room[rec_count];
			log("   %d rooms, %d bytes.", rec_count, sizeof(Room) * rec_count);
			break;
		case DB_BOOT_MOB:
//			CREATE(Character::Index, IndexData, rec_count);
			log("   %d mobs, %d bytes.", rec_count, (sizeof(IndexData<Character>) + sizeof(Character)) * rec_count);
			break;
		case DB_BOOT_OBJ:
//			CREATE(obj_index, IndexData, rec_count);
			log("   %d objs, %d bytes.", rec_count, (sizeof(IndexData<Object>) + sizeof(Object)) * rec_count);
			break;
		case DB_BOOT_ZON:
			CREATE(zone_table, struct zone_data, rec_count);
			log("   %d zones, %d bytes.", rec_count, sizeof(struct zone_data) * rec_count);
			break;
		case DB_BOOT_HLP:
			CREATE(Help::Table, Help, rec_count);
			log("   %d entries, %d bytes.", rec_count, sizeof(Help) * rec_count);
			break;
		case DB_BOOT_WIZHLP:
			CREATE(Help::WizTable, Help, rec_count);
			log("   %d entries, %d bytes.", rec_count, sizeof(Help) * rec_count);
			break;
/*#ifdef CLANS_PATCH
		case DB_BOOT_CLAN:
			CREATE(Clans, struct clan_info, rec_count + 10);
			clan_top = rec_count - 1;
			break;
#endif*/
		case DB_BOOT_TRG:
//			CREATE(trig_index, IndexData, rec_count);
			log("   %d triggers, %d bytes.", rec_count, (sizeof(IndexData<Trigger>) + sizeof(Trigger)) * rec_count);
			break;
	}

	rewind(index);
	fscanf(index, "%s\n", buf1);
	
	char *block = get_buffer(MAX_STRING_LENGTH * 8);
	Character *	ch;
	Object *	obj;
	Trigger *	trig;
	static SInt32 zone = 0;
	
	while (*buf1 != '$') {
		sprintf(filename, "%s%s", prefix, buf1);
		
		if (!(db_file = fopen(filename, "r"))) {
			perror(filename);
			tar_restore_file(filename);
			fclose(index);
			exit(1);
		}
		switch (mode) {
			case DB_BOOT_WLD:
				Parser::file = filename;
				while (1) {
//					char *word = fread_word(db_file);
					
//					if (str_cmp(word, "Room"))	break;
					
					VNum	vnum = fread_block(block, db_file, "Error reading file...", filename);
					
					if (vnum == -1)	break;
					
					world[vnum].Virtual(Parser::number = vnum);
					world[vnum].Parse(block);
					
					Parser::error = false;
				}
				break;

			case DB_BOOT_OBJ:
				Parser::file = filename;
				while (1) {
//					char *word = fread_word(db_file);
					
//					if (str_cmp(word, "Obj"))	break;
					
					VNum	vnum = fread_block(block, db_file, "Error reading file...", filename);
					
					if (vnum == -1)	break;
					
					Object::Index[vnum].vnum = Parser::number = vnum;
					Object::Index[vnum].number = 0;
					Object::Index[vnum].func = NULL;
					Object::Index[vnum].proto = obj = new Object;
					obj->Virtual(vnum);
					obj->Parser(block);
					
					Parser::error = false;
				}
				break;
			case DB_BOOT_MOB:
				Parser::file = filename;
				while (1) {
//					char *word = fread_word(db_file);
					
//					if (str_cmp(word, "Obj"))	break;
					
					VNum	vnum = fread_block(block, db_file, "Error reading file...", filename);
					
					if (vnum == -1)	break;
					
					Character::Index[vnum].vnum = Parser::number = vnum;
					Character::Index[vnum].number = 0;
					Character::Index[vnum].func = NULL;
					Character::Index[vnum].proto = ch = new Character;
					ch->mob = new MobData;
					ch->Virtual(vnum);
					
					ch->real_abils.Str = 10;
					ch->real_abils.Int = 10;
					ch->real_abils.Wis = 10;
					ch->real_abils.Dex = 10;
					ch->real_abils.Con = 10;
					ch->real_abils.Cha = 10;
					ch->weight = 72;
					ch->general.height = 198;
					
					for (SInt32 j = 0; j < 3; j++)
						GET_COND(ch, j) = -1;
					
					ch->general.title = NULL;
					
					ch->Parser(block);
					
					if (!ch->SSKeywords())		ch->SetKeywords("error");
					if (!ch->SSName())			ch->SetName("an erroneous mob");
					if (!ch->SSSDesc())			ch->SetSDesc("A very buggy mob!\r\n");
					if (!ch->SSLDesc())			ch->SetLDesc("ERROR MOB!!!\r\n");
					
					SET_BIT(MOB_FLAGS(ch), MOB_ISNPC);	//	Done AFTER loading, because MOB_FLAGS can be replaced
					
					if (IS_SIMPLE_MOB(ch))		render_simple_mob(ch);

					ch->aff_abils = ch->real_abils;
					
					Parser::error = false;
				}
				break;
/*#ifdef CLANS_PATCH
			case DB_BOOT_CLAN:
#endif*/
			case DB_BOOT_TRG:
				Parser::file = filename;
				while (1) {
//					char *word = fread_word(db_file);
					
//					if (str_cmp(word, "Trig"))	break;
					
					VNum	vnum = fread_block(block, db_file, "Error reading file...", filename);
					
					if (vnum == -1)	break;
					
					Trigger::Index[vnum].vnum = Parser::number = vnum;
					Trigger::Index[vnum].number = 0;
					Trigger::Index[vnum].func = NULL;
					Trigger::Index[vnum].proto = trig = new Trigger;
					trig->Virtual(vnum);
					trig->Parse(block);
					
					Parser::error = false;
				}
				break;
			case DB_BOOT_ZON:
				load_zones(db_file, filename);
				break;
			case DB_BOOT_HLP:
				Help::Load(db_file);
				break;
			case DB_BOOT_WIZHLP:
				Help::WizLoad(db_file);
				break;
			case DB_BOOT_SHP:
				Parser::file = filename;
				while (1) {
//					char *word = fread_word(db_file);
					
//					if (str_cmp(word, "Shop"))	break;
					
					VNum	vnum = fread_block(block, db_file, "Error reading file...", filename);
					
					if (vnum == -1)	break;
					
					Shop & shop = Shop::Index[vnum];
					
					shop.vnum = Parser::number = vnum;
					shop.Parser(block);
					
					Parser::error = false;
				}
				break;
		}

		fclose(db_file);
		fscanf(index, "%s\n", buf1);
	}
	fclose(index);
	
	release_buffer(block);
	release_buffer(buf1);
	release_buffer(filename);
}


#define ZCMD zone_table[zone].cmd[cmd_no]

/* resulve vnums into rnums in the zone reset tables */
void renum_zone_table(void) {
	char *buf = get_buffer(MAX_INPUT_LENGTH);
	int zone, cmd_no, a, b, c, d, olda, oldb, oldc, oldd;

	for (zone = 0; zone <= top_of_zone_table; zone++)
		for (cmd_no = 0; ZCMD.command != 'S'; cmd_no++) {
			a = b = c = d = 0;
			olda = ZCMD.arg1;
			oldb = ZCMD.arg2;
			oldc = ZCMD.arg3;
			oldd = ZCMD.arg4;
			switch (ZCMD.command) {
				case 'M':
					a = ZCMD.arg1 = Character::Find(ZCMD.arg1) ? ZCMD.arg1 : -1;
					c = ZCMD.arg3 = Room::Find(ZCMD.arg3) ? ZCMD.arg3 : -1;
					break;
				case 'O':
					a = ZCMD.arg1 = Object::Find(ZCMD.arg1) ? ZCMD.arg1 : -1;
					if (ZCMD.arg3 != NOWHERE)
						c = ZCMD.arg3 = Room::Find(ZCMD.arg3) ? ZCMD.arg3 : -1;
					break;
				case 'G':
					a = ZCMD.arg1 = Object::Find(ZCMD.arg1) ? ZCMD.arg1 : -1;
					break;
				case 'E':
					a = ZCMD.arg1 = Object::Find(ZCMD.arg1) ? ZCMD.arg1 : -1;
					break;
				case 'P':
					a = ZCMD.arg1 = Object::Find(ZCMD.arg1) ? ZCMD.arg1 : -1;
					c = ZCMD.arg3 = Object::Find(ZCMD.arg3) ? ZCMD.arg3 : -1;
					break;
				case 'D':
					a = ZCMD.arg1 = Room::Find(ZCMD.arg1) ? ZCMD.arg1 : -1;
					break;
				case 'R': /* rem obj from room */
					a = ZCMD.arg1 = Room::Find(ZCMD.arg1) ? ZCMD.arg1 : -1;
					b = ZCMD.arg2 = Object::Find(ZCMD.arg2) ? ZCMD.arg2 : -1;
					break;
				case 'T':
					b = ZCMD.arg2 = Trigger::Find(ZCMD.arg2) ? ZCMD.arg2 : -1;
					switch (ZCMD.arg1) {
						case MOB_TRIGGER:
						case OBJ_TRIGGER:
							break;
						case WLD_TRIGGER:
							ZCMD.arg4 = Room::Find(ZCMD.arg4) ? ZCMD.arg4 : -1;
							break;
						default:
							a = -1;
							break;
					}
			}
			if (a < 0 || b < 0 || c < 0 || d < 0) {
				if (!mini_mud)
					sprintf(buf,  "Invalid vnum %d, cmd disabled",
							(a < 0) ? olda : ((b < 0) ? oldb : oldc));
					log_zone_error(zone, cmd_no, buf);
				ZCMD.command = '*';
			}
		}
	release_buffer(buf);
}


#define Z	zone_table[zone]

/* load the zone table and command tables */
void load_zones(FILE * fl, char *zonename) {
	static int zone = 0;
	int cmd_no = 0, num_of_cmds = 0, line_num = 0, tmp, error, num[7];
	char	*ptr,
			*buf = get_buffer(256),
			*zname = get_buffer(256),
			*buf2 = get_buffer(128),
			*builder;
	int		weather = 0;
	
	strcpy(zname, zonename);

	while (get_line(fl, buf))
		num_of_cmds++;		/* this should be correct within 3 or so */
	rewind(fl);

	if (num_of_cmds == 0) {
		log("SYSERR: %s is empty!", zname);
		exit(0);
	} else
		CREATE(Z.cmd, struct reset_com, num_of_cmds);

	line_num += get_line(fl, buf);

	if (sscanf(buf, "#%d", &Z.number) != 1) {
		log("SYSERR: Format error in %s, line %d", zname, line_num);
		exit(1);
	}
	sprintf(buf2, "beginning of zone #%d", Z.number);

	line_num += get_line(fl, buf);
	if ((ptr = strchr(buf, '~')) != NULL)	/* take off the '~' if it's there */
		*ptr = '\0';
	Z.name = str_dup(buf);

	line_num += get_line(fl, buf);
	if (sscanf(buf, " %d %d %d ", &Z.top, &Z.lifespan, &Z.reset_mode) != 3) {
		log("SYSERR: Format error in 3-constant line of %s", zname);
		exit(1);
	}

	/* get weather info for the zone */
	fscanf(fl, " %hd ", &Z.climate.pattern);
	fscanf(fl, " %d ", &Z.climate.flags);
	fscanf(fl, " %d ", &Z.climate.energy);

	for(int i=0;i<Weather::MaxSeasons;i++)	fscanf(fl," %hd ", &Z.climate.wind[i]);
	for(int i=0;i<Weather::MaxSeasons;i++)	fscanf(fl," %hd ", &Z.climate.windDirection[i]);
	for(int i=0;i<Weather::MaxSeasons;i++)	fscanf(fl," %hd ", &Z.climate.windVariance[i]);
	for(int i=0;i<Weather::MaxSeasons;i++)	fscanf(fl," %hd ", &Z.climate.precipitation[i]);
	for(int i=0;i<Weather::MaxSeasons;i++)	fscanf(fl," %hd ", &Z.climate.temperature[i]);

//	Z.pressure = 960;
//	if ((time_info.month >= 7) && (time_info.month <= 12))	Z.pressure += dice(1, 50);
//	else 													Z.pressure += dice(1, 80);
//	Z.change = 0;
//	if (Z.pressure <= 980)			Z.sky = SKY_LIGHTNING;
//	else if (Z.pressure <= 1000)	Z.sky = SKY_RAINING;
//	else if (Z.pressure <= 1020)	Z.sky = SKY_CLOUDY;
//	else							Z.sky = SKY_CLOUDLESS;

	cmd_no = 0;

	for (;;) {
		if (!(tmp = get_line(fl, buf))) {
			log("SYSERR: Format error in %s - premature end of file", zname);
			exit(1);
		}
		line_num += tmp;
		ptr = buf;
		skip_spaces(ptr);
		
		ZCMD.command = *ptr;
		
		ptr++;
		
		if (ZCMD.command == '*') {
//			int		i[4];
			char * arg1 = get_buffer(MAX_INPUT_LENGTH);
			char * arg2 = get_buffer(MAX_INPUT_LENGTH);
			half_chop(ptr, arg1, arg2);
			if (*arg1 && *arg2) {				
				if (is_abbrev(arg1, "Builder")) {
					builder = str_dup(arg2);
					Z.builders.Append(builder);
				}
/*				 else if (is_abbrev(arg1, "climate")) {
					weather++;
					sscanf(arg2, " %d %d %d ", i + 0, i + 1, i + 2);
					Z.climate.pattern = i[0];
					Z.climate.flags = i[1];
					Z.climate.energy = i[2];
				} else if (is_abbrev(arg1, "wind")) {
					weather++;
					sscanf(arg2, " %d %d %d %d ", i + 0, i + 1, i + 2, i + 3);
					Z.climate.wind[0] = i[0];
					Z.climate.wind[1] = i[1];
					Z.climate.wind[2] = i[2];
					Z.climate.wind[3] = i[3];
				} else if (is_abbrev(arg1, "winddirection")) {
					weather++;
					sscanf(arg2, " %d %d %d %d ", i + 0, i + 1, i + 2, i + 3);
					Z.climate.windDirection[0] = i[0];
					Z.climate.windDirection[1] = i[1];
					Z.climate.windDirection[2] = i[2];
					Z.climate.windDirection[3] = i[3];
				} else if (is_abbrev(arg1, "windvariance")) {
					weather++;
					sscanf(arg2, " %d %d %d %d ", i + 0, i + 1, i + 2, i + 3);
					Z.climate.windVariance[0] = i[0];
					Z.climate.windVariance[1] = i[1];
					Z.climate.windVariance[2] = i[2];
					Z.climate.windVariance[3] = i[3];
				} else if (is_abbrev(arg1, "precipitation")) {
					weather++;
					sscanf(arg2, " %d %d %d %d ", i + 0, i + 1, i + 2, i + 3);
					Z.climate.precipitation[0] = i[0];
					Z.climate.precipitation[1] = i[1];
					Z.climate.precipitation[2] = i[2];
					Z.climate.precipitation[3] = i[3];
				} else if (is_abbrev(arg1, "temperature")) {
					weather++;
					sscanf(arg2, " %d %d %d %d ", i + 0, i + 1, i + 2, i + 3);
					Z.climate.temperature[0] = i[0];
					Z.climate.temperature[1] = i[1];
					Z.climate.temperature[2] = i[2];
					Z.climate.temperature[3] = i[3];
				}
*/			}
			release_buffer(arg1);
			release_buffer(arg2);
			continue;
		}


		if (ZCMD.command == 'S' || ZCMD.command == '$') {
			ZCMD.command = 'S';
			break;
		}
		error = 0;

		sscanf(ptr, " %d %d %d %d %d %d", num, num + 1, num + 2, num + 3, num + 4, num + 5);

		ZCMD.if_flag = num[0] ? 1 : 0;
		ZCMD.arg1 = num[1];
		ZCMD.arg2 = num[2];
		ZCMD.arg3 = num[3];
		ZCMD.arg4 = num[4];
		// CHANGEPOINT:	 Re-enable this when zones are properly converted
		// ZCMD.repeat = (num[5] > 0) ? num[5] : 1;
		ZCMD.repeat = 1;
		
		switch (ZCMD.command) {
			case 'C':
				skip_spaces(ptr);
				if (*ptr) {
//					ZCMD.if_flag = *ptr ? 1 : 0;
					ptr++;
					skip_spaces(ptr);
					ZCMD.command_string = str_dup(ptr);
				}
//				ZCMD.command_string = str_dup(ptr);
				break;
			case 'M':
//				if (sscanf(ptr, " %d %d %d %d %d ", &tmp, &ZCMD.arg1, &ZCMD.arg2, &ZCMD.arg3, &ZCMD.arg4) < 4)
//					error = 1;
//				break;
				ZCMD.repeat = 1;
				break;
			case 'E':
			case 'D':
			case 'Z':
//				if (sscanf(ptr, " %d %d %d %d ", &tmp, &ZCMD.arg1, &ZCMD.arg2, &ZCMD.arg3) != 4)
//					error = 1;
//				break;
				ZCMD.arg4 = -1;
				ZCMD.repeat = 1;
				break;
			case 'O':
			case 'P':
//				if (sscanf(ptr, " %d %d %d %d %d ", &tmp, &ZCMD.arg1, &ZCMD.arg2, &ZCMD.arg3, &ZCMD.repeat) != 5)
//					error = 1;
//				break;
				ZCMD.arg4 = -1;
				break;
			case 'G':
			case 'R':
//				if (sscanf(ptr, " %d %d %d %d ", &tmp, &ZCMD.arg1, &ZCMD.arg2, &ZCMD.repeat) != 4)
//					error = 1;
//				break;
				ZCMD.arg3 = -1;
				ZCMD.arg4 = -1;
				break;
			case 'T':
//				if (sscanf(ptr, " %d %d %d %d %d ", &tmp, &ZCMD.arg1, &ZCMD.arg2, &ZCMD.arg3, &ZCMD.arg4) != 5)
//					error = 1;
//				break;
				ZCMD.repeat = 1;
				break;
			default:
//				if (sscanf(ptr, " %d %d %d ", &tmp, &ZCMD.arg1, &ZCMD.arg2) != 3)
//				error = 1;
//				break;
				ZCMD.arg3 = -1;
				ZCMD.arg4 = -1;
				ZCMD.repeat = 1;
				break;
		}
//		ZCMD.if_flag = tmp;

//		if (error) {
//			log("SYSERR: Format error in %s, line %d: '%s'", zname, line_num, buf);
//			exit(1);
//		}
		ZCMD.line = line_num;
		cmd_no++;
	}
	
	top_of_zone_table = zone++;
	
	release_buffer(buf);
	release_buffer(zname);
	release_buffer(buf2);
}

#undef Z


/* Smandoggi's new weather setup */
void weather_setup()
{
	char s;

	/* default conditions for season values */
	char winds[6] = {2,12,30,40,50,80};
	char precip[9] = {0,0,0,1,3,8,15,23,45};
	char humid[9] = {4,10,20,30,40,50,60,75,100};
	char temps[11] = {-20,-10,0,2,7,17,23,29,37,55,95};

	for(int zon = 0; zon <= top_of_zone_table; zon++) {
		/* get the season */
		s=zone_table[zon].GetSeason();

		/* These are pretty standard start values */
		zone_table[zon].conditions.pressure=980;
		zone_table[zon].conditions.energy=10000;
		zone_table[zon].conditions.precipDepth=0;

		/* These use the default conditions above */
		zone_table[zon].conditions.windspeed=
			winds[zone_table[zon].climate.wind[s]];

		zone_table[zon].conditions.windDirection=
			zone_table[zon].climate.windDirection[s];

		zone_table[zon].conditions.precipRate=
			precip[zone_table[zon].climate.precipitation[s]];

		zone_table[zon].conditions.temperature=
			temps[zone_table[zon].climate.temperature[s]];

		zone_table[zon].conditions.humidity=
			humid[zone_table[zon].climate.precipitation[s]];
	
		/* Set ambient light */
		zone_table[zon].CalcLight();
	}
}



void get_one_line(FILE *fl, char *buf) {
	if (!fgets(buf, READ_SIZE, fl)) {
		log("error reading help file: not terminated with $?");
		exit(1);
	}

	buf[strlen(buf) - 1] = '\0'; /* take off the trailing \n */
}


/*************************************************************************
*  procedures for resetting, both play-time and boot-time	 	 *
*********************************************************************** */



VNum vnum_mobile(char *searchname, Character * ch) {
	SInt32	found = 0;
	VNum 	bottom = -1, top = -1, place = -1;
	Index<Character>::Iterator	iter(Character::Index);
	IndexData<Character> *c;
	char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

	*buf = '\0';

	if (isdigit(*searchname)) {
		half_chop(searchname, buf1, buf2);

		bottom = atoi(buf1);
		top = bottom;

		if (*buf2) {
			// We have a second argument.
			if (isdigit(*buf2)) {
				// Okay, now we know it's vnum obj <number> <number>, so display a range
				top = MAX(0, atoi(buf2));

				if ((bottom + 300) <= top) {
					ch->Send("May not exceed 300 mobs, try using a smaller end number!\r\n");
					return (0);
				}

				for (place = bottom; place <= top; ++place) {
					if (Character::Find(place)) {
						sprintf(buf + strlen(buf), "%3d. [%5d] %s\r\n", ++found,
								place, Character::Index[place].proto->Name());
					}
				}
			} else {
				for (place = 0; place <= top_of_zone_table; ++place)
					if (zone_table[place].number == bottom)
						break;
						
				if (place > top_of_zone_table) {
					ch->Send("That zone doesn't exist!  Try vnum mob <zone number> <keywords>.\r\n");
					return (0);
				} else {
					bottom = zone_table[place].number * 100;
					top = zone_table[place].top;
				}
				
				// It's vnum obj <number> <name>, so search a zone for a keyword.
				// Search a given zone for objects with keyword.
				while ((c = iter.Next()))
					if ((c->vnum >= bottom) && (c->vnum <= top) &&
						isname(buf2, c->proto->Keywords()))
						sprintf(buf + strlen(buf), "%3d. [%5d] %s\r\n", ++found, c->vnum, c->proto->Name());
			}
			
		} else {
			// No second argument, we're just going to list a zone.
			
			for (place = 0; place <= top_of_zone_table; ++place)
				if (zone_table[place].number == bottom)
					break;
					
			if (place > top_of_zone_table) {
				ch->Send("That zone doesn't exist!\r\n");
				return (0);
			} else {
				bottom = zone_table[place].number * 100;
				top = zone_table[place].top;
			}

			while ((c = iter.Next()))
				if ((c->vnum >= bottom) && (c->vnum <= top))
					sprintf(buf + strlen(buf), "%3d. [%5d] %s\r\n", ++found, c->vnum, c->proto->Name());
		}
		
	} else {
		while ((c = iter.Next()))
			if (isname(searchname, c->proto->Keywords()))
				sprintf(buf + strlen(buf), "%3d. [%5d] %s\r\n", ++found, c->vnum, c->proto->Name());
	}

	if (found)		page_string(ch->desc, buf, true);
	
	return (found);
}


VNum vnum_object(char *searchname, Character * ch) {
	SInt32	found = 0;
	VNum 	bottom = -1, top = -1, place = -1;
	Index<Object>::Iterator	iter(Object::Index);
	IndexData<Object> *o;
	char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

	*buf = '\0';

	if (isdigit(*searchname)) {
		half_chop(searchname, buf1, buf2);

		bottom = atoi(buf1);
		top = bottom;

		if (*buf2) {
			// We have a second argument.
			if (isdigit(*buf2)) {
				// Okay, now we know it's vnum obj <number> <number>, so display a range
				top = MAX(0, atoi(buf2));

				if ((bottom + 300) <= top) {
					ch->Send("May not exceed 300 items, try using a smaller end number!\r\n");
					return (0);
				}

				for (place = bottom; place <= top; ++place) {
					if (Object::Find(place)) {
						sprintf(buf + strlen(buf), "%3d. [%5d] %s\r\n", ++found,
								place, Object::Index[place].proto->Name());
					}
				}
			} else {
				for (place = 0; place <= top_of_zone_table; ++place)
					if (zone_table[place].number == bottom)
						break;
						
				if (place > top_of_zone_table) {
					ch->Send("That zone doesn't exist!  Try vnum obj <zone number> <keywords>.\r\n");
					return (0);
				} else {
					bottom = zone_table[place].number * 100;
					top = zone_table[place].top;
				}
				
				// It's vnum obj <number> <name>, so search a zone for a keyword.
				// Search a given zone for objects with keyword.
				while ((o = iter.Next()))
					if ((o->vnum >= bottom) && (o->vnum <= top) &&
						isname(buf2, o->proto->Keywords()))
						sprintf(buf + strlen(buf), "%3d. [%5d] %s\r\n", ++found, o->vnum, o->proto->Name());
			}
			
		} else {
			// No second argument, we're just going to list a zone.
			
			for (place = 0; place <= top_of_zone_table; ++place)
				if (zone_table[place].number == bottom)
					break;
					
			if (place > top_of_zone_table) {
				ch->Send("That zone doesn't exist!\r\n");
				return (0);
			} else {
				bottom = zone_table[place].number * 100;
				top = zone_table[place].top;
			}

			while ((o = iter.Next()))
				if ((o->vnum >= bottom) && (o->vnum <= top))
					sprintf(buf + strlen(buf), "%3d. [%5d] %s\r\n", ++found, o->vnum, o->proto->Name());
		}
		
	} else {
		while ((o = iter.Next()))
			if (isname(searchname, o->proto->Keywords("undefined")))
				sprintf(buf + strlen(buf), "%3d. [%5d] %s\r\n", ++found, o->vnum, o->proto->Name());
	}

	if (found)		page_string(ch->desc, buf, true);
	
	return (found);
}


VNum vnum_trigger(char *searchname, Character * ch) {
	SInt32	found = 0;
	VNum 	bottom = -1, top = -1, place = -1;
	Index<Trigger>::Iterator	iter(Trigger::Index);
	IndexData<Trigger> *t;
	char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

	*buf = '\0';

	if (isdigit(*searchname)) {
		half_chop(searchname, buf1, buf2);

		bottom = atoi(buf1);
		top = bottom;

		if (*buf2) {
			// We have a second argument.
			if (isdigit(*buf2)) {
				// Okay, now we know it's vnum obj <number> <number>, so display a range
				top = MAX(0, atoi(buf2));

				if ((bottom + 300) <= top) {
					ch->Send("May not exceed 300 triggers, try using a smaller end number!\r\n");
					return (0);
				}

				for (place = bottom; place <= top; ++place) {
					if (Trigger::Find(place)) {
						sprintf(buf + strlen(buf), "%3d. [%5d] %s\r\n", ++found,
								place, SSData(Trigger::Index[place].proto->name));
					}
				}
			} else {
				for (place = 0; place <= top_of_zone_table; ++place)
					if (zone_table[place].number == bottom)
						break;
						
				if (place > top_of_zone_table) {
					ch->Send("That zone doesn't exist!  Try vnum obj <zone number> <keywords>.\r\n");
					return (0);
				} else {
					bottom = zone_table[place].number * 100;
					top = zone_table[place].top;
				}
				
				// It's vnum obj <number> <name>, so search a zone for a keyword.
				// Search a given zone for objects with keyword.
				while ((t = iter.Next()))
					if ((t->vnum >= bottom) && (t->vnum <= top) &&
						isname(buf2, SSData(t->proto->name)))
						sprintf(buf + strlen(buf), "%3d. [%5d] %s\r\n", ++found, t->vnum, SSData(t->proto->name));
			}
			
		} else {
			// No second argument, we're just going to list a zone.
			
			for (place = 0; place <= top_of_zone_table; ++place)
				if (zone_table[place].number == bottom)
					break;
					
			if (place > top_of_zone_table) {
				ch->Send("That zone doesn't exist!\r\n");
				return (0);
			} else {
				bottom = zone_table[place].number * 100;
				top = zone_table[place].top;
			}

			while ((t = iter.Next()))
				if ((t->vnum >= bottom) && (t->vnum <= top))
					sprintf(buf + strlen(buf), "%3d. [%5d] %s\r\n", ++found, t->vnum, SSData(t->proto->name));
		}
		
	} else {
		while ((t = iter.Next()))
			if (isname(searchname, SSData(t->proto->name)))
				sprintf(buf + strlen(buf), "%3d. [%5d] %s\r\n", ++found, t->vnum, SSData(t->proto->name));
	}

	if (found)		page_string(ch->desc, buf, true);
	
	return (found);
}


#if 0
int vnum_room(char *searchname, Character * ch) 
{
    int nr, found = 0;

    for (nr = 0; nr < world.Count(); ++nr) {
		if (Room::Find(nr) && strstr(searchname, world[nr].Name())) {
		    sprintf(buf, "%3d. [%5d] %s\r\n", ++found,
			    world[nr].Virtual(), world[nr].Name());
		    ch->Send(buf);
		}
    }
    return (found);
}
#else
int vnum_room(char *searchname, Character * ch) {
	SInt32	found = 0;
	VNum 	bottom = -1, top = -1, place = -1;
	Map<VNum, Room>::Iterator	iter(world);
	Room *r;
	char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

	*buf = '\0';

	if (isdigit(*searchname)) {
		half_chop(searchname, buf1, buf2);

		bottom = atoi(buf1);
		top = bottom;

		if (*buf2) {
			// We have a second argument.
			if (isdigit(*buf2)) {
				// Okay, now we know it's vnum obj <number> <number>, so display a range
				top = MAX(0, atoi(buf2));

				if ((bottom + 300) <= top) {
					ch->Send("May not exceed 300 mobs, try using a smaller end number!\r\n");
					return (0);
				}

				for (place = bottom; place <= top; ++place) {
					if (Room::Find(place)) {
						sprintf(buf + strlen(buf), "%3d. [%5d] %s\r\n", ++found,
								place, world[place].Name());
					}
				}
			} else {
				for (place = 0; place <= top_of_zone_table; ++place)
					if (zone_table[place].number == bottom)
						break;
						
				if (place > top_of_zone_table) {
					ch->Send("That zone doesn't exist!  Try vnum mob <zone number> <keywords>.\r\n");
					return (0);
				} else {
					bottom = zone_table[place].number * 100;
					top = zone_table[place].top;
				}
				
				// It's vnum obj <number> <name>, so search a zone for a keyword.
				// Search a given zone for objects with keyword.
				while ((r = iter.Next()))
					if ((r->Virtual() >= bottom) && (r->Virtual() <= top) &&
						isname(buf2, r->Name()))
						sprintf(buf + strlen(buf), "%3d. [%5d] %s\r\n", ++found, r->Virtual(), r->Name());
			}
			
		} else {
			// No second argument, we're just going to list a zone.
			
			for (place = 0; place <= top_of_zone_table; ++place)
				if (zone_table[place].number == bottom)
					break;
					
			if (place > top_of_zone_table) {
				ch->Send("That zone doesn't exist!\r\n");
				return (0);
			} else {
				bottom = zone_table[place].number * 100;
				top = zone_table[place].top;
			}

			while ((r = iter.Next()))
				if ((r->Virtual() >= bottom) && (r->Virtual() <= top))
					sprintf(buf + strlen(buf), "%3d. [%5d] %s\r\n", ++found, r->Virtual(), r->Name());
		}
		
	} else {
		while ((r = iter.Next()))
			if (isname(searchname, r->Name()))
				sprintf(buf + strlen(buf), "%3d. [%5d] %s\r\n", ++found, r->Virtual(), r->Name());
	}

	if (found)		page_string(ch->desc, buf, true);
	
	return (found);
}
#endif


int vnum_shop(char *searchname, Character * ch) {
	SInt32	found = 0;
	VNum 	bottom = -1, top = -1, place = -1;
	Map<VNum, Shop>::Iterator	iter(Shop::Index);
	Shop *r;
	char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

	*buf = '\0';

	if (isdigit(*searchname)) {
		half_chop(searchname, buf1, buf2);

		bottom = atoi(buf1);
		top = bottom;

		if (*buf2) {
			// We have a second argument.
			if (isdigit(*buf2)) {
				// Okay, now we know it's vnum obj <number> <number>, so display a range
				top = MAX(0, atoi(buf2));

				if ((bottom + 300) <= top) {
					ch->Send("May not exceed 300 mobs, try using a smaller end number!\r\n");
					return (0);
				}

				for (place = bottom; place <= top; ++place) {
					if (Shop::Index.Find(place)) {
						sprintf(buf + strlen(buf), "%3d. V[%5d] R[%5d] %s\r\n", ++found, r->Virtual(), 
	                    Shop::Index[place].in_room[0],
	                    Character::Index[Shop::Index[place].keeper].proto->RealName());
					}
				}
			} else {
				ch->Send("You may only specify a zone number.\r\n");
				return (0);
			}
			
		} else {
			// No second argument, we're just going to list a zone.
			
			for (place = 0; place <= top_of_zone_table; ++place)
				if (zone_table[place].number == bottom)
					break;
					
			if (place > top_of_zone_table) {
				ch->Send("That zone doesn't exist!\r\n");
				return (0);
			} else {
				bottom = zone_table[place].number * 100;
				top = zone_table[place].top;
			}

			while ((r = iter.Next()))
				if ((r->Virtual() >= bottom) && (r->Virtual() <= top))
					sprintf(buf + strlen(buf), "%3d. V[%5d] R[%5d] %s\r\n", ++found, r->Virtual(), 
                    r->in_room[0],
                    r->keeper >= 0 ?
					Character::Index[r->keeper].proto->RealName() : 
					"NOBODY!");
		}
		
	} else {
		ch->Send("That zone doesn't exist!\r\n");
		return (0);
	}

	if (found)		page_string(ch->desc, buf, true);
	
	return (found);
}


#define ZO_DEAD  999

/* update zone ages, queue for reset if necessary, and dequeue when possible */
void zone_update(void)
{
	int i;
	struct reset_q_element *update_u, *temp;
	static int timer = 0;
	char *buf = get_buffer(128);

	/* jelson 10/22/92 */
	if (((++timer * PULSE_ZONE) / PASSES_PER_SEC) >= 60) {
		/* one minute has passed */
		/*
		 * NOT accurate unless PULSE_ZONE is a multiple of PASSES_PER_SEC or a
		 * factor of 60
		 */

		timer = 0;

		/* since one minute has passed, increment zone ages */
		for (i = 0; i <= top_of_zone_table; i++) {
			if (zone_table[i].age < zone_table[i].lifespan && zone_table[i].reset_mode)
				(zone_table[i].age)++;

			if (zone_table[i].age >= zone_table[i].lifespan &&
					zone_table[i].age < ZO_DEAD && zone_table[i].reset_mode) {
				/* enqueue zone */

				CREATE(update_u, struct reset_q_element, 1);

				update_u->zone_to_reset = i;
				update_u->next = 0;

				if (!reset_q.head)
					reset_q.head = reset_q.tail = update_u;
				else {
					reset_q.tail->next = update_u;
					reset_q.tail = update_u;
				}

				zone_table[i].age = ZO_DEAD;
			}
		}
	}	/* end - one minute has passed */


	/* dequeue zones (if possible) and reset */
	/* this code is executed every 10 seconds (i.e. PULSE_ZONE) */
	for (update_u = reset_q.head; update_u; update_u = update_u->next)
		if (zone_table[update_u->zone_to_reset].reset_mode == 2 || is_empty(update_u->zone_to_reset)) {
			reset_zone(update_u->zone_to_reset);
			mudlogf(CMP, NULL, FALSE, "Auto zone reset: %s", zone_table[update_u->zone_to_reset].name);
			/* dequeue */
			if (update_u == reset_q.head)
				reset_q.head = reset_q.head->next;
			else {
				for (temp = reset_q.head; temp->next != update_u;
					temp = temp->next);

				if (!update_u->next)
					reset_q.tail = temp;

				temp->next = update_u->next;
			}

			FREE(update_u);
			break;
		}
	release_buffer(buf);
}

void log_zone_error(int zone, int cmd_no, char *message) {
	mudlogf(NRM, NULL, FALSE,	"ZONEERR: zone file: %s", message);
	mudlogf(NRM, NULL, FALSE,	"ZONEERR: ...offending cmd: '%c' cmd in zone #%d, line %d",
			ZCMD.command, zone_table[zone].number, ZCMD.line);
}

#define ZONE_ERROR(message) \
	{ log_zone_error(zone, cmd_no, message); last_cmd = 0; }

void make_maze(int zone)
{
	int card[400], temp, x, y, dir, room;
	int num, next_room = 0, test, r_back;
	int vnum = zone_table[zone].number;
	
	for (test = 0; test < 400; test++) {
		card[test] = test;
		temp = test;
		dir = temp / 100;
		temp = temp - (dir * 100);
		x = temp / 10;
		temp = temp - (x * 10);
		y = temp;
		room = (vnum * 100) + (x * 10) + y;
//		room = Room::Real(room);
	if (!Room::Find(room))	continue;
		if ((x == 0) && (dir == 0))
			continue;
		if ((y == 9) && (dir == 1))
			continue;
		if ((x == 9) && (dir == 2))
			continue;
		if ((y == 0) && (dir == 3))
			continue;
		world[room].Direction(dir)->to_room = -1;
		REMOVE_BIT(ROOM_FLAGS(room), ROOM_NOTRACK);
	}
	for (x = 0; x < 399; x++) {
		y = Number(0, 399);
		temp = card[y];
		card[y] = card[x];
		card[x] = temp;
	}

	for (num = 0; num < 400; num++) {
		temp = card[num];
		dir = temp / 100;
		temp = temp - (dir * 100);
		x = temp / 10;
		temp = temp - (x * 10);
		y = temp;
		room = (vnum * 100) + (x * 10) + y;
		r_back = room;
//		room = Room::Real(room);
	if (!Room::Find(room))	continue;
		if ((x == 0) && (dir == 0))
			continue;
		if ((y == 9) && (dir == 1))
			continue;
		if ((x == 9) && (dir == 2))
			continue;
		if ((y == 0) && (dir == 3))
			continue;
		if (world[room].Direction(dir)->to_room != -1)
			continue;
		switch(dir) {
			case 0:
				next_room = r_back - 10;
				break;
			case 1:
				next_room = r_back + 1;
				break;
			case 2:
				next_room = r_back + 10;
				break;
			case 3:
				next_room = r_back - 1;
				break;
		}
//		next_room = Room::Real(next_room);
	if (!Room::Find(next_room))	continue;
		test = find_first_step(room, next_room, 0);
		switch (test) {
			case BFS_ERROR:
				log("Maze making error.");
				break;
			case BFS_ALREADY_THERE:
				log("Maze making error.");
				break;
			case BFS_NO_PATH:

				world[room].Direction(dir)->to_room = next_room;
				world[next_room].Direction((int) rev_dir[dir])->to_room = room;
				break;
		}
	}
	for (num = 0;num < 100;num++) {
		room = (vnum * 100) + num;
//		room = Room::Real(room);
/* Remove the next line if you want to be able to track your way through 
the maze */
/*		SET_BIT(ROOM_FLAGS(room), ROOM_NOTRACK); */

//		REMOVE_BIT(ROOM_FLAGS(room), ROOM_BFS_MARK);
	}
}

/* execute the reset command table of a given zone */
void reset_zone(int zone) {
	int cmd_no, repeat_no, last_cmd = 0, maze_made = 0;
	Character *mob = NULL;
	Object *obj = NULL, *obj_to;
	Trigger *trig;
	Room *room;

	for (cmd_no = 0; ZCMD.command != 'S'; ++cmd_no) {

		if ((ZCMD.if_flag || (ZCMD.command == 'C')) && !last_cmd)
			continue;
		
		if (ZCMD.repeat <= 0)
			ZCMD.repeat = 1;

		for (repeat_no = 0; repeat_no < ZCMD.repeat; ++repeat_no) {
			switch (ZCMD.command) {
				case '*':			/* ignore command */
					last_cmd = 0;
					break;

				case 'M':			/* read a mobile */
					if ((!ZCMD.arg2 || ((MOB_FLAGGED(Character::Index[ZCMD.arg1].proto, MOB_SENTINEL) ?
							(count_mobs_in_room(ZCMD.arg1, ZCMD.arg3)) :
							(Character::Index[ZCMD.arg1].number)) < ZCMD.arg2)) &&
							((ZCMD.arg4 <= 0) ||
							(count_mobs_in_zone(ZCMD.arg4, world[ZCMD.arg3].zone) < ZCMD.arg4))) {
						if (ZCMD.arg3 >= 0) {
							mob = Character::Read(ZCMD.arg1);
							mob->ToRoom(ZCMD.arg3);
							load_mtrigger(mob);
							last_cmd = 1;
						}
					} else
						last_cmd = 0;
					break;

				case 'O':			/* read an object */
					if (!ZCMD.arg2 || Object::Index[ZCMD.arg1].number < ZCMD.arg2) {
						if (ZCMD.arg3 >= 0) {
							obj = Object::Read(ZCMD.arg1);
							obj->ToRoom(ZCMD.arg3);
							load_otrigger(obj);
							last_cmd = 1;
						}
					} else
						last_cmd = 0;
					break;

				case 'P':			/* object to object */
					if (!ZCMD.arg2 || Object::Index[ZCMD.arg1].number < ZCMD.arg2) {
						if (!(obj_to = get_obj_num(ZCMD.arg3))) {
							ZONE_ERROR("target obj not found");
							break;
						}
						obj = Object::Read(ZCMD.arg1);
						obj->ToObj(obj_to);
						load_otrigger(obj);
						last_cmd = 1;
					} else
						last_cmd = 0;
					break;

				case 'G':			/* obj_to_char */
					if (!mob) {
						ZONE_ERROR("attempt to give obj to non-existant mob");
						break;
					}
					if (!ZCMD.arg2 || Object::Index[ZCMD.arg1].number < ZCMD.arg2) {
						obj = Object::Read(ZCMD.arg1);
						obj->ToChar(mob);
						load_otrigger(obj);
						last_cmd = 1;
					} else
						last_cmd = 0;
					break;

				case 'E':			/* object to equipment list */
					if (!mob) {
						ZONE_ERROR("trying to equip non-existant mob");
						break;
					}
					if (!ZCMD.arg2 || Object::Index[ZCMD.arg1].number < ZCMD.arg2) {
						if (ZCMD.arg3 < 0 || ZCMD.arg3 >= NUM_WEARS) {
							switch (ZCMD.arg3) {
								case POS_WIELD_TWO:
								case POS_HOLD_TWO:
								case POS_WIELD:
									if (!mob->equipment[ZCMD.arg3]) {
										obj = Object::Read(ZCMD.arg1);
										equip_char(mob, obj, WEAR_HAND_R);
										load_otrigger(obj);
										last_cmd = 1;
									}
									break;
								case POS_WIELD_OFF:
								case POS_LIGHT:
								case POS_HOLD:
									if (!mob->equipment[ZCMD.arg3]) {
										obj = Object::Read(ZCMD.arg1);
										equip_char(mob, obj, WEAR_HAND_L);
										load_otrigger(obj);
										last_cmd = 1;
									}
									break;
								default:
									ZONE_ERROR("invalid equipment pos number");
							}
						} else if (!mob->equipment[ZCMD.arg3]) {
							obj = Object::Read(ZCMD.arg1);
							equip_char(mob, obj, ZCMD.arg3);
							load_otrigger(obj);
							last_cmd = 1;
						}
					} else
						last_cmd = 0;
					break;

				case 'R': /* rem obj from room */
					if ((obj = get_obj_in_list_num(ZCMD.arg2, world[ZCMD.arg1].contents)) != NULL) {
						obj->FromRoom();
						obj->Extract();
					}
					last_cmd = 1;
					break;
					
				case 'C':			/* command mob to do something */
					if(!mob || !ZCMD.command_string) {
						if (!mob)						ZONE_ERROR("trying to command nonexistant mob")
						else if (!ZCMD.command_string)	ZONE_ERROR("trying to command mob with no command")
					} else {
						command_interpreter(mob, ZCMD.command_string);
						last_cmd = 1;
					}
					break;
					
				case 'D':			/* set state of door */
					if (ZCMD.arg2 < 0 || ZCMD.arg2 >= NUM_OF_DIRS || ZCMD.arg1 == NOWHERE || 
							!world[ZCMD.arg1].Direction(ZCMD.arg2)) {
						char *buf = get_buffer(MAX_STRING_LENGTH);
						sprintf(buf, "door %s does not exist in room %d", dirs[ZCMD.arg2], ZCMD.arg1);
						ZONE_ERROR(buf);
						release_buffer(buf);
					} else {
						RoomDirection *dir = world[ZCMD.arg1].Direction(ZCMD.arg2);
						switch (ZCMD.arg3) {
							case 0:
								REMOVE_BIT(dir->exit_info, Exit::Locked);
								REMOVE_BIT(dir->exit_info, Exit::Closed);
								break;
							case 1:
								SET_BIT(dir->exit_info, Exit::Closed);
								REMOVE_BIT(dir->exit_info, Exit::Locked);
								break;
							case 2:
								SET_BIT(dir->exit_info, Exit::Locked);
								SET_BIT(dir->exit_info, Exit::Closed);
								break;
						}
					}
					last_cmd = 1;
					break;
					
				case 'Z':			/* This zone is a MAZE zone */
					if (!maze_made) {
						maze_made = 1;
						make_maze(zone);
					}
					last_cmd = 1;
					break;
					
				case 'T':
					last_cmd = 0;
					switch (ZCMD.arg1) {
						case MOB_TRIGGER:
							if (!mob) {
								ZONE_ERROR("attempt to assign trigger to non-existant mob");
								continue;
							}
							if (Trigger::Index[ZCMD.arg2].number < ZCMD.arg3) {
								trig = Trigger::Read(ZCMD.arg2);
								if (!SCRIPT(mob))
									SCRIPT(mob) = new Script(Datatypes::Character);
								add_trigger(SCRIPT(mob), trig); //, -1);
								last_cmd = 1;
							}
							break;
						case OBJ_TRIGGER:
							if (!obj) {
								ZONE_ERROR("attempt to assign trigger to non-existant object");
								continue;
							}
							if (Trigger::Index[ZCMD.arg2].number < ZCMD.arg3) {
								trig = Trigger::Read(ZCMD.arg2);
								
								if (!SCRIPT(obj))
									SCRIPT(obj) = new Script(Datatypes::Object);
								add_trigger(SCRIPT(obj), trig); //, -1);
								last_cmd = 1;
							}
							break;
						case WLD_TRIGGER:
							if (ZCMD.arg4 == -1) {
								ZONE_ERROR("attempt to assign trigger to non-existant room");
								continue;
							}
							room = &world[ZCMD.arg4];
							if (Trigger::Index[ZCMD.arg2].number < ZCMD.arg3) {
								trig = Trigger::Read(ZCMD.arg2);

								if (!SCRIPT(room))
									SCRIPT(room) = new Script(Datatypes::Room);
								add_trigger(SCRIPT(room), trig); //, -1);
								last_cmd = 1;
							}
							break;
						default:
							ZONE_ERROR("unknown trigger type");
							ZCMD.command = '*';
							break;
					}
					break;
					
				default:
					ZONE_ERROR("unknown cmd in reset table; cmd disabled");
					ZCMD.command = '*';
					break;
			}
		}
	}
	zone_table[zone].age = 0;
	
	for (VNum room_vnum = zone_table[zone].number * 100;
			room_vnum <= zone_table[zone].top; room_vnum++) {
		if (Room::Find(room_vnum))	reset_wtrigger(&world[room_vnum]);
	}
}



/* for use in reset_zone; return TRUE if zone 'nr' is free of PC's  */
int is_empty(int zone_nr) {
	Descriptor *i;

	START_ITER(iter, i, Descriptor *, descriptor_list)
		if ((STATE(i) == CON_PLAYING) && (world[IN_ROOM(i->character)].zone == zone_nr))
			return 0;
	
	return 1;
}





/*************************************************************************
*  stuff related to the save/load player system				 *
*********************************************************************** */


SInt32 get_id_by_name(const char *name) {
	int i;
	char *arg = get_buffer(MAX_INPUT_LENGTH);

	one_argument(name, arg);
	for (i = 0; i < player_table.Count(); i++)
		if (!str_cmp(player_table[i].name, arg)) {
			release_buffer(arg);
			return (player_table[i].id);
		}
	
	release_buffer(arg);
	return 0;
}


const char *get_name_by_id(IDNum id) {
	int i;

	for (i = 0; i < player_table.Count(); i++)
		if (player_table[i].id == id)
			return (player_table[i].name);

	return NULL;
}


/* create a new entry in the in-memory index table for the player file */
int create_entry(const char *name) {
	PlayerIndex	temp;
	
	CREATE(temp.name, char, strlen(name) + 1);

	//	copy lowercase equivalent of name to table field
	for (int i = 0; (*(temp.name + i) = LOWER(*(name + i))); i++);

	player_table.Append(temp);
	
	return (player_table.Count());
}



/************************************************************************
*  funcs of a (more or less) general utility nature			*
********************************************************************** */


/* release memory allocated for a char struct */


/* read contents of a text file, alloc space, point buf to it */
int file_to_string_alloc(const char *name, char **buf) {
	char *temp = get_buffer(MAX_STRING_LENGTH);
	
	if (*buf)	FREE(*buf);

	if (file_to_string(name, temp) < 0) {
		*buf = NULL;
		release_buffer(temp);
		return -1;
	} else {
		*buf = str_dup(temp);
		release_buffer(temp);
		return 0;
	}
}


/* read contents of a text file, and place in buf */
int file_to_string(const char *name, char *buf) {
	FILE *fl;
	char *tmp = get_buffer(READ_SIZE+3);

	*buf = '\0';

	if (!(fl = fopen(name, "r"))) {
		sprintf(tmp, "SYSERR: Error reading %s", name);
		perror(tmp);
		release_buffer(tmp);
		release_buffer(tmp);
		fclose(fl);
		return (-1);
	}
	do {
		fgets(tmp, READ_SIZE, fl);
		tmp[strlen(tmp) - 1] = '\0'; /* take off the trailing \n */
		strcat(tmp, "\r\n");

		if (!feof(fl)) {
			if (strlen(buf) + strlen(tmp) + 1 > MAX_STRING_LENGTH) {
				log("SYSERR: %s: string too big (%d max)", name, MAX_STRING_LENGTH);
				*buf = '\0';
				release_buffer(tmp);
				fclose(fl);
				return -1;
			}
			strcat(buf, tmp);
		}
	} while (!feof(fl));

	fclose(fl);

	release_buffer(tmp);
	return (0);
}



/* clear some of the the working variables of a char */
void Character::Reset(void) {
	int i;

	for (i = 0; i < NUM_WEARS; i++)
		GET_EQ(this, i) = NULL;

	followers.Clear();
	master = NULL;
//	IN_ROOM(ch) = NOWHERE;
	ID(GET_IDNUM(this));
	contents.Clear();
	FIGHTING(this) = NULL;
	general.position = POS_STANDING;

	general.misc.carry_weight = 0;
	general.misc.carry_items = 0;

	if (GET_HIT(this) <= 0)		GET_HIT(this) = 1;
	if (GET_MANA(this) <= 0)	GET_MANA(this) = 1;
	if (GET_MOVE(this) <= 0)	GET_MOVE(this) = 1;

	CheckRegenRates(this);

	GET_LAST_TELL(this) = NOBODY;
}


/* initialize a new character only if class is set */
void Character::Initialize(void) {
	int i;
	IDNum newID = 0;

	// if (GET_PFILEPOS(this) < 0) {
	// }

	newID = get_id_by_name(RealName());
	if (!newID) {
		newID = ++top_idnum;
		GET_PFILEPOS(this) = create_entry(RealName());
		player_table.Top().id = newID;
	}

	//	create a player_special structure
	if (!player)
		player = new PlayerData;

	//	if this is our first player --- he be God
	if (player_table.Count() == 1) {
		GET_LEVEL(this) = 1;
		GET_TRUST(this) = MAX_TRUST;
		
		SET_BIT(PRF_FLAGS(this), Preference::Color | Preference::RoomFlags | Preference::Invuln | Preference::AutoExit);

		points.max_hit = 5000;
		points.max_mana = 5000;
		points.max_move = 800;
		SET_BIT(STF_FLAGS(this), 0xFFFFFFFF);
	}

	player->time.birth = player->time.logon = time(0);
	player->time.played = 0;

	roll_height_weight(this);

	points.max_hit = points.hit = MAX((aff_abils.Con * 10), 1);
	points.max_mana = points.mana = MAX((aff_abils.Wis * 10), 1);
	points.max_move = points.move = MAX((aff_abils.Dex * 10), 1);

	ID(GET_IDNUM(this) = newID);

	// for (i = 1; i <= MAX_SKILLS; i++) {
	// 	SET_SKILL(this, i, !IS_STAFF(this) ? 0 : 100);
	// }

	affectbits = 0;

//	ch->real_abils.intel = 100;
//	ch->real_abils.per = 100;
//	ch->real_abils.agi = 100;
//	ch->real_abils.str = 100;
//	ch->real_abils.con = 100;

	GET_COND(this, 0) = (IS_STAFF(this) ? -1 : 24);
	GET_COND(this, 1) = (IS_STAFF(this) ? -1 : 24);
	GET_COND(this, 2) = (IS_STAFF(this) ? -1 : 0);

	GET_LOADROOM(this) = NOWHERE;
	
	Save(NOWHERE);
	save_player_index();
}


void vwear_object(int wearpos, Character * ch) {
	int found = 0;
	
	Index<Object>::Iterator	iter(Object::Index);
	IndexData<Object> *o;
	while ((o = iter.Next()))
		if (CAN_WEAR(o->proto, wearpos))
			ch->Sendf("%3d. [%5d] %s\r\n", ++found, o->vnum, o->proto->Name());
}


void save_player_index(void) {
	int i;
	FILE *index_file;

	if(!(index_file = fopen(PLR_INDEX_FILE, "w"))) {
		log("SYSERR:  Could not write player index file");
		return;
	}

	for(i = 0; i < player_table.Count(); i++)
		fprintf(index_file, "%u	%s\n", player_table[i].id, player_table[i].name);
//		fprintf(index_file, "%d	%c%s\n", (int)player_table[i].id, LOWER(*player_table[i].name), player_table[i].name + 1);

	fclose(index_file);
}

void free_purged_lists(void);

#if 0
void FreeAllMemory(void);
void FreeAllMemory(void) {
	Character *	ch;
	Object *	obj;
	Trigger *	trig; //, *next_trig;
	CmdlistElement *cmd, *next_cmd;
	ExtraDesc *	exdesc, *next_exdesc;
	struct builder_list *builder, *next_builder;
	struct reset_q_element *rqelement, *next_rqelement;
	SInt32		i, dir, x;
	
	log("Freeing memory for shutdown:");

	if (help_table) {
		log("  - Help");
		for (i = 0; i < top_of_helpt; i++) {
			if (help_table[i].keyword)								free(help_table[i].keyword);
			if (help_table[i].entry && !help_table[i].duplicate)	free(help_table[i].entry);
		}
		FREE(help_table);
	}
	
	log("  - Loaded lists:");
	log("    - Mobs");
	START_ITER(ch_iter, ch, Character *, character_list) {
		ch->extract();
	} END_ITER(ch_iter);
	
	log("    - Objs");
	START_ITER(obj_iter, obj, Object *, object_list) {
		obj->extract();
	} END_ITER(obj_iter);
	
	log("    - Scripts");
	for (i = 0; i <= top_of_world; i++) {	// World scripts first
		if (world[i].script)	delete world[i].script;
	}
	START_ITER(trig_iter, trig, Trigger *, trig_list) {
		trig->extract();
	}
	free_purged_lists();
	
	log("  - Prototypes:");
	log("    - Mobs");
	for (i = 0; i < Character::Index.Count(); i++) {
		if (Character::Index.Real(i).proto)
			delete Character::Index(i).proto;
	}
	
	log("    - Objs");
	for (i = 0; i < Object::Index.Count(); i++) {
		if (Object::Index.Real(i).proto)
			delete Object::Index.Real(i).proto;
	}
	
	log("    - Scripts");
	for (i = 0; i < trig_index.Count(); i++) {
		if (trig_index.Real(i).proto) {
			for (cmd = (trig_index.Real(i).proto)->cmdlist; cmd; cmd = next_cmd) {
				next_cmd = cmd->next;
				delete cmd;
			}
			delete trig_index.Real(i).proto;
		}
	}
	
	log("    - Rooms");
	for (i = 0; i <= top_of_world; i++) {
		SSFree(world[i].name);
		SSFree(world[i].description);
		for (dir = 0; dir < NUM_OF_DIRS; dir++) {
			if (world[i].Direction(dir))	delete world[i].Direction(dir);
		}
		for (exdesc = world[i].ex_description; exdesc; exdesc = next_exdesc) {
			next_exdesc = exdesc->next;
			delete exdesc;
		}
	}
	FREE(world);
	
	log("  - Zone data");
	for (i = 0; i <= top_of_zone_table; i++) {
		if (zone_table[i].name)		free(zone_table[i].name);
		for (builder = zone_table[i].builders; builder; builder = next_builder) {
			next_builder = builder->next;
			if (builder->name)	free(builder->name);
			free(builder);
		}
		for (x = 0; zone_table[i].cmd[x].command != 'S'; x++) {
			if (zone_table[i].cmd[x].command == 'C')
				free(zone_table[i].cmd[x].command_string);
		}
		free(zone_table[i].cmd);
	}
	log("  - Zone Resets");
	for (rqelement = reset_q.head; rqelement; rqelement = next_rqelement) {
		next_rqelement = rqelement->next;
		free(rqelement);
	}
	log("  - Player index");
	for (i = 0; i <= top_of_p_table; i++) {
		if (player_table[i].name)	free(player_table[i].name);
	}
	free(player_table);char *credits = NULL;		/* game credits			 */
	
	log("  - Texts");
	if (credits)	free(credits);
	if (news)		free(news);
	if (motd)		free(motd);
	if (imotd)		free(imotd);
	if (help)		free(help);
	if (info)		free(info);
	if (wizlist)	free(wizlist);
	if (immlist)	free(immlist);
	if (background)	free(background);
	if (handbook)	free(handbook);
	if (policies)	free(policies);
}
#endif


/* returns zone (rnum) 'number' (vnum) falls into.  -1 if none */
int find_owner_zone(VNum number)
{
	int i;
	VNum top, bottom;

	if (number > zone_table[top_of_zone_table].top)
		return -1;

	for (i = 0, top = 0; i <= top_of_zone_table; i++) {
		bottom = zone_table[i].number * 100;
		top = zone_table[i].top;
		
		if (number < bottom)	// This is a number not within the range of the given zone.
			return -1;
		
		if ((number >= bottom) && (number <= top))
			return i;
	}
	
	return -1;
}
