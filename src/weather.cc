/* ************************************************************************
*   File: weather.c                                     Part of CircleMUD *
*  Usage: functions handling time and the weather                         *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */


#include "weather.h"


#include "descriptors.h"
#include "characters.h"
#include "rooms.h"
#include "utils.h"
#include "comm.h"
#include "db.h"

extern struct TimeInfoData time_info;
extern SInt32 top_of_zone_table;
extern struct zone_data *zone_table;

void weather_and_time(int mode);
void another_hour(int mode);


void send_to_zone_outdoor(int zone, const char *text);

void send_to_zone_outdoor(int zone, const char *text) {
	Descriptor *j;
	START_ITER(iter, j, Descriptor *, descriptor_list)
		if ((STATE(j) == CON_PLAYING) && j->character && AWAKE(j->character) && OUTSIDE(j->character) && 
				(world[IN_ROOM(j->character)].zone == zone))
			j->Write(text);
}


Weather::Weather::Weather(void) {
	memset(this, 0, sizeof(Weather));
}

Weather::Climate::Climate(void) {
	memset(this, 0, sizeof(Climate));
}


void weather_and_time(int mode) {
	another_hour(mode);
	if (mode)	Weather::Change();
}
					/*    J   F   M   A   M   J   J   A   S   O   N   D */
int days_in_month[12] = {30, 27, 30, 29, 30, 29, 30, 30, 29, 30, 29, 30};

void another_hour(int mode)
{
  int i;
  static char *time_msgs[Realm::MAX][4] = {
    { // Nyrilis
      "The sun rises in the east.\r\n",
      "The day has begun.\r\n",
      "The sun slowly disappears in the west.\r\n",
      "The night has begun.\r\n"
    }, { // Earth
      "The sun rises in the east.\r\n",
      "The day has begun.\r\n",
      "The sun slowly disappears in the west.\r\n",
      "The night has begun.\r\n"
    }, { // Olodris
      "",
      "",
      "",
      ""
    }, {  // Space
      "",
      "",
      "",
      ""
    }, {  // Astral
      "",
      "",
      "",
      ""
    }, {  // Shadow
      "",
      "",
      "",
      ""
    }, { // Ethereal
      "",
      "",
      "",
      ""
    }
  };

  time_info.hours++;

  for (i = 0; i < Realm::MAX; ++i) {
    if (mode) {
      switch (time_info.hours) {
      case 5:
      sunlight = SUN_RISE;
      send_to_outdoor(i, time_msgs[i][0]);
      break;
      case 6:
      sunlight = SUN_LIGHT;
      send_to_outdoor(i, time_msgs[i][1]);
      break;
      case 21:
      sunlight = SUN_SET;
      send_to_outdoor(i, time_msgs[i][2]);
      break;
      case 22:
      sunlight = SUN_DARK;
      send_to_outdoor(i, time_msgs[i][3]);
      break;
      default:
      break;
      }
    }
  }

  if (time_info.hours > 23) {	/* Changed by HHS due to bug ??? */
    time_info.hours -= 24;
    time_info.day++;

    if (time_info.day > 31) {
      time_info.day = 0;
      time_info.month++;

    if (time_info.month > 12) {
		time_info.month = 0;
		time_info.year++;
      }
    }
  }
}


namespace Weather {

const char *seasons[] = {
	"One",
	"TwoEqual",
	"TwoFirstLong",
	"TwoSecondLong",
	"Three",
	"FourEqual",
	"FourEvenLong",
	"FourOddLong",
	"\n"
};


const char *winds[] = {
	"Calm",
	"Breezy",
	"Unsettled",
	"Windy",
	"Chinook",
	"Violent",
	"Hurricane",
	"\n"
};
	
const char *precipitations[] = {
	"None",
	"Arid",
	"Dry",
	"Low",
	"Average",
	"High",
	"Stormy",
	"Torrent",
	"Constant",
	"\n"
};


const char *temperatures[] = {
	"Arctic",
	"Subfreezing",
	"Freezing",
	"Cold",
	"Cool",
	"Mild",
	"Warm",
	"Hot",
	"Blustery",
	"Heatstroke",
	"Boiling",
	"\n"
};


const char *climateflags[] = {
	"NoMoon",
	"NoSun",
	"Uncontrollable",
	"AffectsIndoors",
	"\n"
};


const char *weatherflags[] = {
	"MoonVisible",
	"SunVisible",
	"Controlled",
	"\n"
};


}	//	namespace Weather


namespace {
const int	precipStart = 1060,
			precipStop = 970;
}	//	namespace


void Weather::Change(void) {
	int		magic,
			oldWind;
	SInt8	oldTemp;
	UInt8	oldPrecip, season;
	Climate *climate;
	Weather *conditions;
	
	
	for (int zone = 0; zone <= top_of_zone_table; zone++) {
		climate = &zone_table[zone].climate;
		conditions = &zone_table[zone].conditions;
		oldTemp = conditions->temperature;
		oldPrecip = conditions->precipRate;
		oldWind = conditions->windspeed;
		
		season = zone_table[zone].GetSeason();		//	Find season
		
		REMOVE_BIT(conditions->flags, Controlled);	//	Clear controlled weather bit
		
		conditions->energy = RANGE(300, climate->energy + conditions->energy, 50000);
		
		switch (climate->wind[season]) {
			case Wind::Calm:		conditions->windspeed += (conditions->windspeed > 25) ? -5 : Number(-2, 1);	break;
			case Wind::Breezy:		conditions->windspeed += (conditions->windspeed > 40) ? -5 : Number(-2, 2);	break;
			case Wind::Unsettled:	conditions->windspeed += (conditions->windspeed < 5) ? 5 :
					(conditions->windspeed > 60) ? -5 : Number(-6, 6);		break;
			case Wind::Windy:		conditions->windspeed += (conditions->windspeed < 15) ? 5 :
					(conditions->windspeed > 80) ? -5 : Number(-6, 6);		break;
			case Wind::Chinook:		conditions->windspeed += (conditions->windspeed < 25) ? 5 :
					(conditions->windspeed > 110) ? -5 : Number(-15, 15);	break;
			case Wind::Violent:		conditions->windspeed += (conditions->windspeed < 40) ? 5 : Number(-9, 8);	break;
			case Wind::Hurricane:	conditions->windspeed = 100;			break;
		}
		
		conditions->energy += conditions->windspeed;	//	Add or subtract?
		
		if (conditions->energy < 0)				conditions->energy = 0;
		else if (conditions->energy > 20000)	conditions->windspeed += Number(-10, -1);
		
		conditions->windspeed = MAX((UInt8)0, conditions->windspeed);
		
		switch(climate->windVariance[season]) {
			case 0:	conditions->windDirection = climate->windDirection[season];	break;
			case 1:	if (dice(2, 15) * 1000 < conditions->energy)	conditions->windDirection = Number(0, 3);	break;
		}
//			conditions->windDirection = climate->windVariance[season] ?
//					((dice(2, 15) * 1000 < conditions->energy) ? Number(0, 3) : conditions->windDirection) :
//					climate->windDirection[season];
		
		switch (climate->temperature[season]) {
			case Temperature::Arctic:		conditions->temperature += (conditions->temperature > -20) ? -4 : Number(-3, 3);	break;
			case Temperature::SubFreezing:	conditions->temperature += (conditions->temperature < -40) ? 2 :
					conditions->temperature > 5 ? -3 : Number(-3, 3);		break;
			case Temperature::Freezing:	conditions->temperature += (conditions->temperature < -20) ? 2 :
					conditions->temperature > 0 ? -2 : Number(-2, 2);		break;
			case Temperature::Cold:			conditions->temperature += (conditions->temperature < -10) ? 1 :
					conditions->temperature > 5 ? -2 : Number(-2, 2);		break;
			case Temperature::Cool:			conditions->temperature += (conditions->temperature < -3) ? 2 :
					conditions->temperature > 14 ? -2 : Number(-3, 3);		break;
			case Temperature::Mild:			conditions->temperature += (conditions->temperature < 7) ? 2 :
					conditions->temperature > 26 ? -2 : Number(-2, 2);		break;
			case Temperature::Warm:			conditions->temperature += (conditions->temperature < 19) ? 2 :
					conditions->temperature > 33 ? -2 : Number(-3, 3);		break;
			case Temperature::Hot:			conditions->temperature += (conditions->temperature < 24) ? 3 :
					conditions->temperature > 46 ? -2 : Number(-3, 3);		break;
			case Temperature::Blustery:		conditions->temperature += (conditions->temperature < 44) ? 5 :
					conditions->temperature > 60 ? -5 : Number(-3, 3);		break;
			case Temperature::Boiling:		conditions->temperature += (conditions->temperature < 80) ? 5 :
					conditions->temperature > 120 ? -5 : Number(-6, 6);		break;
		}
		
		if (IS_SET(conditions->flags, SunVisible))	conditions->temperature += 2;
		else if (!IS_SET(climate->flags, NoSun))	conditions->temperature -= 2;
		
		switch (climate->precipitation[season]) {
			case Precipitation::None:
				if (conditions->precipRate > 0)	conditions->precipRate /= 2;
				conditions->humidity = 0;
				break;
			case Precipitation::Arid:
				conditions->humidity += (conditions->humidity > 30)	? - 3 : Number(-3, 2);
				if (oldPrecip > 20)	conditions->precipRate -= 8;
				break;
			case Precipitation::Dry:
				conditions->humidity += (conditions->humidity > 50)	? - 3 : Number(-4, 3);
				if (oldPrecip > 35)	conditions->precipRate -= 6;
				break;
			case Precipitation::Low:
				conditions->humidity += (conditions->humidity < 13) ? 3 : (conditions->humidity > 91) ? -2 :
						Number(-5, 4);
				if (oldPrecip > 45)	conditions->precipRate -= 10;
				break;
			case Precipitation::Average:
				conditions->humidity += (conditions->humidity < 30) ? 3 : (conditions->humidity > 80) ? -2 :
						Number(-9, 9);
				if (oldPrecip > 55)	conditions->precipRate -= 10;
				break;
			case Precipitation::High:
				conditions->humidity += (conditions->humidity < 40) ? 3 : (conditions->humidity > 90) ? -2 :
						Number(-8, 8);
				if (oldPrecip > 65)	conditions->precipRate -= 10;
				break;
			case Precipitation::Stormy:
				conditions->humidity += (conditions->humidity < 50) ? 4 : Number(-6, 6);
				if (oldPrecip > 80)	conditions->precipRate -= 10;
				break;
			case Precipitation::Torrent:
				conditions->humidity += (conditions->humidity < 60) ? 4 : Number(-6, 9);
				if (oldPrecip > 100)	conditions->precipRate -= 15;
				break;
			case Precipitation::Constant:
				conditions->humidity = 100;
				if (conditions->precipRate == 0)	conditions->precipRate = Number(5, 12);
				break;
		}
		
		conditions->humidity = RANGE((SInt8)0, conditions->humidity, (SInt8)100);
		
		conditions->pressureChange = RANGE(-8, conditions->pressureChange + Number(-3, 3), 8);
		conditions->pressure = RANGE(960, conditions->pressure + conditions->pressureChange, 1040);
		
		conditions->energy + conditions->pressureChange;

		/* The numbers that follow are truly magic since  */
		/* they have little bearing on reality and are an */
		/* attempt to create a mix of precipitation which */
		/* will seem reasonable for a specified climate   */
		/* without any complicated formulae that could    */
		/* cause a performance hit. To get more specific  */
		/* or exacting would certainly not be "Diku..."   */
		
		magic = ((1240 - conditions->pressure) * conditions->humidity / 16) +
				conditions->temperature + (oldPrecip * 2) + ((conditions->energy - 10000)/100);
			
#if 0
		char	buf[256];
		sprintf(buf, "Windspeed: %d  Temperature: %d  Precipitation: %d\r\n",
				conditions->windspeed, conditions->temperature, conditions->precipRate);
		send_to_zone_outdoor(zone, buf);
#endif

		if (oldPrecip == 0) {
			if (magic > precipStart) {
				conditions->precipRate += 1;
				send_to_zone_outdoor(zone, (conditions->temperature > 0) ? "It begins to rain.\r\n" :
						"It starts to snow.\r\n");
			} else if (!oldWind && conditions->windspeed)
				send_to_zone_outdoor(zone, "The wind begins to blow.\r\n");
			else if ((conditions->windspeed - oldWind > 10))
				send_to_zone_outdoor(zone, "The wind picks up some.\r\n");
			else if ((conditions->windspeed - oldWind < -10))
				send_to_zone_outdoor(zone, "The wind calms down a bit.\r\n");
			else if (conditions->windspeed > 60) {
				if (conditions->temperature > 50)
					send_to_zone_outdoor(zone, "A violent scorching wind blows hard in your face.\r\n");
				else if (conditions->temperature > 21)
					send_to_zone_outdoor(zone, "A hot wind gusts wildly through the area.\r\n");
				else if (conditions->temperature > 0)
					send_to_zone_outdoor(zone, "A fierce wind cuts the air like a razor-sharp knife.\r\n");
				else if (conditions->temperature > -10)
					send_to_zone_outdoor(zone, "A freezing gale blasts through the area.\r\n");
				else
					send_to_zone_outdoor(zone, "An icy wind drains the warmth from all in sight.\r\n");
			} else if (conditions->windspeed > 25) {
				if (conditions->temperature > 50)
					send_to_zone_outdoor(zone, "A hot, dry breeze blows languidly around.\r\n");
				else if (conditions->temperature > 22)
					send_to_zone_outdoor(zone, "A warm pocket of air is rolling through here.\r\n");
				else if (conditions->temperature > 2)
					send_to_zone_outdoor(zone, "A cool breeze wafts by.\r\n");
				else if (conditions->temperature > -5)
					send_to_zone_outdoor(zone, "A freezing wind blows gently but firmly against you.\r\n");
				else
					send_to_zone_outdoor(zone, "The wind isn't very strong here, but the cold makes it quite noticeable.\r\n");
			} else if (conditions->temperature > 52)
				send_to_zone_outdoor(zone, "It's hotter than anyone could imagine.\r\n");
			else if (conditions->temperature > 37)
				send_to_zone_outdoor(zone, "It's really, really hot here.  A slight breeze would really improve things.\r\n");
			else if (conditions->temperature > 25)
				send_to_zone_outdoor(zone, "It's hot out here.\r\n");
			else if (conditions->temperature > 19)
				send_to_zone_outdoor(zone, "It's nice and warm out.\r\n");
			else if (conditions->temperature > 9)
				send_to_zone_outdoor(zone, "It's quite mild out right now.\r\n");
			else if (conditions->temperature > 1)
				send_to_zone_outdoor(zone, "It's cool out here.\r\n");
			else if (conditions->temperature > -5)
				send_to_zone_outdoor(zone, "It's a bit nippy here.\r\n");
			else if (conditions->temperature > -20)
				send_to_zone_outdoor(zone, "It's cold!\r\n");
			else if (conditions->temperature > -25)
				send_to_zone_outdoor(zone, "It's really cold!\r\n");
			else
				send_to_zone_outdoor(zone, "Better get inside - this is too cold for you!\r\n");
		} else if (magic < precipStop) {
			conditions->precipRate = 0;
			if (oldTemp > 0)	send_to_zone_outdoor(zone, "The rain stops.\r\n");
			else				send_to_zone_outdoor(zone, "It stops snowing.\r\n");
		} else {
			//	Stil precipitating, update the rain
			//	Check rain->snow or snow->rain changes
			
			conditions->precipChange += (conditions->energy > 10000) ? Number(-3, 4) : Number(-4, 2);
			conditions->precipChange = RANGE((SInt8)-10, conditions->precipChange, (SInt8)10);
			conditions->precipRate = RANGE(1, conditions->precipRate + conditions->precipChange, 100);
			
			conditions->energy -= conditions->precipRate * 3 - abs(conditions->precipChange);
			
			if (oldTemp > 0 && conditions->temperature <= 0)
				send_to_zone_outdoor(zone, "The rain turns to snow.\r\n");
			else if (oldTemp <= 0 && conditions->temperature > 0)
				send_to_zone_outdoor(zone, "The snow turns to a cold rain.\r\n");
			else if (conditions->precipChange > 5) {
				if (conditions->temperature > 0)	send_to_zone_outdoor(zone, "It rains a bit harder.\r\n");
				else								send_to_zone_outdoor(zone, "The snow is coming down faster now.\r\n");
			} else if (conditions->precipChange < -5) {
				if (conditions->temperature > 0)	send_to_zone_outdoor(zone, "The rain is falling less heavily now.\r\n");
				else								send_to_zone_outdoor(zone, "The snow has let up a little.\r\n");
			} else if (conditions->temperature > 0) {
				if (conditions->precipRate > 80) {
					if (conditions->windspeed > 80)
						send_to_zone_outdoor(zone, "There's a hurricane out here!\r\n");
					else if (conditions->windspeed > 40)
						send_to_zone_outdoor(zone, "The wind and rain are nearly too much to handle.\r\n");
					else
						send_to_zone_outdoor(zone, "It's raining really hard right now.\r\n");
				} else if (conditions->precipRate > 50) {
					if (conditions->windspeed > 60)
						send_to_zone_outdoor(zone, "What a rainstorm!\r\n");
					else if (conditions->windspeed > 30)
						send_to_zone_outdoor(zone, "The wind is lashing this wild rain straight into your face.\r\n");
					else
						send_to_zone_outdoor(zone, "It's raining pretty hard.\r\n");
				} else if (conditions->precipRate > 30) {
					if (conditions->windspeed > 50)
						send_to_zone_outdoor(zone, "A respectable rain is being thrashed about by a vicious wind.\r\n");
					else if (conditions->windspeed > 25)
						send_to_zone_outdoor(zone, "It's rainy and windy but, altogether not too uncomfortable.\r\n");
					else
						send_to_zone_outdoor(zone, "It's raining.\r\n");
				} else if (conditions->precipRate > 10) {
					if (conditions->windspeed > 50)
						send_to_zone_outdoor(zone, "The light rain here is nearly unnoticeable compared to the horrendous wind.\r\n");
					else if (conditions->windspeed > 24)
						send_to_zone_outdoor(zone, "A light rain is being driven fiercely by the wind.\r\n");
					else
						send_to_zone_outdoor(zone, "It's raining lightly.\r\n");
				} else if (conditions->windspeed > 50)
					send_to_zone_outdoor(zone, "A few drops of rain are falling amidst a fierce windstorm.\r\n");
				else if (conditions->windspeed > 30)
					send_to_zone_outdoor(zone, "The wind and a bit of rain hint at the possibility of a storm.\r\n");
				else
					send_to_zone_outdoor(zone, "A light drizzle is falling.\r\n");
			} else {
				if (conditions->precipRate > 40) {	//	snow
					if (conditions->windspeed > 60)
						send_to_zone_outdoor(zone, "The heavily falling snow is being whipped up to a frenzy by a ferocious wind.\r\n");
					else if (conditions->windspeed > 35)
						send_to_zone_outdoor(zone, "A heavy snow is being blown randomly about by a brisk wind.\r\n");
					else if (conditions->windspeed > 18)
						send_to_zone_outdoor(zone, "Drifts in the snow are being formed by the wind.\r\n");
					else
						send_to_zone_outdoor(zone, "The snow's coming down pretty fast now.\r\n");
				} else if (conditions->precipRate > 19) {
					if (conditions->windspeed > 70)
						send_to_zone_outdoor(zone, "The snow wouldn't be too bad, except for the awful wind blowing it everywhere.\r\n");
					else if (conditions->windspeed > 45)
						send_to_zone_outdoor(zone, "There's a minor blizzard, more wind than snow.\r\n");
					else if (conditions->windspeed > 12)
						send_to_zone_outdoor(zone, "Snow is being blown about by a stiff breeze.\r\n");
					else
						send_to_zone_outdoor(zone, "It is snowing.\r\n");
				} else if (conditions->windspeed > 60)
					send_to_zone_outdoor(zone, "A light snow is being tossed about by a fierce wind.\r\n");
				else if (conditions->windspeed > 42)
					send_to_zone_outdoor(zone, "A lightly falling snow is being driven by a strong wind.\r\n");
				else if (conditions->windspeed > 12)
					send_to_zone_outdoor(zone, "A light snow is falling admist an unsettled wind.\r\n");
				else
					send_to_zone_outdoor(zone, "It is lightly snowing.\r\n");
			}
		}
		
		//	Handle celestial objects
		if (!IS_SET(climate->flags, NoSun)) {
			if (time_info.hours < 6 || time_info.hours > 18 || conditions->humidity > 90 || conditions->precipRate > 80)
				REMOVE_BIT(conditions->flags, SunVisible);
			else
				SET_BIT(conditions->flags, SunVisible);
		}
		if (!IS_SET(climate->flags, NoMoon)) {
			if (time_info.hours > 5 || time_info.hours < 19 || conditions->humidity > 80 || conditions->precipRate > 70 ||
					time_info.day < 3 || time_info.day > 31)
				REMOVE_BIT(conditions->flags, MoonVisible);
			else if (!IS_SET(conditions->flags, MoonVisible)) {
				SET_BIT(conditions->flags, MoonVisible);
				if (time_info.day == 17)	send_to_zone_outdoor(zone, "The full moon floods the area with light.\r\n");
				else						send_to_zone_outdoor(zone, "The moon casts a little bit of light on the ground.\r\n");
			}
		}
		zone_table[zone].CalcLight();
	}	//	End zone iteration
	//	Blow out lights?
	
#if 0	
	Descriptor *j;
	char *buf = "<ERROR>";

	for (i = 0; i <= top_of_zone_table; i++) {
		if ((time_info.month >= 7) && (time_info.month <= 11))
			diff = (zone_table[i].pressure > 985 ? -2 : 2);
		else
			diff = (zone_table[i].pressure > 1015 ? -2 : 2);

		zone_table[i].change += (dice(1, 4) * diff + dice(2, 6) - dice(2, 6));
		zone_table[i].change = RANGE(-12, zone_table[i].change, 12);

		zone_table[i].pressure += zone_table[i].change;
		zone_table[i].pressure = RANGE(960, zone_table[i].pressure, 1040);

		change = 0;

		switch (zone_table[i].sky) {
			case SKY_CLOUDLESS:
				if (zone_table[i].pressure < 990)			change = 1;
				else if (zone_table[i].pressure < 1010)
					if (dice(1, 4) == 1)					change = 1;
				break;
			case SKY_CLOUDY:
				if (zone_table[i].pressure < 970)			change = 2;
				else if (zone_table[i].pressure < 990) {
					if (dice(1, 4) == 1)					change = 2;
				} else if (zone_table[i].pressure > 1030)
					if (dice(1, 4) == 1)					change = 3;
				break;
			case SKY_RAINING:
				if (zone_table[i].pressure < 970) {
					if (dice(1, 4) == 1)					change = 4;
				} else if (zone_table[i].pressure > 1030)	change = 5;
				else if (zone_table[i].pressure > 1010)
					if (dice(1, 4) == 1)					change = 5;
				break;
			case SKY_LIGHTNING:
				if (zone_table[i].pressure > 1010)			change = 6;
				else if (zone_table[i].pressure > 990)
					if (dice(1, 4) == 1)					change = 6;
				break;
			default:
				change = 0;
				zone_table[i].sky = SKY_CLOUDLESS;
				break;
		}

		switch (change) {
			case 1:
				buf = "The sky starts to get cloudy.\r\n";
				zone_table[i].sky = SKY_CLOUDY;
				break;
			case 2:
				buf = "It starts to rain.\r\n";
				zone_table[i].sky = SKY_RAINING;
				break;
			case 3:
				buf = "The clouds disappear.\r\n";
				zone_table[i].sky = SKY_CLOUDLESS;
				break;
			case 4:
				buf = "Lightning starts to show in the sky.\r\n";
				zone_table[i].sky = SKY_LIGHTNING;
				break;
			case 5:
				buf = "The rain stops.\r\n";
				zone_table[i].sky = SKY_CLOUDY;
				break;
			case 6:
				buf = "The lightning stops.\r\n";
				zone_table[i].sky = SKY_RAINING;
				break;
			default:
				break;
		}
		if ((change >= 1) && (change <= 6)) {
			START_ITER(iter, j, Descriptor *, descriptor_list)
				if ((STATE(j) == CON_PLAYING) && j->character && AWAKE(j->character) && OUTSIDE(j->character) && 
						(zone_table[world[IN_ROOM(j->character)].zone].number == i))
					j->Write("%s", buf);
		}
	}
#endif
}


void zone_data::CalcLight(void) {
	char	lightSum = 0,
			temp1, temp2;
	
	if (!IS_SET(climate.flags, Weather::NoSun)) {
		temp1 = time_info.hours;
		if (temp1 > 11)	temp1 = 23 - temp1;
		temp1 -= 5;
		if (temp1 > 0) {
			SET_BIT(conditions.flags, Weather::SunVisible);
			lightSum += temp1 * 10;
		} else
			REMOVE_BIT(conditions.flags, Weather::SunVisible);
	}
	
	if (!IS_SET(climate.flags, Weather::NoMoon)) {
		temp1 = abs(time_info.hours - 12) - 7;
		if (temp1 > 0) {
			temp2 = 17 - abs(time_info.day - 17) - 3;
			if (temp2 > 0) {
				SET_BIT(conditions.flags, Weather::MoonVisible);
				lightSum += temp1 * temp2 / 2;
			} else
				REMOVE_BIT(conditions.flags, Weather::MoonVisible);
		}
	}
	
	conditions.light = RANGE(0, lightSum - conditions.precipRate, 100); 
}


char zone_data::GetSeason() {
	char	season = 0;
	
	switch (climate.pattern) {
		case Weather::Seasons::One:				season = 0;									break;
		case Weather::Seasons::TwoEqual:		season = time_info.month / 6;				break;
		case Weather::Seasons::TwoFirstLong:	season = (time_info.month < 8) ? 0 : 1;		break;
		case Weather::Seasons::TwoSecondLong:	season = (time_info.month < 4) ? 0 : 1;		break;
		case Weather::Seasons::Three:			season = (time_info.month < 4) ? 0 : (time_info.month < 8) ? 1 : 2;
		case Weather::Seasons::FourEqual:		season = time_info.month / 3;				break;
		case Weather::Seasons::FourEvenLong:	season = (time_info.month < 2) ? 0 : (time_info.month < 6) ? 1 :
													 (time_info.month < 8) ? 2 : 3;		break;
		case Weather::Seasons::FourOddLong:		season = (time_info.month < 4) ? 0 : (time_info.month < 6) ? 1 :
													 (time_info.month < 10) ? 2 : 3;	break;
		default:
			log("SYSERR: Zone %d (%s): Bad season (%d) in zone_data::GetSeason.",
					number, name, climate.pattern);
			break;
	}
	return season;
}


