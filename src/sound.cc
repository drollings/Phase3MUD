#include "descriptors.h"
#include "characters.h"
#include "rooms.h"
#include "buffer.h"
#include "comm.h"
#include "interpreter.h"
#include "utils.h"
#include "skills.h"
#include "handler.h"
#include "sound.h"

const char* midi_files[] = {
	 "None.",
	 "Off",
	 "\n"
};

void sound_to_char(Character *ch, char *filename, int volume, int repeats, int priority, char *type) 
{
	 if (!PRF_FLAGGED(ch, Preference::Sound)) 
		 return;

	 if(is_abbrev(filename, "off") && strlen(filename) == 3) {
		 ch->Send("!!SOUND(Off)\r");
		 return;
	 }

	 volume = MIN(100, MAX(1, volume));
	 repeats = MIN(10, MAX(1, repeats));
	 priority = MIN(100, MAX(1, priority));

	 sprintf(buf, "\r\n!!SOUND(sounds/wav/%s V=%d L=%d P=%d T=%s)\r", filename, 
				volume, repeats, priority, type);
	 ch->Send(buf);
}


void sound_to_room(int room, char *filename, int volume, int repeats, int priority, char *type) 
{
	Character *i = NULL;

	if (room == NOWHERE)
	    return;

	volume = MIN(100, MAX(1, volume));
	repeats = MIN(10, MAX(1, repeats));
	priority = MIN(100, MAX(1, priority));

	sprintf(buf, "\r\n!!SOUND(sounds/wav/%s V=%d L=%d P=%d T=%s)\r", filename, 
	    	   volume, repeats, priority, type);

	START_ITER(iter, i, Character *, world[IN_ROOM(i)].people)
		if ((i->desc) && PRF_FLAGGED(i, Preference::Sound))
			i->Send(buf);
}


void sound_to_outdoor(int realm, char *filename, int zone, int volume, int repeats, int priority, char *type) 
{
	Descriptor *i;
	
	volume = MIN(100, MAX(1, volume));
	repeats = MIN(10, MAX(1, repeats));
	priority = MIN(100, MAX(1, priority));

	if (filename) {
	    sprintf(buf, "\r\n!!SOUND(sounds/wav/%s V=%d L=%d P=%d T=%s)\r", filename, 
	    	   volume, repeats, priority, type);

	   // Added a for-loop ala send_to_outdoor.    Noticed a continue statement
	   // w/o a loop and assumed that this is what was wanted. :)  -DH
	   START_ITER(iter, i, Descriptor *, descriptor_list) {
	       if (!i->connected && i->character && AWAKE(i->character) && 
	    	   !PURGED(i->character) && IS_SOMEWHERE(i->character) &&
	    	   (world[IN_ROOM(i->character)].zone == zone) &&
	    	   OUTSIDE(i->character) && (realm == world[IN_ROOM(i->character)].Realm())) {
	    	   if (PRF_FLAGGED(i->character, Preference::Sound)) 
	    		   i->character->Send(buf);
	       }
	   }
	}
}


void sound_to_zone(char *filename, int zone, int volume, int repeats, int priority, char *type) 
{
	Descriptor *i;
 
	volume = MIN(100, MAX(1, volume));
	repeats = MIN(10, MAX(1, repeats));
	priority = MIN(100, MAX(1, priority));

	if (filename) {
	    sprintf(buf, "\r\n!!SOUND(sounds/wav/%s V=%d L=%d P=%d T=%s)\r", filename, 
	    	   volume, repeats, priority, type);

	    // Added a for-loop ala send_to_outdoor.   Noticed a continue statement
	    // w/o a loop and assumed that this is what was wanted. :) -DH
		START_ITER(iter, i, Descriptor *, descriptor_list) {
			if (!i->connected && i->character && AWAKE(i->character) &&
				!PURGED(i->character) && IS_SOMEWHERE(i->character) &&
				(world[IN_ROOM(i->character)].zone == zone)) {
				if (PRF_FLAGGED(i->character, Preference::Sound)) 
	    			i->character->Send(buf);
			}
		}
	}
}


void music_to_char(Character *ch, char *filename, int volume, int repeats, int cont, char *type) 
{
	 if (!PRF_FLAGGED(ch, Preference::Sound)) return;

	 if (is_abbrev(filename, "off") /*&& strlen(filename) == 3*/) {
		 ch->Send("\r\n!!MUSIC(Off)\r");
		 return;
	 }
	 
	 volume = MIN(100, MAX(1, volume));
	 repeats = MIN(10, MAX(1, repeats));
	 cont = MIN(0, MAX(1, cont));

	 sprintf(buf, "\r\n!!MUSIC(sounds/midi/%s V=%d L=%d C=%d T=%s)\r", filename, 
				volume, repeats, cont, type);
	 ch->Send(buf);
}
