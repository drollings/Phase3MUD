/***************************************************************************\
[*]    __     __  ___                ___  | LexiMUD is a C++ MUD codebase [*]
[*]   / /  ___\ \/ (_) /\/\  /\ /\  /   \ |        for Sci-Fi MUDs        [*]
[*]  / /  / _ \\  /| |/    \/ / \ \/ /\ / | Copyright(C) 1997-1999        [*]
[*] / /__|  __//  \| / /\/\ \ \_/ / /_//  | All rights reserved           [*]
[*] \____/\___/_/\_\_\/    \/\___/___,'   | Chris Jacobson (FearItself)   [*]
[*]      A Creation of the AvP Team!      | fear@technologist.com         [*]
[-]---------------------------------------+-------------------------------[-]
[*] File : clans.c++                                                      [*]
[*] Usage: Clan module code                                               [*]
\***************************************************************************/


#include "clans.h"
#include "characters.h"
#include "objects.h"
#include "interpreter.h"
#include "descriptors.h"
#include "db.h"
#include "utils.h"
#include "olc.h"
#include "files.h"
#include "comm.h"
#include "find.h"
#include "rooms.h"
#include "buffer.h"
#include "strings.h"


//	Global Data
Map<VNum, Clan>	Clan::Clans;
extern int	scheck;


//	External Functions
int		count_hash_records(FILE * fl);


//	Local Functions
ACMD(do_apply);
ACMD(do_enlist);
ACMD(do_boot);
ACMD(do_clanlist);
ACMD(do_clanstat);
ACMD(do_members);
ACMD(do_forceenlist);
ACMD(do_resign);
ACMD(do_clanwho);
ACMD(do_clanpromote);
ACMD(do_clandemote);
ACMD(do_deposit);
//ACMD(do_withdraw);


//	Clan Data
Clan::Clan(void) : vnum(0), name(NULL), description(NULL), owner(0), room(0), savings(0) {
	for (SInt32 x = 0; x < NumRanks - 1; x++)
		ranks[x] = NULL;
}


Clan::Clan(const Clan &clan) : vnum(clan.vnum), name(str_dup(clan.name)),
		description(str_dup(clan.description)), owner(clan.owner), room(clan.room),
		savings(clan.savings), members(clan.members) {
	for (SInt32 x = 0; x < NumRanks - 1; x++)
#ifdef GOT_RID_OF_IT
		ranks[x] = str_dup(clan.ranks[x] ? clan.ranks[x] : titles[RACE_HUMAN][x + 1].title_m);
#else
		ranks[x] = (clan.ranks[x] ? str_dup(clan.ranks[x]) : NULL);
#endif
}


Clan::~Clan(void) {
	if (name)			free(name);
	if (description)	free(description);
	for (SInt32 x = 0; x < Clan::NumRanks - 1; x++)
		if (ranks[x])	free(ranks[x]);
	members.Clear();
}


bool Clan::AddMember(UInt32 id) {
	if (IsMember(id))	return false;
	members.Add(id);
	
	Clan::Save();
	return true;
}


bool Clan::RemoveMember(UInt32 id) {
	if (IsMember(id)) {
		members.Remove(id);
		Clan::Save();
		return true;
	}
	return false;
}


ACMD(do_clanlist) {
	char *buf = get_buffer(MAX_STRING_LENGTH);
	char *buf2 = get_buffer(MAX_INPUT_LENGTH);
	const char *name;
	
	strcpy(buf, "    #  Clan                             Members   Leader\r\n");
	strcat(buf, "-------------------------------------------------------------\r\n");
//				"  123. 123456789012345678901234567890     123     123456789012345\r\n"
	MapIterator<VNum, Clan>		iter(Clan::Clans);
	Clan *clan;
	while ((clan = iter.Next())) {
//		Clan &	clan = Clan::Clans.Real(i);
		name = get_name_by_id(clan->owner);
		strcpy(buf2, name ? name : "<NONE>");
		sprintf(buf + strlen(buf), "  %3d. %-30.30s     %3d     %-15s\r\n", clan->vnum,
				clan->name, clan->members.Count(), CAP(buf2));
	}
	page_string(ch->desc, buf, true);

	release_buffer(buf);
	release_buffer(buf2);
}


ACMD(do_clanstat) {
	char *			arg = get_buffer(MAX_INPUT_LENGTH);
	const char *	name;
	VNum			clan, room;
	
	one_argument(argument, arg);
	
	if (!*arg)	clan = GET_CLAN(ch);
	else		clan = atoi(arg);
	
	if (!Clan::Find(clan))
		ch->Send("Stats on which clan?\r\n");
	else {
		room = Clan::Clans[clan].room;
		name = get_name_by_id(Clan::Clans[clan].owner);
		strcpy(arg, name ? name : "<NONE>");
		ch->Sendf("Clan       : %d\r\n"
				 "Name       : %s\r\n"
				 "Description:\r\n"
				 "%s",
				Clan::Clans[clan].vnum,
				Clan::Clans[clan].name,
				Clan::Clans[clan].description ? Clan::Clans[clan].description : "<NONE>\r\n");

		if (GET_CLAN(ch) == clan || STF_FLAGGED(ch, Staff::Clans)) {
			ch->Sendf("Owner      : [%5d] %s\r\n"
					 "Room       : [%5d] %20s\r\n"
					 "Savings    : %d gold\r\n"
					 "Members    : %d\r\n",

					Clan::Clans[clan].owner, CAP(arg),
					Clan::Clans[clan].room, (room != NOWHERE) ? world[room].Name() : "<Not Created>",
					Clan::Clans[clan].savings,
					Clan::Clans[clan].members.Count());
		}
	}
	release_buffer(arg);
}


ACMD(do_members) {
	SInt32			i,
					counter,
					columns = 0;
	UInt32			member;
	Character *		applicant;
	Descriptor *	d;
	VNum			clanVNum;
	char *			buf;
	const char *	name;
	bool			found = false;
	
	clanVNum = IS_STAFF(ch) ? atoi(argument) : GET_CLAN(ch);
	
	if (!Clan::Find(clanVNum) || ((GET_CLAN_LEVEL(ch) < Clan::Rank::Member) && !IS_STAFF(ch))) {
		if (IS_STAFF(ch))	ch->Send("Clan does not exist.\r\n");
		else				ch->Send("You aren't even in a clan!\r\n");
		return;
	}
	
	buf = get_buffer(MAX_STRING_LENGTH);
	
	Clan &clan = Clan::Clans[clanVNum];
	
	sprintf(buf, "Members of %s:\r\n", clan.name);
	columns = counter = 0;
	for (i = 0; i < clan.members.Count(); i++) {
		member = clan.members[i];
		if ((name = get_name_by_id(member))) {
			found = true;
			++counter;
			sprintf(buf + strlen(buf), "  %c%-19.19s %s", UPPER(*name), name + 1,
					!(++columns % 2) ? "\r\n" : "");
		}
	}
	if (columns % 2)	strcat(buf, "\r\n");
	if (!found)			strcat(buf, "  None.\r\n");
	columns = 0;
	found = false;
	LListIterator<Descriptor *>	desc_iter(descriptor_list);
	while ((d = desc_iter.Next())) {
		if ((applicant = d->Original()) && !IS_NPC(applicant) &&
				(GET_CLAN(applicant) == clanVNum) &&
				(GET_CLAN_LEVEL(applicant) == Clan::Rank::Applicant)) {
			if (!found) {
				strcat(buf, "Applicants Currently Online:\r\n");
				found = true;
			}
			sprintf(buf + strlen(buf), "  %-20.20s %s", applicant->RealName(),
					!(++columns % 2) ? "\r\n" : "");
			++counter;
		}
	}
	sprintf(buf + strlen(buf), "%s\r\n%d players listed.\r\n", columns % 2 ? "\r\n" : "", counter);
	page_string(ch->desc, buf, true);

	release_buffer(buf);
}


ACMD(do_clanwho) {
	Descriptor *d;
	Character *		wch;
	VNum			clan;
	UInt32			players = 0;
	char			*buf;

	buf = get_buffer(MAX_STRING_LENGTH);
	
	one_argument(argument, buf);

	clan = IS_STAFF(ch) ? atoi(buf) : GET_CLAN(ch);
	
	if (!Clan::Find(clan) || ((GET_CLAN_LEVEL(ch) < Clan::Rank::Member) && !IS_STAFF(ch))) {
		if (IS_STAFF(ch))	ch->Send("Clan does not exist.\r\n");
		else				ch->Send("You aren't even in a clan!\r\n");
		release_buffer(buf);
		return;
	}
	
	strcpy(buf,	"Clan Members currently online\r\n"
				"-----------------------------\r\n");
	
	LListIterator<Descriptor *>		iter(descriptor_list);
	while ((d = iter.Next())) {
		if (!(wch = d->Original()))							continue;
		if ((STATE(d) != CON_PLAYING) || !wch->CanBeSeen(ch))	continue;
		 
		if ((GET_CLAN(wch) != Clan::Clans[clan].vnum) || (GET_CLAN_LEVEL(wch) < Clan::Rank::Member))
			continue;
		
		if (GET_CLAN_LEVEL(wch) > Clan::Rank::Leader)
			continue;
		
		sprintf(buf + strlen(buf), "&c0[%3d] %s %s&c0", GET_LEVEL(wch),
				Clan::Clans[clan].ranks[GET_CLAN_LEVEL(wch) - 1],
				wch->Name());
		players++;
		
		if (PLR_FLAGGED(wch, PLR_MAILING))			strcat(buf, " (&cbmailing&c0)");
		else if (PLR_FLAGGED(wch, PLR_WRITING))		strcat(buf, " (&crwriting&c0)");

		if (CHN_FLAGGED(wch, Channel::NoShout))			strcat(buf, " (&cgdeaf&c0)");
		if (CHN_FLAGGED(wch, Channel::NoTell))			strcat(buf, " (&cgnotell&c0)");
		if (CHN_FLAGGED(wch, Channel::Quest))			strcat(buf, " (&cmquest&c0)");
		// if (PLR_FLAGGED(wch, PLR_TRAITOR))			strcat(buf, " (&cRTRAITOR&c0)");
		if (GET_AFK(wch))							strcat(buf, " &cc[AFK]&c0");
		strcat(buf, "\r\n");
	}
	
	if (!players)	strcat(buf, "\r\nNo clan members are currently visible to you.\r\n");
	else			sprintf(buf + strlen(buf), "\r\nThere %s %d visible clan member%s.\r\n", (players == 1 ? "is" : "are"), players, (players == 1 ? "" : "s"));
	
	page_string(ch->desc, buf, true);
	
	release_buffer(buf);
}


ACMD(do_apply) {
	char *		arg = get_buffer(MAX_INPUT_LENGTH);
	VNum		clan;

	one_argument(argument, arg);
	
	if (IS_NPC(ch) || IS_STAFF(ch))							ch->Send("You can't join a clan you dolt!\r\n");
	else if (Clan::Find(GET_CLAN(ch)) && (GET_CLAN_LEVEL(ch) > Clan::Rank::Applicant))
		ch->Send("You are already a member of a clan!\r\n");
	else if (!*arg || !Clan::Find(clan = atoi(arg)))		ch->Send("Apply to WHAT clan?\r\n");
	else {
		GET_CLAN(ch) = Clan::Clans[clan].vnum;
		GET_CLAN_LEVEL(ch) = Clan::Rank::Applicant;
		ch->Sendf("You have applied to \"%s\".\r\n", Clan::Clans[clan].name);
	}
	release_buffer(arg);
}


ACMD(do_resign) {
	VNum		clan;
	
	clan = GET_CLAN(ch);
	
	if (IS_NPC(ch))											ch->Send("Very funny.\r\n");
	else if (!Clan::Find(clan))								ch->Send("You aren't even in a clan!\r\n");
	else {
		const char *	clanName = Clan::Clans[clan].name;
		if (GET_CLAN_LEVEL(ch) > Clan::Rank::Applicant) {
			if (!Clan::Clans[clan].RemoveMember(GET_IDNUM(ch)))
				mudlogf(BRF, NULL, TRUE, "SYSERR: CLAN: %s's attempt to resign from clan \"%s\" failed.",
						ch->RealName(), clanName);
			ch->Sendf("You resign from \"%s\".\r\n", clanName);
		} else
			ch->Sendf("You remove your application to \"%s\".\r\n", clanName);
		GET_CLAN(ch) = 0;
		GET_CLAN_LEVEL(ch) = 0;
	}
}


ACMD(do_enlist) {
	char *arg = get_buffer(MAX_INPUT_LENGTH);
	Character *applicant;
	VNum	clan;
	
	one_argument(argument, arg);
	
	clan = GET_CLAN(ch);
	
	if (!Clan::Find(clan) || (GET_CLAN_LEVEL(ch) < Clan::Rank::Member))	ch->Send("You aren't even in a clan!\r\n");
	else if (GET_CLAN_LEVEL(ch) < Clan::Rank::Commander)	ch->Send("You are of insufficient clan rank to do that.\r\n");
	else if (!*arg)											ch->Send("Enlist who?\r\n");
	else if (!(applicant = get_player_vis(ch, arg, false)))	ch->Send(NOPERSON);
	else if (GET_CLAN(applicant) != GET_CLAN(ch))			ch->Send("They aren't applying to this clan!\r\n");
	else if (GET_CLAN_LEVEL(applicant) > Clan::Rank::Applicant)	ch->Send("They are already in your clan!\r\n");
	else {
		if (!Clan::Clans[clan].AddMember(GET_IDNUM(applicant))) {
			ch->Send("Problem adding player to clan, contact Staff.\r\n");
			mudlogf(BRF, NULL, TRUE, "SYSERR: CLAN: %s's attempt to enlist %s to clan \"%s\" failed.",
					ch->RealName(), applicant->RealName(), Clan::Clans[clan].name);
		}
		act("You have been accepted into $t by $n!", TRUE, ch, (Object *)Clan::Clans[clan].name, applicant, TO_VICT);
		act("You have enlisted $N into $t!", TRUE, ch, (Object *)Clan::Clans[clan].name, applicant, TO_CHAR);
		GET_CLAN_LEVEL(applicant) = Clan::Rank::Member;
	}
	
	release_buffer(arg);
}


ACMD(do_clanpromote) {
	char *		arg = get_buffer(MAX_INPUT_LENGTH);
	Character *	victim;
	VNum		clan;
	SInt32		highestRank;
	
	one_argument(argument, arg);
	
	clan = GET_CLAN(ch);
	
	highestRank = (GET_CLAN_LEVEL(ch) == Clan::Rank::Leader) ?
					Clan::Rank::Commander : (Clan::Rank::Commander - 1);
	
	if (!Clan::Find(clan) || (GET_CLAN_LEVEL(ch) < Clan::Rank::Member))	ch->Send("You aren't even in a clan!\r\n");
	else if (GET_CLAN_LEVEL(ch) < Clan::Rank::Commander)	ch->Send("You are of insufficient clan rank to do that.\r\n");
	else if (!*arg)											ch->Send("Promote who?\r\n");
	else if (!(victim = get_player_vis(ch, arg, false)))	ch->Send(NOPERSON);
	else if (GET_CLAN(victim) != GET_CLAN(ch))				ch->Send("They aren't in your clan!\r\n");
	else if (ch == victim)									ch->Send("Promote yourself?  Dumbass.\r\n");
	else if (GET_CLAN_LEVEL(victim) < Clan::Rank::Member || GET_CLAN_LEVEL(victim) >= highestRank)
		ch->Sendf("You can only promote clan members up to %s.\r\n", Clan::Clans[clan].ranks[highestRank - 1]);
	else {
		GET_CLAN_LEVEL(victim)++;
		victim->Save(victim->AbsoluteRoom());
		act("You have been promoted to $t by $n!", TRUE, ch, (Object *)Clan::Clans[clan].ranks[GET_CLAN_LEVEL(victim) - 1],
				victim, TO_VICT);
		act("You have promoted $N to $t!", TRUE, ch, (Object *)Clan::Clans[clan].ranks[GET_CLAN_LEVEL(victim) - 1],
				victim, TO_CHAR);
	}
	
	release_buffer(arg);
}


ACMD(do_clandemote) {
	char *		arg = get_buffer(MAX_INPUT_LENGTH);
	Character *	victim;
	VNum		clan;
	SInt32		topRank;
	
	one_argument(argument, arg);
	
	clan = GET_CLAN(ch);
	topRank = (GET_CLAN_LEVEL(ch) == Clan::Rank::Leader) ?
					Clan::Rank::Leader : Clan::Rank::Commander;
	
	if (!Clan::Find(clan) || (GET_CLAN_LEVEL(ch) < Clan::Rank::Member))	ch->Send("You aren't even in a clan!\r\n");
	else if (GET_CLAN_LEVEL(ch) < Clan::Rank::Commander)	ch->Send("You are of insufficient clan rank to do that.\r\n");
	else if (!*arg)											ch->Send("Demote who?\r\n");
	else if (!(victim = get_player_vis(ch, arg, false)))	ch->Send(NOPERSON);
	else if (GET_CLAN(victim) != GET_CLAN(ch))				ch->Send("They aren't in your clan!\r\n");
	else if (ch == victim)									ch->Send("Demote yourself?  Dumbass.\r\n");
//	else if (GET_CLAN_LEVEL(victim) > Clan::Rank::Commander)	ch->Send("You can only demote clan members.\r\n");
	else if (GET_CLAN_LEVEL(victim) < Clan::Rank::Member || (GET_CLAN_LEVEL(victim) >= GET_CLAN_LEVEL(ch)))
		ch->Sendf("You can only demote clan members of a lower rank than you down to %s.\r\n",
				Clan::Clans[clan].ranks[Clan::Rank::Member - 1]);
	else {
		GET_CLAN_LEVEL(victim)--;
		victim->Save(victim->AbsoluteRoom());
		act("You have been promoted to a clan Commander!", TRUE, 0, 0, victim, TO_VICT);
		act("You have promoted $N to a clan Commander!", TRUE, ch, 0, victim, TO_CHAR);
	}
	
	release_buffer(arg);
}


ACMD(do_forceenlist) {
	char *		arg = get_buffer(MAX_INPUT_LENGTH);
	Character *	applicant;
	VNum		clan;
	
	one_argument(argument, arg);
	
	if (!*arg)
		ch->Send("Force-Enlist who?\r\n");
	else if (!(applicant = get_player_vis(ch, arg, false)))
		ch->Send(NOPERSON);
	else if (!Clan::Find(clan = GET_CLAN(applicant)))
		ch->Send("They aren't applying to a clan.\r\n");
	else if (GET_CLAN_LEVEL(applicant) > Clan::Rank::Applicant)
		ch->Send("They are already a member of a clan.\r\n");
	else {
		if (!Clan::Clans[clan].AddMember(GET_IDNUM(applicant))) {
			ch->Send("Problem adding player to clan.");
			mudlogf(BRF, ch, TRUE, "SYSERR: CLAN: %s's attempt to force-enlist %s to clan \"%s\" failed.",
					ch->RealName(), applicant->RealName(), Clan::Clans[clan].name);
//		} else {
		}
//			applicant->Sendf("You have been accepted into %s!", Clans[clan].name);
//			ch->Sendf("You have force-enlisted %s into %s", applicant->RealName(), Clans[clan].name);
			act("You have been accepted into $t!", TRUE, 0, (Object *)Clan::Clans[clan].name, applicant, TO_VICT);
			act("You have force-enlisted $N into $t!", TRUE, ch, (Object *)Clan::Clans[clan].name, applicant, TO_CHAR);
			GET_CLAN_LEVEL(applicant) = (GET_IDNUM(applicant) == Clan::Clans[clan].owner) ?
					Clan::Rank::Leader : Clan::Rank::Member;
//		}
	}
	release_buffer(arg);
}


ACMD(do_boot) {
	char *		arg = get_buffer(MAX_INPUT_LENGTH);
	Character *	member;
	VNum		clan;
	SInt32		id;
	
	one_argument(argument, arg);
	
	if (!Clan::Find(clan = GET_CLAN(ch)))
		ch->Send("You aren't even in a clan!\r\n");
	else if (GET_CLAN_LEVEL(ch) < Clan::Rank::Commander)
		ch->Send("You are of insufficient clan rank to do that.\r\n");
	else if (!*arg)
		ch->Send("Enlist who?\r\n");
	else if ((member = get_player_vis(ch, arg, false))) {
		if (GET_CLAN(ch) != GET_CLAN(member))
			act("$N isn't in your clan!", TRUE, ch, 0, member, TO_CHAR);
//			ch->Sendf("%s isn't in your clan!\r\n", member->RealName());
		else if (GET_CLAN_LEVEL(ch) <= GET_CLAN_LEVEL(member))
			act("$N is not below you in rank!", TRUE, ch, 0, member, TO_CHAR);
//			ch->Sendf("%s is not below you in clan rank!\r\n", member->RealName());
		else {
			if (!Clan::Clans[clan].RemoveMember(GET_IDNUM(member))) {
				ch->Send("Problem booting player from clan, contact Sr. Staff.");
				mudlogf(BRF, NULL, TRUE, "SYSERR: CLAN: %s's attempt to boot %s from clan \"%s\" failed.",
						ch->RealName(), member->RealName(), Clan::Clans[clan].name);
			} else {
				member->Sendf("You have been booted from %s by %s!", Clan::Clans[clan].name, ch->RealName());
				ch->Sendf("You have booted %s from %s!", member->RealName(), Clan::Clans[clan].name);
				GET_CLAN(member) = 0;
				GET_CLAN_LEVEL(member) = 0;
			}
		}
	} else if ((id = get_id_by_name(arg))) {
		if (Clan::Clans[clan].IsMember(id)) {
			if (!Clan::Clans[clan].RemoveMember(id)) {
				ch->Send("Problem booting player from clan, contact Sr. Staff.");
				mudlogf(BRF, NULL, TRUE, "SYSERR: CLAN: %s's attempt to boot %s from clan \"%s\" failed.",
						ch->RealName(), member->RealName(), Clan::Clans[clan].name);
			}
			ch->Sendf("You have booted %s from %s!", CAP(arg), Clan::Clans[clan].name);
		} else 
			ch->Send("They are not a member of your clan!");
	} else
		ch->Send(NOPERSON);
	
	release_buffer(arg);
}


ACMD(do_clandeposit) {
	char *	arg = get_buffer(MAX_INPUT_LENGTH);
	SInt32	amount;
	VNum	clan;
	
	one_argument(argument, arg);
	
	if (!Clan::Find(clan = GET_CLAN(ch)))
		ch->Send("You aren't even in a clan!\r\n");
	else if (!*arg || !is_number(arg))
		ch->Send("Deposit how much?\r\n");
	else if ((amount = atoi(arg)) <= 0)
		ch->Send("Deposit something more than nothing, please.\r\n");
	else if (amount > GET_GOLD(ch))
		ch->Send("You don't have that much gold!\r\n");
	else {
		FILE *		fl;
		time_t		ct;
		const char *tmp;
		
		ct = time(0);
		tmp = asctime(localtime(&ct));

		sprintf(arg, CLAN_PREFIX "%d.txt", GET_CLAN(ch));
		if (!(fl = fopen(arg, "ac"))) {
			perror("do_deposit");
			ch->Send("Could not log deposit, so unable to complete transaction.\r\n");
		} else {
			fprintf(fl, "%-12s (%6.6s) Deposited %d gold pieces.\r\n", ch->Name(), (tmp + 4), amount);
			fclose(fl);
			GET_GOLD(ch) -= amount;
			Clan::Clans[clan].savings += amount;
			ch->Sendf("You deposit %d gold pieces into clan savings.\r\n", amount);
		}
	}
	release_buffer(arg);
}


/*
ACMD(do_withdraw) {
	char *	arg = get_buffer(MAX_INPUT_LENGTH);
	UInt32	amount;
	RNum	clan;
	
	one_argument(argument, arg);
	
	if ((clan = real_clan(GET_CLAN(ch))) == -1)
		ch->Send("You aren't even in a clan!\r\n");
	else if (!*arg || !is_number(arg))
		ch->Send("Withdraw how much?\r\n");
	else if ((amount = atoi(arg)) <= 0)
		please.\r\n", ch->Send("Withdraw something more than nothing);
	else if (amount > Clans[clan].savings)
		ch->Send("You're clan doesn't have that many mission points!\r\n");
	else {
		GET_MISSION_POINTS(ch) += amount;
		Clans[clan].savings -= amount;
		ch->Sendf("You withdraw %d mission points from clan savings.\r\n", amount);
	}
}
*/


void Clan::Save(void) {
	FILE *		fl;
	char *		buf;
	
	if (!(fl = fopen(CLAN_FILE, "w"))) {
		mudlogf(NRM, NULL, TRUE, "SYSERR: Cannot write clans to disk!");
		return;
	}
	
	buf = get_buffer(MAX_STRING_LENGTH);
	
	MapIterator<VNum, Clan>	iter(Clans);
	Clan *	clan;
	while ((clan = iter.Next())) {
		strcpy(buf, clan->description ? clan->description : "Undefined\r\n");
		strip_string(buf);
		
		fprintf(fl,	"#%d\n"
					"%s~\n"
					"%s~\n"
					"%d %d %d\n",
				clan->vnum,
				clan->name ? clan->name : "Undefined",
				buf,
				clan->owner, clan->room, clan->savings);
		
		for (SInt32 x = 0; x < NumRanks - 1; x++)
			fprintf(fl, "%s~\n", clan->ranks[x]);
		
		for (SInt32 member = 0; member < clan->members.Count(); member++)
			fprintf(fl, "%d\n", clan->members[member]);
		fprintf(fl, "~\n");
	}
	fputs("$~\n", fl);
	fclose(fl);
	
	release_buffer(buf);
	
	olc_remove_from_save_list(CEDIT_PERMISSION, OLC_CEDIT);
}


void Clan::Load(void) {
	FILE *		fl;
	char *		filename = CLAN_FILE;
	char *		line = get_buffer(MAX_INPUT_LENGTH);
	SInt32		clan_count,
				nr = -1,
				last = 0;
	
	
	if (!(fl = fopen(filename, "r"))) {
		log("Clan file %s not found.", filename);
		tar_restore_file(filename);
		exit(1);
	}
	
	clan_count = count_hash_records(fl);
	rewind(fl);
	
	for (;;) {
		if (!get_line(fl, line)) {
			log("Format error after clan #%d", nr);
			tar_restore_file(filename);
			exit(1);
		}
		
		if (*line == '$')
			break;
		else if (*line == '#') {
			last = nr;
			if (sscanf(line, "#%d", &nr) != 1) {
				log("Format error after clan #%d", nr);
				tar_restore_file(filename);
				exit(1);
			}
			Clan::Parse(fl, nr, filename);
		} else {
			log("Format error after clan #%d", nr);
			log("Offending line: '%s'", line);
			tar_restore_file(filename);
			exit(1);
		}
	}
	fclose(fl);
	release_buffer(line);
}


void Clan::Parse(FILE *fl, VNum virtual_nr, char *filename) {
	char 	*line = get_buffer(MAX_INPUT_LENGTH),
			*buf = get_buffer(MAX_INPUT_LENGTH),
			*members,
			*s;
	SInt32	t[3], member_num;
	
	Clan &	clan = Clan::Clans[virtual_nr];
			
	sprintf(buf, "Parsing clan #%d", virtual_nr);
	
	clan.vnum			= virtual_nr;
	clan.name			= fread_string(fl, buf, filename);
	clan.description	= fread_string(fl, buf, filename);
	if (!get_line(fl, line) || (sscanf(line, " %d %d %d", t, t + 1, t + 2) != 3)) {
		log("Format error in clan #%d", virtual_nr);
		tar_restore_file(filename);
		exit(1);
	}
	clan.owner		= t[0];
	clan.room		= t[1];
	clan.savings	= t[2];
	
	for (SInt32 x = 0; x < NumRanks - 1; x++)
		clan.ranks[x] = fread_string(fl, buf, filename);
	
	if ((members = fread_string(fl, buf, filename))) {
		for (s = strtok(members, " \r\n"); s; s = strtok(NULL, " \r\n")) {
			member_num = atoi(s);
			if (!scheck && !get_name_by_id(member_num))	// Make sure the characters still exist
				continue;
			clan.members.Add(member_num);
		}
		FREE(members);
	}
	
	release_buffer(line);
	release_buffer(buf);
}

/*
RNum Clan::Real(VNum vnum) {
	int bot, top, mid, nr, found = NOWHERE;
	
	//	First binary search
	bot = 0;
	top = Clan::Top;
	
	for (;;) {
		mid = (bot + top) >> 1;

		if (Clan::Clans[mid].vnum == vnum)	return mid;
		if (bot >= top)						break;
		if (Clan::Clans[mid].vnum > vnum)	top = mid - 1;
		else								bot = mid + 1;
	}
	
	//	If not found - use linear on the "new" parts.
	for(nr = 0; nr <= Clan::Top; nr++) {
		if(Clan::Clans[nr].vnum == vnum) {
			found = nr;
			break;
		}
	}
	return (found);
}
*/


struct clan_cmd_struct {
	const char *	cmd;
	ACMD(*command_pointer);
	Flags			staffcmd;
} clan_cmds[] = {
	//	Clan commands
	{ "apply"		, do_apply			, 0					},
	{ "resign"		, do_resign			, 0					},
	{ "enlist"		, do_enlist			, 0					},
	{ "forceenlist"	, do_forceenlist	, Staff::Clans		},
	{ "boot"		, do_boot			, 0					},
	{ "clans"		, do_clanlist		, 0					},
	{ "who"			, do_clanwho		, 0					},
	{ "promote"		, do_clanpromote	, 0					},
	{ "demote"		, do_clandemote		, 0					},
	{ "deposit"		, do_clandeposit	, 0					},
	{ "list"		, do_clanlist		, 0					},
	{ "stat"		, do_clanstat		, 0					},
	{ "members"		, do_members		, 0					},
	{ "\n"			, 0					, 0					}
};


ACMD(do_clan)
{
	int i = 0, displayed = 0;
	ACMD(*cptr) = NULL;

	half_chop(argument, buf1, buf2);
	
	if (*buf1) {
		for (i = 0; *clan_cmds[i].cmd != '\n'; ++i) {
			if ((!clan_cmds[i].staffcmd || (clan_cmds[i].staffcmd & STF_FLAGS(ch))) && 
				(is_abbrev(buf1, clan_cmds[i].cmd)))
				cptr = clan_cmds[i].command_pointer;
		}
	}
	
	if (!cptr) {
		ch->Send("Clan command list:\r\n");

		for (i = 0; *clan_cmds[i].cmd != '\n'; ++i) {
			if (clan_cmds[i].staffcmd && !(clan_cmds[i].staffcmd & STF_FLAGS(ch)))
				continue;
			++displayed;
			ch->Sendf("%-12s%s", clan_cmds[i].cmd, 
					(displayed && !(displayed % 6)) ? "\r\n" : "");
		}

		if (displayed % 6)		ch->Send("\r\n");
	} else {
		((*cptr) (ch, buf2, i, clan_cmds[i].cmd, 0));
	}
}
