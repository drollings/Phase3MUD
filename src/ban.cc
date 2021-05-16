/***************************************************************************\
[*]    __     __  ___                ___  | LexiMUD is a C++ MUD codebase [*]
[*]   / /  ___\ \/ (_) /\/\  /\ /\  /   \ |        for Sci-Fi MUDs        [*]
[*]  / /  / _ \\  /| |/    \/ / \ \/ /\ / | Copyright(C) 1997-1999        [*]
[*] / /__|  __//  \| / /\/\ \ \_/ / /_//  | All rights reserved           [*]
[*] \____/\___/_/\_\_\/    \/\___/___,'   | Chris Jacobson (FearItself)   [*]
[*]      A Creation of the AvP Team!      | fear@technologist.com         [*]
[-]---------------------------------------+-------------------------------[-]
[*] File : ban.c++                                                        [*]
[*] Usage: Host Ban Services                                              [*]
\***************************************************************************/


#include "types.h"

#include "ban.h"

#include "characters.h"
#include "descriptors.h"
#include "utils.h"
#include "buffer.h"
#include "comm.h"
#include "interpreter.h"
#include "db.h"


Vector<Ban *>	bans;


//	prototypes
ACMD(do_ban);
ACMD(do_unban);


const char *ban_types[] = {
	"no",
	"new",
	"select",
	"all",
	"\n"
};


Ban::Ban(void) : mSite(NULL), mID(0), mDate(time(0)), mType(Ban::Not) {
}


Ban::Ban(const char *site, const char *name, SInt32 type, time_t date) :
		mSite(str_dup(site)), mID(get_id_by_name(name)),
		mDate(date ? date : time(0)), mType((BanType)type) {
//	strncpy(site, bannedSite, BANNED_SITE_LENGTH);
//	site[BANNED_SITE_LENGTH] = '\0';
//	strncpy(name, bannerName, MAX_NAME_LENGTH);
//	name[MAX_NAME_LENGTH] = '\0';
}


Ban::~Ban(void) {
	if (mSite)	free(mSite);
}


void Ban::Load(void) {
	FILE *	fl;
	SInt32	date, type;
	char *site, *ban_type, *name;

	if (!(fl = fopen(BAN_FILE, "r"))) {
		if (errno == ENOENT)
			log("Ban file %s doesn't exist.", BAN_FILE);
		else {
			log("SYSERR: Unable to open banfile");
			perror(BAN_FILE);
		}
		return;
	}
	
	site = get_buffer(BANNED_SITE_LENGTH + 1);
	ban_type = get_buffer(100);
	name = get_buffer(MAX_NAME_LENGTH + 1);

	while (fscanf(fl, " %s %s %d %s ", ban_type, site, &date, name) == 4) {
//		CREATE(next_node, struct ban_list_element, 1);
		type = search_block(ban_type, ban_types, true);
		if (type != -1)	bans.Append(new Ban(site, name, type, date));
	}

	release_buffer(site);
	release_buffer(ban_type);
	release_buffer(name);
	fclose(fl);
}


Ban::BanType Ban::Banned(char *hostname) {
	BanType			banLevel = Not;
	char *			nextchar;

	if (!hostname || !*hostname)
		return Ban::Not;

	for (nextchar = hostname; *nextchar; nextchar++)
		*nextchar = LOWER(*nextchar);

	for (int i = 0; i < bans.Count(); i++)
		if ((bans[i]->mType > banLevel) && strstr(hostname, bans[i]->mSite))
			banLevel = bans[i]->mType;
	
	return banLevel;
}


void Ban::Write(void) {
	FILE *	fp = fopen(BAN_FILE, "w");
	
	if (!fp) {
		perror("SYSERR: write_ban_list");
		return;
	}
	
	for (int i = 0; i < bans.Count(); i++) {
		const char *name = get_name_by_id(bans[i]->mID);
		fprintf(fp, "%s %s %ld %s\n", ban_types[bans[i]->mType], bans[i]->mSite, bans[i]->mDate,
				name ? name : "<UNKNOWN>");
	}
	
	fclose(fp);
}


ACMD(do_ban) {
	Ban::BanType	type;

	if (!*argument) {
		const char *format = "%-25.25s  %-8.8s  %-10.10s  %-16.16s\r\n";
		const char *theTime, *banner;
		char *		timestr;
		
		if (!bans.Count()) {
			ch->Send("No sites are banned.\r\n");
			return;
		}
		
		ch->Sendf(format,
				"Banned Site Name",
				"Ban Type",
				"Banned On",
				"Banned By");
		ch->Sendf(format,
				"---------------------------------",
				"---------------------------------",
				"---------------------------------",
				"---------------------------------");

		for (int i = 0; i < bans.Count(); i++) {
			if (bans[i]->mDate) {
				timestr = asctime(localtime(&(bans[i]->mDate)));
				*(timestr + 10) = 0;
				theTime = timestr;
			} else
				theTime = "Unknown";
			banner = get_name_by_id(bans[i]->mID);
			ch->Sendf(format, bans[i]->mSite, ban_types[bans[i]->mType], theTime, banner ? banner : "<UNKNOWN>");
		}
	} else {
		char *nextchar;
		char *site = get_buffer(MAX_INPUT_LENGTH);
		char *flag = get_buffer(MAX_INPUT_LENGTH);

		two_arguments(argument, flag, site);
		
		if (!*site || !*flag)
			ch->Send("Usage: ban all|select|new <site>\r\n");
		else if ((type = (Ban::BanType)search_block(flag, ban_types, false)) == -1)
			ch->Send("Flag must be ALL, SELECT, or NEW.\r\n");
		else {
			int i = 0;
			for (i; i < bans.Count(); i++) {
				if (!str_cmp(bans[i]->mSite, site)) {
					ch->Send("That site has already been banned -- unban it to change the ban type.\r\n");
					return;
				}
			}
			
			for (nextchar = site; *nextchar; nextchar++)
				*nextchar = LOWER(*nextchar);
			
			bans.Add(new Ban(site, ch->RealName(), type));

			Ban::Write();
			
			mudlogf(NRM, ch, TRUE,
					"BAN: %s has banned %s for %s players.", ch->RealName(), site,
					ban_types[type]);
			ch->Sendf("Site banned.\r\n");
		}
		release_buffer(site);
		release_buffer(flag);
	}
}


ACMD(do_unban) {
	char *site = get_buffer(MAX_INPUT_LENGTH);

	one_argument(argument, site);
	
	if (!*site)
		ch->Send("A site to unban might help.\r\n");
	else {
		int i = 0;
		for (i; i < bans.Count(); i++)
			if (!str_cmp(bans[i]->mSite, site))
				break;

		if (i < bans.Count()) {
			ch->Send("Site unbanned.\r\n");
			mudlogf(NRM, ch, TRUE,
					"BAN: %s removed the %s-player ban on %s.",
					ch->RealName(), ban_types[bans[i]->mType], bans[i]->mSite);
			delete bans[i];
			bans.Remove(i);
			Ban::Write();
		} else
			ch->Send("That site is not currently banned.\r\n");
	}
	release_buffer(site);
}


/**************************************************************************
 *  Code to check for invalid names (i.e., profanity, etc.)		  *
 *  Written by Sharon P. Goza						  *
 **************************************************************************/

#define MAX_INVALID_NAMES	200
char *invalid_list[MAX_INVALID_NAMES];

int num_invalid = 0;

bool Ban::ValidName(const char *newname) {
	int i;
	char *tempname;
	const char *	name;
	Descriptor *dt;

	/*
	 * Make sure someone isn't trying to create this same name.  We want to
	 * do a 'str_cmp' so people can't do 'Bob' and 'BoB'.  This check is done
	 * here because this will prevent multiple creations of the same name.
	 * Note that the creating login will not have a character name yet. -gg
	 */
	LListIterator<Descriptor *>	iter(descriptor_list);
	while ((dt = iter.Next())) {
		if (dt->character && (name = dt->character->RealName()) && !str_cmp(name, newname)) {
			return false;
		}
	}
	
	/* return valid if list doesn't exist */
	if (!invalid_list || num_invalid < 1)
		return true;
	
	tempname = get_buffer(MAX_INPUT_LENGTH);
	
	/* change to lowercase */
	strcpy(tempname, newname);
	for (i = 0; tempname[i]; i++)
		tempname[i] = LOWER(tempname[i]);

	/* Does the desired name contain a string in the invalid list? */
	for (i = 0; i < num_invalid; i++)
		if (strstr(tempname, invalid_list[i])) {
			release_buffer(tempname);
			return false;
		}
	
	release_buffer(tempname);
	
	return true;
}


void Ban::ReadInvalids(void) {
	FILE *fp;
	char *temp;

	if (!(fp = fopen(XNAME_FILE, "r"))) {
		perror("SYSERR: Unable to open invalid name file");
		return;
	}
	
	temp = get_buffer(MAX_INPUT_LENGTH);
	num_invalid = 0;
	while(get_line(fp, temp) && num_invalid < MAX_INVALID_NAMES)
		invalid_list[num_invalid++] = str_dup(temp);
	release_buffer(temp);
	
	if (num_invalid > MAX_INVALID_NAMES) {
		log("SYSERR: Too many invalid names; change MAX_INVALID_NAMES in ban.c");
		exit(1);		
	}

	fclose(fp);
}
