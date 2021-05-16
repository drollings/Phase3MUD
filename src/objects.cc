/***************************************************************************\
[*]    __     __  ___                ___  | LexiMUD is a C++ MUD codebase [*]
[*]   / /  ___\ \/ (_) /\/\  /\ /\  /   \ |        for Sci-Fi MUDs        [*]
[*]  / /  / _ \\  /| |/    \/ / \ \/ /\ / | Copyright(C) 1997-1999        [*]
[*] / /__|  __//  \| / /\/\ \ \_/ / /_//  | All rights reserved           [*]
[*] \____/\___/_/\_\_\/    \/\___/___,'   | Chris Jacobson (FearItself)   [*]
[*]      A Creation of the AvP Team!      | fear@te;chnologist.com         [*]
[-]---------------------------------------+-------------------------------[-]
[*] File : objects.c++                                                    [*]
[*] Usage: Objects code                                                   [*]
\***************************************************************************/


#include "types.h"

#include "objects.h"
#include "rooms.h"
#include "characters.h"
#include "utils.h"
#include "scripts.h"
#include "comm.h"
#include "buffer.h"
#include "files.h"
#include "event.h"
#include "affects.h"
#include "db.h"
#include "extradesc.h"
#include "handler.h"
#include "find.h"
#include "interpreter.h"
#include "constants.h"
#include "eventdefines.h"

// External function declarations
void tag_argument(char *argument, char *tag);
void strip_string(char *buffer);


LList<Object *>	Objects;		//	global linked list of objs
LList<Object *>	PurgedObjs;		//	list of objs to be deleted
Index<Object>	Object::Index;	//	index table for object file


#ifdef GOT_RID_OF_IT
GunData::GunData(void) : attack(0), rate(0), rank(1), range(0), closeRange(0), longRange(0) {
	ammo.type = 0;
	ammo.vnum = -1;
	ammo.amount = 0;
}


GunData::GunData(const GunData &data) : attack(data.attack), rate(data.rate), rank(data.rank),
		range(data.range), closeRange(data.closeRange), longRange(data.longRange) {
	ammo = data.ammo;
}


GunData::~GunData(void) {
}
#endif


Object::Object(void) : MUDObject(-1), /* number(NOTHING), */ 
		actionDesc(NULL), 
		worn_on(-1), cost(0), weight(0), timer(0), type(0),
		wear(0), extra(0) {
	for (int i = 0; i < 8; ++i)
		value[i] = 0;
	ID(max_id++);
}


Object::Object(const Object &proto) : MUDObject(proto), /* number(proto.number), */
		actionDesc(SSShare(proto.actionDesc)),
		worn_on(-1), cost(proto.cost), weight(proto.weight), 
		timer(proto.timer), type(proto.type), wear(proto.wear), extra(proto.extra) {
	ExtraDesc *	ed;
	SInt32		i;
	Object		*newObj, *contentObj;
	
	for (i = 0; i < 8; i++)
		value[i] = proto.value[i];
	
	for (i = 0; i < MAX_OBJ_AFFECT; i++)
		affectmod[i] = proto.affectmod[i];
	
	//	Share GUN data
	// if (proto.gun)	gun = new GunData(*proto.gun);
	
	// Duplicate extra descriptions
	LListIterator<ExtraDesc *>	descIter(proto.exDesc);
	while ((ed = descIter.Next())) {
		exDesc.Append(ed->Share());
	}
	
	//	This doesn't happen on protos - only on copied real objects
	START_ITER(iter, contentObj, Object *, proto.contents) {
		newObj = new Object(*contentObj);
//		newObj->ToObj(this);				This messes up weight...
		contents.Append(newObj);	//	Safer, and we can add to end of list
		newObj->inside = this;			//	Plus no need for weight correction
	}
	
	ID(max_id++);
}


/* Delete the object */
Object::~Object(void) {
	ExtraDesc *ed;

	while ((ed = exDesc.Top())) {
		exDesc.Remove(ed);
		ed->Free();
	}
	SSFree(actionDesc);

	// if (gun);		delete gun;
}


/* Extract an object from the world */
void Object::Extract(void) {
	Object *	temp;
	Character *	ch;
	Affect *tmp_affect;
	
	if (Purged())
		return;
	
	if (WornBy() != NULL)
		if (unequip_char(WornBy(), worn_on) != this)
			log("SYSERR: Inconsistent WornBy() and worn_on pointers!!");

	if (InRoom() != NOWHERE)	FromRoom();
	else if (CarriedBy())		FromChar();
	else if (InObj())			FromObj();

	/* Get rid of the contents of the object, as well. */
//	while (contains)
//		contains->extract();
	while ((temp = contents.Top())) {
		contents.Remove(temp);
		temp->Extract();
	}

	if (InRoom() != NOWHERE && IS_SITTABLE(this))
		while ((ch = get_char_on_obj(this)))
			ch->SetMount(NULL);

	if (script) {
		script->Extract();
		script = NULL;
	}
	
	START_ITER(iter, tmp_affect, Affect *, affected) {
		delete tmp_affect;
		affected.Remove(tmp_affect);
	}

	/* remove any pending event for/from this object. */
//	clean_events(this);
	
	EventCleanup();

	if (Find(Virtual()))
		Index[Virtual()].number--;
	
	Objects.Remove(this);
	PurgedObjs.Add(this);
	
	Purged(true);
}


/* give an object to a char   */
void Object::ToChar(Character * ch) {
	if (!ch)
		log("SYSERR: %s->Object::ToChar(ch == NULL)", Name());
	else if (Purged())
		log("SYSERR: %s->Object::ToChar(ch == %s): purged == true", Name(), ch->RealName());
	else if (ch->Purged())
		log("SYSERR: %s->Object::ToChar(ch == %s): ch->purged == true", Name(), ch->RealName());
	else {
		ch->contents.Add(this);
		inside = ch;
		
		IS_CARRYING_W(ch) += GET_OBJ_WEIGHT(this);
		IS_CARRYING_N(ch)++;

		if (!IS_NPC(ch))			SET_BIT(PLR_FLAGS(ch), PLR_CRASH);
	}
}


/* take an object from a char */
void Object::FromChar() {
	Character * carried_by = CarriedBy();
	if (!carried_by)
		log("SYSERR: %s->Object::FromChar(): carried_by == NULL", Name());
	else if (Purged())
		log("SYSERR: %s->Object::FromChar(): purged == true", Name());
	else {
		if (!IS_NPC(carried_by))	SET_BIT(PLR_FLAGS(carried_by), PLR_CRASH);

		IS_CARRYING_W(carried_by) -= GET_OBJ_WEIGHT(this);
		IS_CARRYING_N(carried_by)--;
		
		carried_by->contents.Remove(this);
		inside = NULL;
	}
}


/* put an object in a room */
void Object::ToRoom(VNum newRoom) {
	if (newRoom < 0 || newRoom > world.Top())
		log("SYSERR: %s->Object::ToRoom(newRoom == %d): world.Top() == %d", Name(), newRoom,
				world.Top());
	else if (InRoom() != -1)
		log("SYSERR: %s->Object::ToRoom(newRoom == %d): room == %d [%s]", Name(), newRoom,
				InRoom(), world[InRoom()].Name());
	else if (Purged())
		log("SYSERR: %s->Object::ToRoom(newRoom == %d): purged == true", Name(), newRoom);
	else {
		world[newRoom].contents.Add(this);
		SetRoom(newRoom);
		
		if (ROOM_FLAGGED(newRoom, ROOM_HOUSE))
			SET_BIT(ROOM_FLAGS(newRoom), ROOM_HOUSE_CRASH);

		if (ROOM_FLAGGED(room, ROOM_GRAVITY) && !AFF_FLAGGED(this, AffectBit::Fly) && 
			!Event::Find(events, typeid(FallingEvent))) {
//			add_event(3, gravity, this, NULL, NULL);
			new FallingEvent(3 RL_SEC, this);
		}
	}
}


/* Take an object from a room */
void Object::FromRoom(void) {
	VNum	theRoom;

	if ((theRoom = InRoom()) == NOWHERE)
		log("SYSERR: %s->Object::FromRoom(): room == -1", Name());
	else if (Purged())
		log("SYSERR: %s->Object::FromRoom(): purged = true", Name());
	else {
		Character *	tch;
		LListIterator<Character *>	people(world[theRoom].people);
		while ((tch = people.Next()))
			if (tch->SittingOn() == this)
				tch->SetMount(NULL);
	
		world[theRoom].contents.Remove(this);
		SetRoom(NOWHERE);

		if (ROOM_FLAGGED(theRoom, ROOM_HOUSE))
			SET_BIT(ROOM_FLAGS(theRoom), ROOM_HOUSE_CRASH);
	}
}


/* put an object in an object (quaint)  */
void Object::ToObj(Object* obj_to) {
	Object *tmp_obj, *in_obj;

	if (!obj_to || this == obj_to) {
		log("SYSERR: NULL object (%p) or same source and target obj passed to obj_to_obj", obj_to);
		return;
	}
	
	if (PURGED(this))
		return;
	
	obj_to->contents.Add(this);
	inside = obj_to;
	in_obj = (Object *) inside;

	for (tmp_obj = in_obj; tmp_obj->InObj(); tmp_obj = tmp_obj->InObj())
		GET_OBJ_WEIGHT(tmp_obj) += GET_OBJ_WEIGHT(this);

	/* top level object.  Subtract weight from inventory if necessary. */
	GET_OBJ_WEIGHT(tmp_obj) += GET_OBJ_WEIGHT(this);
	if (tmp_obj->CarriedBy())
		IS_CARRYING_W(tmp_obj->CarriedBy()) += GET_OBJ_WEIGHT(this);
	else if (tmp_obj->WornBy())
		IS_CARRYING_W(tmp_obj->WornBy()) += GET_OBJ_WEIGHT(this);
	
	if ((GET_OBJ_TYPE(this) == ITEM_ATTACHMENT) && (!IS_CONTAINER(obj_to))) {
		// DO THE ATTACHMENT
	}
}


/* remove an object from an object */
void Object::FromObj() {
	Object *temp, *obj_from, *in_obj;
	in_obj = InObj();

	if (in_obj == NULL) {
		log("SYSERR: trying to illegally extract obj from obj");
		return;
	}
	if (PURGED(this))
		return;
		
	obj_from = InObj();
	obj_from->contents.Remove(this);

	/* Subtract weight from containers container */
	for (temp = obj_from; temp->InObj(); temp = temp->InObj())
		GET_OBJ_WEIGHT(temp) -= GET_OBJ_WEIGHT(this);

	/* Subtract weight from char that carries the object */
	GET_OBJ_WEIGHT(temp) -= GET_OBJ_WEIGHT(this);
	if (temp->CarriedBy())
		IS_CARRYING_W(temp->CarriedBy()) -= GET_OBJ_WEIGHT(this);
	else if (temp->WornBy())
		IS_CARRYING_W(temp->WornBy()) -= GET_OBJ_WEIGHT(this);

	inside = NULL;

	if ((GET_OBJ_TYPE(this) == ITEM_ATTACHMENT) && (!IS_CONTAINER(in_obj))) {
		// DO THE DETACHMENT
	}
}


/*
void Object::unequip() {


}

void Object::equip(Character *ch) {

}
*/


void Object::Update(UInt32 use) {
	Object *	obj;
	if (!SCRIPT_CHECK(this, OTRIG_TIMER) && GET_OBJ_TIMER(this) > 0)
			GET_OBJ_TIMER(this) -= use;
	LListIterator<Object *>	iter(contents);
	while ((obj = iter.Next())) {
		obj->Update(use);
	}
//	if (contains && contains != this)			contains->update(use);
//	if (next_content && next_content != this)	next_content->update(use);
}


SInt32 Object::TotalValue(void) {
	SInt32		value;
	Object *	obj;
	
	// value = OBJ_FLAGGED(this, ITEM_MISSION) ? cost : 0;
	value = cost;
	
	START_ITER(iter, obj, Object *, contents) {
		value += obj->TotalValue();
	}
	
	return value;
}


void Object::ExtractNoKeep(Object *corpse, Character *ch) {
	Object *obj;

	if (OBJ_FLAGGED(this, ITEM_PROXIMITY)) {
		Extract();
	} else {
		LListIterator<Object *>	iter(contents);
		while ((obj = iter.Next()))
			obj->ExtractNoKeep(corpse, ch);

		if (WornBy())			unequip_char(ch, worn_on);

		if (InObj())			FromObj();
		else if (CarriedBy())	FromChar();

		ToObj(corpse);
	}
}


bool Object::Load(FILE *fl, char *filename, Character *ch = NULL) {
	char *line, *tag, *buf;
	SInt32	num, t[3];
	UInt8	currentAffect = 0;
	bool	result = true;
//	ExtraDesc *new_descr;
	Trigger *trig;
	
	line = get_buffer(MAX_INPUT_LENGTH);
	tag = get_buffer(6);
	buf = get_buffer(MAX_INPUT_LENGTH);

	sprintf(buf, "plr obj file %s", filename);

	while(get_line(fl, line)) {
		if (*line == '$')	break;	// Done reading object
		tag_argument(line, tag);
		num = atoi(line);

		if (!strcmp(tag, "Call")) {
			if (ch && !IS_NPC(ch)) {
				ch->player->CalledNames[ID()].Set(ID(), line);
			}
		} else if (!strcmp(tag, "Actn")) {
			SSFree(actionDesc);
			actionDesc = SString::fread(fl, buf, filename);
		} else if (!strcmp(tag, "Aff ")) {
			sscanf(line, "%d %d %d", t, t + 1, t + 2);
			affectmod[t[0]].location = (Affect::Location) t[1];
			affectmod[t[0]].modifier = t[2];
		} else if (!strcmp(tag, "Affs"))	affectbits = asciiflag_conv(line);
		else if (!strcmp(tag, "Cost"))	cost = num;
		else if (!strcmp(tag, "SDes")) SDesc(line);
		else if (!strcmp(tag, "Name")) Name(line);
		else if (!strcmp(tag, "Keyw")) Keywords(line);
		else if (!strcmp(tag, "Time"))		timer = num;
		else if (!strcmp(tag, "Trig")) {
			if ((trig = Trigger::Read(num))) {
				if (!SCRIPT(this))	SCRIPT(this) = new Script(Datatypes::Object);
				add_trigger(SCRIPT(this), trig); //, 0);
			}
		} else if (!strcmp(tag, "Type"))	type = num;
		else if (!strcmp(tag, "XDsc")) {
//			new_descr = new ExtraDesc();
//			new_descr->keyword = SString::Create(line);
//			new_descr->description = SSfread(fl, buf, filename);
//			new_descr->next = exDesc;
//			exDesc = new_descr;
		} else if (!strcmp(tag, "Val0"))	value[0] = num;
		else if (!strcmp(tag, "Val1"))		value[1] = num;
		else if (!strcmp(tag, "Val2"))		value[2] = num;
		else if (!strcmp(tag, "Val3"))		value[3] = num;
		else if (!strcmp(tag, "Val4"))		value[4] = num;
		else if (!strcmp(tag, "Val5"))		value[5] = num;
		else if (!strcmp(tag, "Val6"))		value[6] = num;
		else if (!strcmp(tag, "Val7"))		value[7] = num;
		else if (!strcmp(tag, "Wate"))		weight = num;
		else if (!strcmp(tag, "Wear"))		wear = asciiflag_conv(line);
		else if (!strcmp(tag, "Xtra"))		extra = asciiflag_conv(line);
		else {
			log("SYSERR: PlrObj file %s has unknown tag %s (rest of line: %s)", filename, tag, line);
			result = false;
		}
	}
	release_buffer(line);
	release_buffer(tag);
	release_buffer(buf);
	return result;
}


#define NOT_SAME(field)		((Virtual() == -1) || (field != Object::Index[Virtual()].proto->field))

void Object::Save(FILE *fl, SInt32 location, Character *ch = NULL) {
	char *	buf = get_buffer(MAX_STRING_LENGTH);
//	ExtraDesc *extradesc, *extradesc2;
//	bool	found;
//	UInt32	x;
	if (!Object::Find(Virtual()))	Virtual(-1);
	
	fprintf(fl, "#%d %d\n", Virtual(), location);
	if (ch && !IS_NPC(ch) && ch->player->CalledNames.Find(ID())) {
		fprintf(fl, "Call: %s\n", ch->player->CalledNames[ID()].Value());
	}
	if (NOT_SAME(SSKeywords()))			fprintf(fl, "Keyw: %s\n", Keywords());
	if (NOT_SAME(SSName()))	fprintf(fl, "Name: %s\n", Name());
	if (NOT_SAME(SSSDesc()))	fprintf(fl, "SDes: %s\n", SDesc());
	if (actionDesc && NOT_SAME(actionDesc))	{
		strcpy(buf, SSData(actionDesc));
		strip_string(buf);
		fprintf(fl, "Actn:\n%s~\n", buf);
	}
/*
	for (extradesc = exDesc; extradesc; extradesc = extradesc->next) {
		found = false;
		if (number > -1) {
			for (extradesc2 = ((Object *)obj_index[number].proto)->exDesc;
					extradesc2 && !found; extradesc2 = extradesc2->next) {
				if (!strcmp(SSData(extradesc->keyword), SSData(extradesc2->keyword)))
					if (!strcmp(SSData(extradesc->description
			}
		}
	}
*/
	if (NOT_SAME(value[0]))		fprintf(fl, "Val0: %d\n", value[0]);
	if (NOT_SAME(value[1]))		fprintf(fl, "Val1: %d\n", value[1]);
	if (NOT_SAME(value[2]))		fprintf(fl, "Val2: %d\n", value[2]);
	if (NOT_SAME(value[3]))		fprintf(fl, "Val3: %d\n", value[3]);
	if (NOT_SAME(value[4]))		fprintf(fl, "Val4: %d\n", value[4]);
	if (NOT_SAME(value[5]))		fprintf(fl, "Val5: %d\n", value[5]);
	if (NOT_SAME(value[6]))		fprintf(fl, "Val6: %d\n", value[6]);
	if (NOT_SAME(value[7]))		fprintf(fl, "Val7: %d\n", value[7]);
	
	if (NOT_SAME(cost))			fprintf(fl, "Cost: %d\n", cost);
	if (NOT_SAME(weight))		fprintf(fl, "Wate: %d\n", weight);
	if (NOT_SAME(timer))		fprintf(fl, "Time: %d\n", timer);
	if (NOT_SAME(type))			fprintf(fl, "Type: %d\n", type);
	if (NOT_SAME(wear)) {
		sprintbits(wear, buf);
		fprintf(fl, "Wear: %s\n", buf);
	}
	if (NOT_SAME(extra)) {
		sprintbits(extra, buf);
		fprintf(fl, "Xtra: %s\n", buf);
	}
	if (NOT_SAME(affectbits)) {
		sprintbits(affectbits, buf);
		fprintf(fl, "Affs: %s\n", buf);
	}
// 	if (NOT_SAME(speed))		fprintf(fl, "Sped: %d\n", speed);
// 	if (gun && type == ITEM_WEAPON) {
// 		if (NOT_SAME(gun->attack))		fprintf(fl, "GAtk: %d\n", gun->attack);
// 		if (NOT_SAME(gun->rate))		fprintf(fl, "GRat: %d\n", gun->rate);
// 		if (NOT_SAME(gun->range))		fprintf(fl, "GRng: %d\n", gun->range);
// 		if (NOT_SAME(gun->rank))		fprintf(fl, "GRnk: %d\n", gun->rank);
// 		if (NOT_SAME(gun->closeRange))	fprintf(fl, "GCRg: %d\n", gun->closeRange);
// 		if (NOT_SAME(gun->longRange))	fprintf(fl, "GLRg: %d\n", gun->longRange);
// 		if (NOT_SAME(gun->ammo.type))	fprintf(fl, "GATp: %d\n", gun->ammo.type);
// 		if (gun->ammo.amount)
// 			fprintf(fl, "Ammo: %d %d\n", gun->ammo.vnum, gun->ammo.amount);
// 	}
//	for (x = 0; x < MAX_OBJ_AFFECT; x++) {
//		if (NOT_SAME(affect[x].modifier))
//			fprintf(fl, "Aff : %d %d %d\n", x, affect[x].location, affect[x].modifier);
//	}
	
	fprintf(fl, "$\n");
	
	release_buffer(buf);
}


bool proper_wear(Character *ch, Object *obj, int pos)
{
	Flags mod_flags = obj->wear;
	
	REMOVE_BIT(mod_flags, ITEM_WEAR_TAKE);

	if (IS_SET(mod_flags, GET_WEARSLOT(pos, GET_BODYTYPE(ch)).wear_bitvector))
		return (TRUE);

	return FALSE;
}


void equip_char(Character * ch, Object * obj, UInt8 pos) {
	int j;

	if (pos > NUM_WEARS) {
		core_dump();
		return;
	}

	if (PURGED(obj))
		return;

	if (GET_EQ(ch, pos)) {
//		log("SYSERR: Char is already equipped: %s, %s", ch->RealName(), obj->Name());
		return;
	}
	if (obj->CarriedBy()) {
		log("SYSERR: EQUIP: Obj is CarriedBy() when equip.");
		return;
	}
	if (IN_ROOM(obj) != NOWHERE) {
		log("SYSERR: EQUIP: Obj is in_room when equip.");
		return;
	}

	if (!ch->CanUse(obj)) {
		act("You can't figure out how to use $p.", FALSE, ch, obj, 0, TO_CHAR);
		act("$n tries to use $p, but can't figure it out.", FALSE, ch, obj, 0, TO_ROOM);
		obj->ToChar(ch);	/* changed to drop in inventory instead of ground */
		return;
	}

	GET_EQ(ch, pos) = obj;
	obj->inside = ch;
	obj->worn_on = pos;
	IS_CARRYING_W(ch) += GET_OBJ_WEIGHT(obj);

	// if (GET_OBJ_TYPE(obj) == ITEM_ARMOR)
	// 	GET_AC(ch) += GET_OBJ_VAL(GET_EQ(ch, pos), 0);

	if (IN_ROOM(ch) != NOWHERE) {
		if (((pos == WEAR_HAND_R) || (pos == WEAR_HAND_L)) && GET_OBJ_TYPE(obj) == ITEM_LIGHT)
			if (GET_OBJ_VAL(obj, 2))	/* if light is ON */
				world[IN_ROOM(ch)].ChangeLight(1);
		if (IS_SET(obj->affectbits, AffectBit::Light) && !AFF_FLAGGED(ch, AffectBit::Light))
			world[IN_ROOM(ch)].ChangeLight(1);
	} else
		log("SYSERR: IN_ROOM(ch) = NOWHERE when equipping char %s.", ch->RealName());

	if (proper_wear(ch, obj, pos)) {
		for (j = 0; j < MAX_OBJ_AFFECT; j++)
			ch->AffectModify(obj->affectmod[j].location, obj->affectmod[j].modifier, obj->affectbits, true);
	}

	ch->AffectTotal();
}



Object *unequip_char(Character * ch, UInt8 pos) {
	int j;
	Object *obj;

	if ((pos >= NUM_WEARS) || !GET_EQ(ch, pos)) {
		core_dump();
		return NULL;
	}

//	if(pos > NUM_WEARS) {
//		log("SYSERR: pos passed to unequip_char is invalid.  Pos = %d", pos);
//		return Object::Read(0, REAL);
//	}
//	if (!GET_EQ(ch, pos)) {
//		log("SYSERR: no EQ at position %d on character %s", pos, ch->RealName());
//		return Object::Read(0, REAL);
//	}

	obj = GET_EQ(ch, pos);
	obj->inside = NULL;
	obj->worn_on = -1;
	IS_CARRYING_W(ch) -= GET_OBJ_WEIGHT(obj);

	// if (GET_OBJ_TYPE(obj) == ITEM_ARMOR)
	//	GET_AC(ch) -= GET_OBJ_VAL(GET_EQ(ch, pos), 0);

	if (IN_ROOM(ch) != NOWHERE) {
		if (((pos == WEAR_HAND_R) || (pos == WEAR_HAND_L)) && GET_OBJ_TYPE(obj) == ITEM_LIGHT)
			if (GET_OBJ_VAL(obj, 2))	/* if light is ON */
				world[IN_ROOM(ch)].ChangeLight(-1);
		if (IS_SET(obj->affectbits, AffectBit::Light))
			world[IN_ROOM(ch)].ChangeLight(-1);
	} else
		log("SYSERR: IN_ROOM(ch) = NOWHERE when unequipping char %s.", ch->RealName());

	GET_EQ(ch, pos) = NULL;

	if (proper_wear(ch, obj, pos)) {
		for (j = 0; j < MAX_OBJ_AFFECT; j++)
			ch->AffectModify(obj->affectmod[j].location, obj->affectmod[j].modifier,
					obj->affectbits, false);
	}


	ch->AffectTotal();
	
	if ((IN_ROOM(ch) != NOWHERE) && IS_SET(obj->affectbits, AffectBit::Light) && AFF_FLAGGED(ch, AffectBit::Light))
		world[IN_ROOM(ch)].ChangeLight(1);;
	
	return (obj);
}


/* create a new object from a prototype */
Object *Object::Read(VNum nr) {
	Object *obj;
	Trigger *trig;

	if (!Object::Find(nr)) {
		log("SYSERR: Object (V) %d does not exist in database.", nr);
		return NULL;
	}

	obj = new Object(*Index[nr].proto);
	Objects.Add(obj);

	Index[nr].number++;
	
	/* add triggers */
	for (int i = 0; i < Index[nr].triggers.Count(); i++) {
		VNum	vnum = Index[nr].triggers[i];
		if (!Trigger::Find(vnum))
			log("SYSERR: Invalid trigger %d assigned to object %d", vnum, Index[nr].vnum);
		else {
			trig = Trigger::Read(vnum);

			if (!SCRIPT(obj))
				SCRIPT(obj) = new Script(Datatypes::Object);
			add_trigger(SCRIPT(obj), trig); //, 0);
		}
	}

	return obj;
}

/*
RNum Object::Real(VNum vnum) {
	int bot, top, mid, nr, found = NOWHERE;

	//	First binary search.
	bot = 0;
	top = top_of_objt;

	for (;;) {
		mid = (bot + top) / 2;

		if ((obj_index + mid)->vnum == vnum)
			return mid;
		if (bot >= top)
			break;
		if ((obj_index + mid)->vnum > vnum)
			top = mid - 1;
		else
			bot = mid + 1;
	}

	//	If not found - use linear on the "new" parts.
	for(nr = 0; nr <= top_of_objt; nr++) {
		if(obj_index[nr].vnum == vnum) {
			found = nr;
			break;
		}
	}
	return(found);
}
*/

/* create an object, and add it to the object list */
Object *Object::Create(void) {
	Object *obj;

	obj = new Object;
	Objects.Add(obj);

	return obj;
}


// Character *	Object::TargetChar(const char *arg) const	{ return get_char_by_obj(this, arg);	}
// Object *	Object::TargetObj(const char *arg) const	{ return get_obj_by_obj(this, arg);		}

Room *		Object::TargetRoom(const char *arg) const {
	VNum		location = NOWHERE;
	Character *	targetMob;
	Object *	targetObj;
	
	if (is_number(arg) && !strchr(arg, '.'))		location = atoi(arg);
	else if ((targetMob = TargetChar(arg)))	location = IN_ROOM(targetMob);
	else if ((targetObj = TargetObj(arg)))	location = IN_ROOM(targetObj);

	return Room::Find(location) ? &(world[location]) : NULL;
}


/* returns the real room number that the object or object's carrier is in */
VNum Object::AbsoluteRoom(void) const {
	if (InRoom() != NOWHERE)	return InRoom();
	if (Inside())				return Inside()->AbsoluteRoom();
	else						return NOWHERE;
}


Character *	Object::CarriedBy(void) const { 
	if (inside && (worn_on < 0) && (Inside()->DataType() == Datatypes::Character))
		return (Character *) Inside();
	return NULL;
}

Character *	Object::WornBy(void) const { 
	if (inside && (worn_on >= 0) && (Inside()->DataType() == Datatypes::Character))
		return (Character *) Inside();
	return NULL;
}

Object *		Object::InObj(void) const { 
	if (inside && (worn_on < 0) && (Inside()->DataType() == Datatypes::Object))
		return (Object *) Inside();
	return NULL;
}


void fprintstring(FILE *fp, const char *fmt, const char *txt);
void fprintbits(FILE *fp, const char *fmt, UInt32 bits, const char *bitnames[]);

void Object::SaveDisk(Object *obj, FILE *fp) 
{
	char	*bitList = get_buffer(MAX_STRING_LENGTH),
			*bitList2 = get_buffer(MAX_STRING_LENGTH);

	fprintf(fp, "#%d = {\n", obj->Virtual());
	fprintstring(fp, "\tkeyw = %s;\n", obj->Keywords("undefined"));
	fprintstring(fp, "\tname = %s;\n", obj->Name("undefined"));
	fprintstring(fp, "\tshort = %s;\n", obj->SDesc("undefined"));
	if (obj->actionDesc->Data())
		fprintstring(fp, "\taction = %s;\n", obj->actionDesc->Data());
	
	if (obj->exDesc.Count()) {
		fprintf(fp, "\texdesc = {\n");
		LListIterator<ExtraDesc *>	descIter(obj->exDesc);
		ExtraDesc *	ex_desc;
		while ((ex_desc = descIter.Next())) {
			fprintstring(fp, "\t\t%s = ", SSData(ex_desc->keyword));
			fprintstring(fp, "%s;\n", SSData(ex_desc->description));	// CRASHPOINT, null pointer
		}
		fprintf(fp, "\t};\n");
	}
	
	fprintf(fp, "\ttype = \"%s\";\n", item_types[obj->type]);
	
	if (GET_OBJ_VAL(obj, 0) || GET_OBJ_VAL(obj, 1) || GET_OBJ_VAL(obj, 2) ||
			GET_OBJ_VAL(obj, 3) || GET_OBJ_VAL(obj, 4) || GET_OBJ_VAL(obj, 5) ||
			GET_OBJ_VAL(obj, 6) || GET_OBJ_VAL(obj, 7)) {
		fprintf(fp, "\tvalues = { ");
		
		bool	shouldContinue = true;
		for (SInt32 valCount = 0; shouldContinue && (valCount < 8); valCount++) {
			fprintf(fp, "%d ", GET_OBJ_VAL(obj, valCount));
			shouldContinue = false;
			for (SInt32 subCount = valCount + 1; subCount < 8; subCount++) {
				if (GET_OBJ_VAL(obj, subCount)) {
					shouldContinue = true;
					break;
				}
			}
		}
		
		fprintf(fp, "};\n");
	}
	
	if (obj->cost)		fprintf(fp, "\tcost = %d;\n", obj->cost);
	if (obj->weight)	fprintf(fp, "\tweight = %d;\n", obj->weight);
	if (obj->timer)		fprintf(fp, "\ttimer = %d;\n", obj->timer);
	if (obj->wear)		fprintbits(fp, "\twear = { %s };\n", obj->wear, wear_bits);
	if (obj->extra)		fprintbits(fp, "\textra = { %s };\n", obj->extra, extra_bits);
	if (obj->affectbits)	fprintbits(fp, "\taffects = { %s };\n", obj->affectbits, affected_bits);

	if (GET_PD(obj))	fprintf(fp, "\tpd = %d;\n", GET_PD(obj));
	if (GET_DR(obj))	fprintf(fp, "\tdr = %d;\n", GET_DR(obj));
	if (GET_OBJ_COST(obj))	fprintf(fp, "\tcost = %d;\n", GET_OBJ_COST(obj));
	if (GET_MATERIAL(obj))	fprintf(fp, "\tmaterial = \"%s\";\n", material_types[GET_MATERIAL(obj)]);
	
	bool	affectStarted = false;
	for (SInt32 applyCount = 0; applyCount < MAX_OBJ_AFFECT; applyCount++) {
		if (obj->affectmod[applyCount].location) {
			if (!affectStarted) {
				fprintf(fp, "\tapplies = {\n");
				affectStarted = true;
			}
			fprintf(fp, "\t\t%s = %d;\n", apply_types[obj->affectmod[applyCount].location],
					obj->affectmod[applyCount].modifier);
		}
		
	}
	if (affectStarted)
		fprintf(fp, "\t};\n");
	
	Vector<VNum> &trigList = Object::Index[obj->Virtual()].triggers;
	if (trigList.Count()) {
		fprintf(fp, "\ttriggers = { ");
		for (int i = 0; i < trigList.Count(); i++)
			fprintf(fp, "%d ", trigList[i]);
		fprintf(fp, "};\n");
	}
	
	if (Object::Index[obj->Virtual()].func) {
		fprintf(fp, "\tspecproc = %s \"%s\";\n", 
			spec_obj_name(Object::Index[obj->Virtual()].func), 
			Object::Index[obj->Virtual()].farg);
	}

	fprintf(fp, "};\n");
	
	release_buffer(bitList);
	release_buffer(bitList2);
}


bool Object::Find(VNum vnum) {	return (Index.GetElement(vnum));	}



