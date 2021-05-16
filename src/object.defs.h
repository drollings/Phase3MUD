
#define NOTHING			-1    /* nil reference for objects		*/

/* Item types: used by obj_data.obj_flags.type_flag */
#define ITEM_LIGHT      1		/* Item is a light source	*/
#define ITEM_SCROLL     2		/* Item is a scroll		*/
#define ITEM_WAND       3		/* Item is a wand		*/
#define ITEM_STAFF      4		/* Item is a staff		*/
#define ITEM_WEAPON     5		/* Item is a weapon		*/
#define ITEM_FIREWEAPON 6		/* Item is a fire weapon	*/
#define ITEM_MISSILE    7		/* Item is a missile		*/
#define ITEM_TREASURE   8		/* Item is a treasure, not gold	*/
#define ITEM_ARMOR      9		/* Item is armor		*/
#define ITEM_POTION    10 		/* Item is a potion		*/
#define ITEM_REAGENT   11		/* Unimplemented		*/
#define ITEM_TALISMAN  12		/* Unimplemented		*/
#define ITEM_SPELLBOOK 13		/* Unimplemented		*/
#define ITEM_TRAP      14		/* Unimplemented		*/
#define ITEM_CONTAINER 15		/* Item is a container		*/
#define ITEM_NOTE      16		/* Item is note 		*/
#define ITEM_DRINKCON  17		/* Item is a drink container	*/
#define ITEM_KEY       18		/* Item is a key		*/
#define ITEM_FOOD      19		/* Item is food			*/
#define ITEM_MONEY     20		/* Item is money (gold)		*/
#define ITEM_PEN       21		/* Item is a pen		*/
#define ITEM_BOAT      22		/* Item is a boat		*/
#define ITEM_FOUNTAIN  23		/* Item is a fountain		*/
#define ITEM_GRENADE   24		/* Item is a grenade		*/
#define ITEM_WEAPONCON 25		/* Item is a scabbard or quiver	*/
#define ITEM_ATTACHMENT	26
#define ITEM_BOARD		27		/* Misc object			*/
#define ITEM_FURNITURE	28
#define ITEM_VEHICLE	29
#define ITEM_V_HATCH	30
#define ITEM_V_CONTROLS	31
#define ITEM_V_WINDOW	32
// #define ITEM_V_WEAPON	33
#define NUM_ITEM_TYPES	33

/* Take/Wear flags: used by obj_data.obj_flags.wear_flags */
#define ITEM_WEAR_TAKE		(1 << 0)  /* Item can be taken		*/
#define ITEM_WEAR_FINGER	(1 << 1)  /* Can be worn on finger	*/
#define ITEM_WEAR_NECK		(1 << 2)  /* Can be worn around neck 	*/
#define ITEM_WEAR_BODY		(1 << 3)  /* Can be worn on body 	*/
#define ITEM_WEAR_HEAD		(1 << 4)  /* Can be worn on head 	*/
#define ITEM_WEAR_LEGS		(1 << 5)  /* Can be worn on legs	*/
#define ITEM_WEAR_FEET		(1 << 6)  /* Can be worn on feet	*/
#define ITEM_WEAR_HANDS		(1 << 7)  /* Can be worn on hands	*/
#define ITEM_WEAR_ARMS		(1 << 8)  /* Can be worn on arms	*/
#define ITEM_WEAR_SHIELD	(1 << 9)  /* Can be used as a shield	*/
#define ITEM_WEAR_ABOUT		(1 << 10) /* Can be worn about body 	*/
#define ITEM_WEAR_WAIST 	(1 << 11) /* Can be worn around waist 	*/
#define ITEM_WEAR_WRIST		(1 << 12) /* Can be worn on wrist 	*/
#define ITEM_WEAR_WIELD		(1 << 13) /* Can be wielded		*/
#define ITEM_WEAR_HOLD		(1 << 14) /* Can be held		*/
#define ITEM_WEAR_BACK 		(1 << 15) /* Can be worn on back    	*/
#define ITEM_WEAR_ONBELT	(1 << 16) /* Can be worn from waist   	*/
#define ITEM_WEAR_FACE		(1 << 17) /* Can be worn on face 	*/
#define ITEM_WEAR_EYES		(1 << 18) /* Can be worn on eyes 	*/
#define ITEM_WEAR_EAR		(1 << 19) /* Can be worn on ears 	*/
#define ITEM_WEAR_SHOULDER	(1 << 20) /* Can be worn over shoulder	*/
#define ITEM_WEAR_SYMBIOT	(1 << 21) 
#define ITEM_WEAR_TERRAN_HARDPOINT	(1 << 22) 
#define ITEM_WEAR_TRULOR_HARDPOINT	(1 << 23) 


/* Extra object flags: used by obj_data.obj_flags.extra_flags */
#define ITEM_HUM				(1 << 0)	// Item is humming
#define ITEM_NORENT				(1 << 1)	// Item cannot be rented
#define ITEM_NODONATE			(1 << 2)	// Item cannot be donated
#define ITEM_NOINVIS			(1 << 3)	// Item cannot be made invis
#define ITEM_INVISIBLE			(1 << 4)	// Item is invisible
#define ITEM_MAGIC				(1 << 5)	// Item is magical
#define ITEM_NODROP				(1 << 6)	// Item is cursed: can't drop
#define ITEM_BLESS				(1 << 7)	// Item is blessed
#define ITEM_ANTI_GOOD			(1 << 8)	// 
#define ITEM_ANTI_EVIL			(1 << 9)	// 
#define ITEM_ANTI_NEUTRAL		(1 << 10)	// 
#define ITEM_NOSELL				(1 << 11)	// Shopkeepers won't touch it
#define ITEM_NO_PURGE			(1 << 12)	// Must specifically purge
#define ITEM_TWO_HAND			(1 << 13)	// Requires two hands to hold
#define ITEM_HIDDEN				(1 << 14)	// Item is hidden in the room
#define ITEM_PROXIMITY			(1 << 15)	// Item is destroyed upon leaving character
#define ITEM_GLOW				(1 << 16)	// Item is glowing
#define ITEM_MOVEABLE			(1 << 17)
#define ITEM_NOLOSE				(1 << 18)

#define ITEM_APPROVED			(1 << 30) /* Item has been APPROVED for auto-loading */
#define ITEM_DELETED			(1 << 31) /* Item has been DELETED */

/* Container flags - value[1] */
#define CONT_CLOSEABLE      (1 << 0)	/* Container can be closed	*/
#define CONT_PICKPROOF      (1 << 1)	/* Container is pickproof	*/
#define CONT_CLOSED         (1 << 2)	/* Container is closed		*/
#define CONT_LOCKED         (1 << 3)	/* Container is locked		*/


/* Some different kind of liquids for use in values of drink containers */
#define LIQ_WATER      0
#define LIQ_BEER       1
#define LIQ_WINE       2
#define LIQ_ALE        3
#define LIQ_DARKALE    4
#define LIQ_WHISKY     5
#define LIQ_LEMONADE   6
#define LIQ_FIREBRT    7
#define LIQ_LOCALSPC   8
#define LIQ_SLIME      9
#define LIQ_MILK       10
#define LIQ_TEA        11
#define LIQ_COFFE      12
#define LIQ_BLOOD      13
#define LIQ_SALTWATER  14
#define LIQ_CLEARWATER 15

#define MAX_OBJ_AFFECT		6	/* Used in obj_file_elem *DO*NOT*CHANGE* */

#define NUM_OBJ_VALUES		8

#define IS_CONTAINER(obj)	((GET_OBJ_TYPE(obj) == ITEM_CONTAINER) || \
							(GET_OBJ_TYPE(obj) == ITEM_WEAPONCON))

