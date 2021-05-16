#ifndef __SOUND_H__
#define __SOUND_H__

#include "characters.h"

extern void sound_to_char(Character *ch,
char* filename, int volume, int repeats, int priority, char* type);

extern void sound_to_room(int room,
char* filename, int volume, int repeats, int priority, char* type);

extern void sound_to_outdoor(int realm, char *filename, int zone, 
int volume, int repeats, int priority, char *type);

extern void sound_to_zone(char *filename, int zone, int volume, int repeats, int priority, char *type);

extern void music_to_char(Character *ch,
char* filename, int volume, int repeats, int cont, char* type);


#define MSP_FIGHT_PRIORITY   90
#define MSP_GRUNT_PRIORITY   90

#endif

