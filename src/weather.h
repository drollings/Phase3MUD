/***************************************************************************\
[*]    __     __  ___                ___  | LexiMUD is a C++ MUD codebase [*]
[*]   / /  ___\ \/ (_) /\/\  /\ /\  /   \ |        for Sci-Fi MUDs        [*]
[*]  / /  / _ \\  /| |/    \/ / \ \/ /\ / | Copyright(C) 1997-1999        [*]
[*] / /__|  __//  \| / /\/\ \ \_/ / /_//  | All rights reserved           [*]
[*] \____/\___/_/\_\_\/    \/\___/___,'   | Chris Jacobson (FearItself)   [*]
[*]      A Creation of the AvP Team!      | fear@technologist.com         [*]
[-]---------------------------------------+-------------------------------[-]
[*] File : weather.h                                                      [*]
[*] Usage: Weather system (based on Copper)                               [*]
\***************************************************************************/


#ifndef __WEATHER_H__
#define __WEATHER_H__


#include "types.h"
#include "internal.defs.h"


namespace Weather {

namespace Seasons {
	enum { One, TwoEqual, TwoFirstLong, TwoSecondLong, Three,
			FourEqual, FourEvenLong, FourOddLong };
}	//	namespace Seasons

const int MaxSeasons = 4;

namespace Wind {
	enum { Calm, Breezy, Unsettled, Windy, Chinook, Violent, Hurricane };
}	//	namespace Wind

namespace Precipitation {
	enum { None, Arid, Dry, Low, Average, High, Stormy, Torrent, Constant };
}	//	namespace Precipitation

namespace Temperature {
	enum { Arctic, SubFreezing, Freezing, Cold, Cool, Mild, Warm, Hot, Blustery,
			Heatstroke, Boiling };
}	//	namespace Temperature
	
enum {		//	Flags
	NoMoon			= (1 << 0),
	NoSun			= (1 << 1),
	Uncontrollable	= (1 << 2),
	AffectsIndoors	= (1 << 3)
};


class Climate {
public:
					Climate();
				
	SInt16			pattern,
					wind[MaxSeasons],
					windDirection[MaxSeasons],
					windVariance[MaxSeasons],
					precipitation[MaxSeasons],
					temperature[MaxSeasons];
	Flags			flags;
	SInt32			energy;
};


enum {
	MoonVisible		= (1 << 0),
	SunVisible		= (1 << 1),
	Controlled		= (1 << 2)
};


class Weather {
public:
					Weather();
					
	SInt8			temperature,	//	metric
					humidity;
	UInt8			windspeed;
	SInt8			windDirection;
	
	SInt8			precipRate,
					precipDepth,
					precipChange;

	SInt8			pressureChange;
	SInt32			pressure;

	SInt32			energy;
	Flags			flags;
	
	SInt8			light;
};


void Change(void);


extern const char *seasons[];
extern const char *winds[];
extern const char *precipitations[];
extern const char *temperatures[];
extern const char *climateflags[];
extern const char *weatherflags[];


}	//	namespace Weather


#endif	//	__WEATHER_H__

