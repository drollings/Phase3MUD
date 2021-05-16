#1 = {
	name = "memory test trigger";
	narg = 100;
	mobtriggers = { Memory };
	commands = "* assign this to a mob, force the mob to mremember you, then enter the\n
* room the mob is in while visible (not via goto)\n
say I remember you, %actor.name%!\n";
};
#2 = {
	name = "mob greet test";
	narg = 100;
	mobtriggers = { Greet };
	commands = "say Hello, %actor.name%, how are things to the %direction%?\n";
};
#3 = {
	name = "obj get test";
	narg = 100;
	mobtriggers = { Greet };
	commands = "echo You hear, 'Please put me down, %actor.name%'\n";
};
#4 = {
	name = "Generic Test Trigger";
	arg = "detach";
	narg = 100;
	mobtriggers = { Command };
	objtriggers = { Get };
	wldtriggers = { Enter };
	commands = "echo Okay.\n
detach mob %self% 4\n";
};
#5 = {
	name = "car/cdr test";
	arg = "test";
	narg = 100;
	mobtriggers = { Speech };
	commands = "set guy %actor%\n
' You're <%guy.name%>, and you're fighting <%guy.fighting.name%>.\n
' You're <%actor.name%>, and you're fighting <%actor.fighting.name%>.\n
' I'm fighting <%self.fighting.name%>\n
' You said <%speech%>, <%speech.head%>, <%speech.tail%>, <%speech.word(2)%>\n
if (%speech.contains(dozy)%)\n
  ' You said DOZY!\n
end\n
if (%self.name.contains(beggar)%)\n
  ' Oh, bother.\n
end\n
' I'll say <%ugmok%>, <%ugmok.head%>, <%ugmok.tail%>, <%ugmok.word(2)%>\n
' What about <%beggar.name%>, <%self.name.2%>, <%self.name(2)%>?\n";
};
#6 = {
	name = "subfield test";
	arg = "test";
	narg = 100;
	mobtriggers = { Command };
	commands = "* test to make sure %actor.skill(skillname)% works\n
say your hide ability is %actor.skill(hide)% percent.\n
' I am hunting %self.hunting%.\n
' I am seeking %self.seeking%.\n
' I am roomseeking %self.roomseeking%.\n";
};
#7 = {
	name = "object transform test";
	arg = "test";
	narg = 2;
	objtriggers = { Command };
	commands = "* test of object otransformation (and remove trigger)\n
* test is designed for objects 3020 and 3021\n
* assign the trigger then wear/remove the item\n
* repeatedly.\n
echo Beginning object otransform.\n
if %self.vnum% == 3020\n
  transform 3021\n
else\n
  transform 3020\n
end\n
echo Transform complete.\n";
};
#8 = {
	name = "makeuid and remote testing";
	arg = "test";
	wldtriggers = { Command };
	commands = "* makeuid test ---- assuming your MOBOBJ_ID_BASE is 200000,\n
* this will display the names of the first 10 mobs loaded on your MUD,\n
* if they are still around.\n
eval counter 0\n
while (%counter% < 10)\n
  makeuid mob 200000+%counter%\n
  echo #%counter%      %mob.id%   %mob.name%\n
  eval counter %counter% + 1\n
done\n
*\n
*\n
* this will also serve as a test of getting a remote mob's globals.\n
* we know that puff, when initially loaded, is id 200000. We'll use remote\n
* to give her a global, then %mob.globalname% to read it.\n
makeuid mob 200000\n
eval globalname 12345\n
remote globalname %mob.id%\n
echo %mob.name%'s "globalname" value is %mob.globalname%\n";
};
#9 = {
	name = "transform test";
	arg = "bah";
	narg = 7;
	mobtriggers = { Command };
	objtriggers = { Command };
	wldtriggers = { Command Enter Reset };
	commands = "* mtransform test\n
* as a greet trigger, entering the room will cause\n
* the mob this is attached to, to toggle between mob 1 and 99.\n
echo Beginning transform.\n
if %self.vnum%==1\n
  transform 99\n
else\n
  transform 1\n
end\n
echo Transform complete.\n";
};
#10 = {
	name = "death trigger test";
	narg = 101;
	mobtriggers = { Death };
	commands = "' Schade for me!\n";
};
#11 = {
	name = "DEMO: Object command trigger test";
	arg = "test";
	narg = 7;
	objtriggers = { Command };
	commands = "* Note that the NARG is critical here.  The NARG behaves differently\n
* for object command scripts than for anything else; it is a\n
* bitvector of on/off flags which determine whether it is called\n
* when the object is in the room, inventory, or equipment list.\n
echo Object command trigger responding.\n";
};
#12 = {
	name = "DEMO: Switch test";
	arg = "test";
	narg = 100;
	wldtriggers = { Command };
	commands = "set counter 1\n
switch (%counter%)\n
  case 0\n
    echo Ugmok!\n
  case 1\n
    echo Blargh!\n
  case 2\n
    echo Eh.\n
done\n
unset counter\n";
};
#13 = {
	name = "DEMO: Load trigger";
	narg = 101;
	mobtriggers = { Load };
	commands = "* The 101 argument means a 101% chance of this happening - our\n
* hacking way of making a script execute every chance the program\n
* gives it a chance, even if it means executing multiple scripts!\n
' I'm ALIIIIIIIIIVE!\n";
};
#14 = {
	name = "UTILITY: Mobile hunt/seek/roomseek repo";
	narg = 101;
	mobtriggers = { Random };
	commands = "eval hunted %self.hunting%\n
eval sought %self.seeking%\n
shout Hunting %hunted.name%, seeking %sought.name%, roomseeking %self.roomseeking%.\n
unset hunted\n
unset sought\n";
};
#15 = {
	name = "DEMO:  Generic commands.";
	narg = 100;
	mobtriggers = { Greet };
	objtriggers = { Get };
	wldtriggers = { Enter };
	commands = "echo Test trigger responding.\n
force %actor% bounce\n
set inroom %self.room%\n
set i %inroom.objects%\n
if %i%\n
  force %i% echo %i.name% flashes with random bursts of light.\n
end\n";
};
#16 = {
	name = "DEMO: Shared variable test";
	narg = 50;
	mobtriggers = { Random };
	commands = "if (%shared_test% == 1)\n
say Happy Happy Joy Joy!\n
end\n
if (%global_test% == 1)\n
say Doobie Doobie Doo!\n
set shared_test 1\n
shared shared_test\n
end\n";
};
#17 = {
	name = "DEMO: Global variable set on speech";
	arg = "Make it so";
	mobtriggers = { Speech };
	commands = "eval global_test 1\n
global global_test\n
tell %actor% Variable set!\n";
};
#18 = {
	name = "DEMO:  Mobile command trigger";
	arg = "howdy";
	narg = 100;
	mobtriggers = { Command };
	commands = "' Doobie doobie do!\n";
};
#19 = {
	name = "Direct script command";
	arg = "globalvar";
	narg = 7;
	mobtriggers = { Command };
	objtriggers = { Command };
	wldtriggers = { Command };
	commands = "* Simple script to easily set global variables\n
return 0\n
if (%actor.trust% <= 0)\n
  halt\n
end\n
setglobal %arg.head% %arg.tail%\n";
};
#20 = {
	name = "Shrine of Pelendra";
	arg = "meditate";
	narg = 103;
	objtriggers = { Command };
	wldtriggers = { Command };
	commands = "set inroom %actor.room%\n
send %actor% You kneel and seek enlightenment...\n
echoaround %actor% %actor.name% kneels in meditation.\n
force %actor% sit\n
wait 10\n
if (%inroom% != %actor.room%)\n
  halt\n
end\n
if (%actor.align% < 0)\n
  send %actor% You sense that you have strayed...\n
  halt\n
end\n
eval newmana %actor.mana% - 50\n
eval newmove %actor.move% - 50\n
if (%newmana% < 0) || (%newmove% < 0)\n
  send %actor% You feel too drained to focus on meditation.\n
  halt\n
end\n
set ordination %actor.skillpts(ordination of pelendra)%\n
if (%ordination% == 0)\n
  send %actor% You sense a presence, a deep love of life here.\n
  halt\n
end\n
eval counter 0\n
while (%counter% < 3)\n
  switch (%counter%)\n
  case 0\n
    eval sphere %ordination%\n
    skillset %actor% 'sphere of healing' %sphere%\n
    break\n
  case 1\n
    eval sphere %ordination% - 3\n
    skillset %actor% 'sphere of protection' %sphere%\n
    break\n
  case 2\n
    eval sphere %ordination%\n
    skillset %actor% 'sphere of nature' %sphere%\n
    break\n
  done\n
  eval counter %counter% + 1\n
done\n
echoaround %actor% %actor.name%'s face is touched with new vision.\n
send %actor% &cBA voice bids you, '&cGGo, bring forth healing in my name!&cB'&c0\n
wait 3\n
send %actor% You feel the hand of Pelendra upon you.  Insights into the healing of wounds,  \n
send %actor% &c0the restoration of nature, and protection from harm flood your consciousness!\n
setchar %actor% mana %newmana%\n
setchar %actor% move %newmove%\n";
};
#21 = {
	name = "Shrine of Dymordian";
	arg = "meditate";
	narg = 103;
	objtriggers = { Command };
	wldtriggers = { Command };
	commands = "set inroom %actor.room%\n
send %actor.name% You kneel and seek enlightenment...\n
echoaround %actor% %actor.name% kneels in meditation.\n
force %actor% sit\n
wait 10\n
if (%inroom% != %actor.room%)\n
  halt\n
end\n
if (%actor.align% < -200)\n
  send %actor% You sense that you have strayed...\n
  halt\n
end\n
eval newmana %actor.mana% - 50\n
eval newmove %actor.move% - 50\n
if (%newmana% < 0) || (%newmove% < 0)\n
  send %actor% You feel too drained to focus on meditation.\n
  halt\n
end\n
set ordination %actor.skillpts(ordination of dymordian)%\n
if (%ordination% == 0)\n
  send %actor% Reality's veil becomes translucent here, and thoughts transcending\n
  send %actor% time and space echo in your mind...\n
  halt\n
end\n
eval counter 0\n
while (%counter% < 3)\n
  switch (%counter%)\n
  case 0\n
    eval sphere %ordination%\n
    skillset %actor% 'sphere of creation' %sphere%\n
    break\n
  case 1\n
    eval sphere %ordination% - 1\n
    skillset %actor% 'sphere of divination' %sphere%\n
    break\n
  case 2\n
    eval sphere %ordination% - 3\n
    skillset %actor% 'sphere of heavens' %sphere%\n
    break\n
  done\n
  eval counter %counter% + 1\n
done\n
echoaround %actor% %actor.name%'s gaze grows keen with arcane knowledge.\n
send %actor% &cBA voice speaks, '&cCI bid you, carry my knowledge to the world!&cB'&c0\n
wait 3\n
send %actor% Dymordian's wisdom suffuses you.  The strands of the fabric of the universe\n
send %actor% reach from void to void, carrying your thought with them to infinity and back,\n
send %actor% and the secrets of the world make themselves known to you...\n
setchar %actor% mana %newmana%\n
setchar %actor% move %newmove%\n";
};
#22 = {
	name = "Shrine of Tal-Vindas";
	arg = "meditate";
	narg = 103;
	objtriggers = { Command };
	wldtriggers = { Command };
	commands = "set inroom %actor.room%\n
send %actor% You kneel and seek enlightenment...\n
echoaround %actor% %actor.name% kneels in meditation.\n
force %actor% sit\n
wait 10\n
if (%inroom% != %actor.room%)\n
  halt\n
end\n
if (%actor.align% < -200)\n
  send %actor% You sense that you have strayed...\n
  halt\n
end\n
eval newmana %actor.mana% - 50\n
eval newmove %actor.move% - 50\n
if (%newmana% < 0) || (%newmove% < 0)\n
  send %actor% You feel too drained to focus on meditation.\n
  halt\n
end\n
set ordination %actor.skillpts(ordination of talvindas)%\n
if (%ordination% == 0)\n
  send %actor% You bow your head and almost feel the divine light...\n
  send %actor% But not quite.\n
  halt\n
end\n
eval counter 0\n
while (%counter% < 3)\n
  switch (%counter%)\n
  case 0\n
    eval sphere %ordination%\n
    skillset %actor% 'sphere of protection' %sphere%\n
    break\n
  case 1\n
    eval sphere %ordination% - 1\n
    skillset %actor% 'sphere of war' %sphere%\n
    break\n
  case 2\n
    eval sphere %ordination% - 3\n
    skillset %actor% 'sphere of heavens' %sphere%\n
    break\n
  done\n
  eval counter %counter% + 1\n
done\n
echoaround %actor% %actor.name%'s countenance radiates holiness.\n
send %actor% &cBA voice speaks, '&cWTake forth my blessings, and defend the just!&cB'&c0\n
wait 3\n
send %actor% Tal-Vindas' presence makes itself known, a powerful force in your mind that \n
send %actor% searches your mind and heart, urging you on to make right in the world.\n
setchar %actor% mana %newmana%\n
setchar %actor% move %newmove%\n";
};
#23 = {
	name = "Shrine of Dymordian";
	arg = "meditate";
	narg = 100;
	wldtriggers = { Command };
	commands = "set inroom %actor.room%\n
send %actor.name% You kneel and seek enlightenment...\n
echoaround %actor% %actor.name% kneels in meditation.\n
force %actor% sit\n
wait 10\n
if (%inroom% != %actor.room%)\n
  halt\n
end\n
if (%actor.align% < -200)\n
  send %actor% You sense that you have strayed...\n
  halt\n
end\n
eval newmana %actor.mana% - 50\n
eval newmove %actor.move% - 50\n
if (%newmana% < 0) || (%newmove% < 0)\n
  send %actor% You feel too drained to focus on meditation.\n
  halt\n
end\n
set ordination %actor.skillpts(ordination of dymordian)%\n
if (%ordination% == 0)\n
  send %actor% You bow your head and almost feel the divine light...\n
  send %actor% But not quite.\n
  halt\n
end\n
eval counter 0\n
while (%counter% < 3)\n
  switch (%counter%)\n
  case 0\n
    eval sphere %ordination%\n
    skillset %actor% 'sphere of protection' %sphere%\n
    break\n
  case 1\n
    eval sphere %ordination% - 1\n
    skillset %actor% 'sphere of war' %sphere%\n
    break\n
  case 2\n
    eval sphere %ordination% - 3\n
    skillset %actor% 'sphere of heavens' %sphere%\n
    break\n
  done\n
  eval counter %counter% + 1\n
done\n
echoaround %actor% %actor.name%'s countenance radiates holiness.\n
send %actor% &cBA voice speaks, '&cWTake forth my blessings, and defend the just!&cB'&c0\n
wait 3\n
send %actor% Tal-Vindas' presence makes itself known, a powerful force in your mind that \n
send %actor% searches your mind and heart, urging you on to make right in the world.\n
setchar %actor% mana %newmana%\n
setchar %actor% move %newmove%\n";
};
#24 = {
	name = "Shrine of Nymris";
	arg = "meditate";
	narg = 100;
	wldtriggers = { Command };
	commands = "set inroom %actor.room%\n
send %actor% You kneel and seek enlightenment...\n
echoaround %actor% %actor.name% kneels in meditation.\n
force %actor% sit\n
wait 10\n
if (%inroom% != %actor.room%)\n
  halt\n
end\n
if (%actor.align% < -200)\n
  send %actor% You sense that you have strayed...\n
  halt\n
end\n
eval newmana %actor.mana% - 50\n
eval newmove %actor.move% - 50\n
if (%newmana% < 0) || (%newmove% < 0)\n
  send %actor% You feel too drained to focus on meditation.\n
  halt\n
end\n
set ordination %actor.skillpts(ordination of nymris)%\n
if (%ordination% == 0)\n
  send %actor% Mystery tingles in your mind; you can almost see the\n
  send %actor% stories of the past before your mind's eye.\n
  halt\n
end\n
eval counter 0\n
while (%counter% < 2)\n
  switch (%counter%)\n
  case 0\n
    eval sphere %ordination% + 2\n
    skillset %actor% 'sphere of protection' %sphere%\n
    break\n
  case 1\n
    eval sphere %ordination% + 2\n
    skillset %actor% 'sphere of divination' %sphere%\n
    break\n
  done\n
  eval counter %counter% + 1\n
done\n
echoaround %actor% %actor.name%'s eyes light with the knowledge of unseen things.\n
send %actor% &cBA voice speaks, '&cwMortal, harken to the stories of those who walk with you.&cB'&c0\n
wait 3\n
send %actor% Haunting scenes play before your eyes, as the past greets you like a familiar\n
send %actor% companion.  You feel very aware of things most mortals do not see...\n
setchar %actor% mana %newmana%\n
setchar %actor% move %newmove%\n";
};
#25 = {
	name = "Shrine of Vaadh";
	arg = "meditate";
	narg = 103;
	objtriggers = { Command };
	wldtriggers = { Command };
	commands = "set inroom %actor.room%\n
send %actor.name% You kneel and seek enlightenment...\n
echoaround %actor% %actor.name% kneels in meditation.\n
force %actor% sit\n
wait 10\n
if (%inroom% != %actor.room%)\n
  halt\n
end\n
if (%actor.align% > -200)\n
  send %actor% You sense that you have strayed...\n
  halt\n
end\n
eval newmana %actor.mana% - 50\n
eval newmove %actor.move% - 50\n
if (%newmana% < 0) || (%newmove% < 0)\n
  send %actor% You feel too drained to focus on meditation.\n
  halt\n
end\n
set ordination %actor.skillpts(ordination of vaadh)%\n
if (%ordination% == 0)\n
  send %actor% You feel the strong will of Vaadh overshadowing you.\n
  halt\n
end\n
eval counter 0\n
while (%counter% < 3)\n
  switch (%counter%)\n
  case 0\n
    eval sphere %ordination% - 2\n
    skillset %actor% 'sphere of heavens' %sphere%\n
    break\n
  case 1\n
    eval sphere %ordination%\n
    skillset %actor% 'sphere of necromancy' %sphere%\n
    break\n
  case 2\n
    eval sphere %ordination%\n
    skillset %actor% 'sphere of war' %sphere%\n
    break\n
  done\n
  eval counter %counter% + 1\n
done\n
echoaround %actor% %actor.name%'s countenance fills with awe.\n
send %actor% &cBA voice speaks, '&cLTake hold of the chaos, and establish my will!&cB'&c0\n
wait 3\n
send %actor% Vaadh's dictates pound within you, filling you with a sense of awe and the\n
send %actor% absolute need to bring his order to the world.\n
setchar %actor% mana %newmana%\n
setchar %actor% move %newmove%\n";
};
#26 = {
	name = "Shrine of Shestu";
	arg = "meditate";
	narg = 103;
	objtriggers = { Command };
	wldtriggers = { Command };
	commands = "set inroom %actor.room%\n
send %actor% You bow your head and seek the power...\n
echoaround %actor% %actor.name% kneels in meditation.\n
force %actor% sit\n
wait 10\n
if (%inroom% != %actor.room%)\n
  halt\n
end\n
if (%actor.align% >= -200)\n
  send %actor% You sense that you have strayed...\n
  halt\n
end\n
eval newmana %actor.mana% - 50\n
eval newmove %actor.move% - 50\n
if (%newmana% < 0) || (%newmove% < 0)\n
  send %actor% You feel too drained to focus on meditation.\n
  halt\n
end\n
set ordination %actor.skillpts(ordination of shestu)%\n
if (%ordination% == 0)\n
  send %actor% You feel feral bloodlust filling the air about you.\n
  halt\n
end\n
eval sphere %ordination%\n
skillset %actor% 'sphere of war' %sphere%\n
echoaround %actor% %actor.name%'s eyes flare with bloodlust!\n
send %actor% &cBA voice speaks, '&cRGo, and drive the weak and complacent before you!&cB'&c0\n
wait 3\n
send %actor% There is no stopping you now.  You feel the call of battle, of rampant mayhem,\n
send %actor% and you must have victory!\n
setchar %actor% mana %newmana%\n
setchar %actor% move %newmove%\n";
};
#30 = {
	name = "Pushfront/pushback demo";
	arg = "push";
	narg = 100;
	wldtriggers = { Command };
	commands = "if !(%queue%)\n
  setglobal queue\n
  setglobal stack\n
end\n
pushfront stack %arg%\n
pushfront queue %arg%\n";
};
#31 = {
	name = "Popfront demo";
	arg = "popfront";
	narg = 100;
	wldtriggers = { Command };
	commands = "popfront queue poppedq\n
popfront stack poppeds\n
echo Queue:  <%queue%> <%poppedq%>\n
echo Stack:  <%stack%> <%poppeds%>\n
global poppedq\n
global poppeds\n";
};
#32 = {
	name = "Push/pop demo check";
	arg = "test";
	narg = 100;
	wldtriggers = { Command };
	commands = "echo Queue:  <%queue%> <%poppedq%>\n
echo Stack:  <%stack%> <%poppeds%>\n";
};
#33 = {
	name = "Push/Pop demo: queue cycle";
	arg = "cycle";
	narg = 100;
	wldtriggers = { Command };
	commands = "set poppedq %queue.popfront%\n
global poppedq\n";
};
#34 = {
	name = "Popback demo";
	arg = "popback";
	narg = 100;
	wldtriggers = { Command };
	commands = "popback queue poppedq\n
popback stack poppeds\n
echo Queue:  <%queue%> <%poppedq%>\n
echo Stack:  <%stack%> <%poppeds%>\n
global poppedq\n
global poppeds\n";
};
#35 = {
	name = "Caltrops";
	narg = 100;
	mobtriggers = { Greet-All };
	objtriggers = { Greet };
	wldtriggers = { Enter };
	commands = "if %actor.affected(flying)%\n
  halt\n
end\n
eval damage %random.6% + %random.6% + %random.6%\n
send %actor% Caltrops spring from the ground, stabbing your feet!\n
echoaround %actor% Caltrops spring from the ground, stabbing at %actor.name%'s feet!\n
if (%actor.skillroll(dexterity)%) > 5 && (%actor.skillroll(intelligence)% > 5)\n
  eval damage %damage% / 3\n
  send %actor% You cleverly avoid most of the spikes!\n
  echoaround %actor% %actor.name% cleverly avoids most of the spikes!\n
else\n
damage %actor% %damage% piercing\n
detach mob %self% 35\n";
};
#40 = {
	name = "Law enforcement: guardhouse";
	arg = "offense";
	narg = 100;
	mobtriggers = { Signal };
	wldtriggers = { Random Reset Signal };
	commands = "* This trigger is expected to be copied for each zone that needs it.  Attach it\n
* to the guardhouse room and to each guard and citizen in the town.\n
* Fill in the list of offenders from the queue on random/command trigger\n
while %offenders.head%\n
  popfront offenders temp\n
  if %temp.id%\n
    context %temp.id%\n
    popfront offenders temp\n
    pushback offense %temp%\n
    shared offense\n
    context 0\n
  end\n
done\n
* If this is a command, accept input\n
if %arg%\n
  set badguy %arg.head%\n
  context %badguy.id%\n
  pushback offense %arg.tail%\n
  shared offense\n
  context 0\n
end\n
* Don't set the variables a second time, for zone reset\n
if %guardhouse%\n
  halt\n
end\n
setshared guardhouse %self.vnum%\n
setshared offenders\n";
};
#41 = {
	name = "Law enforcement: citizen alarm";
	narg = 101;
	mobtriggers = { Entry Fight };
	commands = "if %self.fighting%\n
  switch (%random.7%)\n
  case 1\n
    shout HELP!  GUARDS!!!\n
    signal zone trouble %self.room%\n
    break\n
  case 2\n
    set guy %self.fighting%\n
    shout Help me!  Someone's attacking me!\n
    signal zone trouble %self.room%\n
    break\n
  done\n
  setglobal notify %self.fighting% \n
  pushback notify %self.room%\n
  signal room guards %notify%\n
* roomseek %guardhouse%\n
elseif %notify%\n
  wait 1\n
  if %self.room% == %guardhouse%\n
    set toroom %notify.tail%\n
    signal room guards %notify%\n
    say Hurry!  There's trouble at %toroom.name%!\n
  elseif %random.2% == 1\n
    say Someone get the guards, quick!\n
    signal room guards %notify%\n
  end\n
end\n";
};
#42 = {
	name = "Law enforcement: citizen signals";
	arg = "guards";
	narg = 100;
	mobtriggers = { Signal };
	commands = "if %notify% == ""\n
  if %self.roomseeking%\n
    pushfront waypoint %self.roomseeking%\n
  end\n
  roomseek %guardhouse%\n
  setglobal notify %arg%\n
  * gossip Notified of <%arg%>, passing it along!\n
end\n";
};
#43 = {
	name = "Law enforcement: guard alarm";
	narg = 101;
	mobtriggers = { Entry Fight };
	commands = "halt\n
if %self.fighting%\n
  switch (%random.7%)\n
  case 1\n
    shout To arms!  To arms!\n
    signal zone trouble %self.room%\n
    break\n
  case 2\n
    shout We're being attacked!\n
    signal zone trouble %self.room%\n
    break\n
  done\n
  signal room guards %notify%\n
  pushback offenders %self.fighting% 8\n
end\n";
};
#44 = {
	name = "Law enforcement: guard signals";
	arg = "guards gotem";
	mobtriggers = { Signal };
	commands = "shout Received signal <%cmd%:%arg%>\n
switch %cmd%\n
case guards\n
  if (%apprehend% == %arg.head%)\n
    unset apprehend\n
    pushback killonsight %arg.head%\n
    rescue %actor%\n
  end\n
  break\n
case gotem\n
  if (%apprehend% == %arg.head%)\n
    unset apprehend\n
    pushback killonsight %arg.head%\n
    rescue %actor%\n
  end\n
done\n";
};
#45 = {
	name = "Law enforcement: shout signal";
	arg = "trouble";
	narg = 50;
	mobtriggers = { Signal };
	commands = "if (!%destination%)\n
  pushfront destination %arg%\n
  global destination\n
  tell Garadon Seeking troublespot %destination.name%\n
  roomseek %destination.head%\n
end\n";
};
#46 = {
	name = "Law enforcement: prisoner restraint";
	arg = "west east north south down up follow stalk hit kick punch headbutt kill cast quit flee";
	narg = 100;
	mobtriggers = { Command };
	commands = "if %actor% == %prisoner%\n
  if %cmd% == quit\n
    growl\n
    delay 5 ' You aren't getting away that easy, %actor.name%!\n
    halt\n
  end\n
  eval strength %actor.skillroll(strength)%\n
  eval mystrength %self.skillroll(strength)% + 5\n
  if %strength% < %mystrength% || %strength% < 0\n
    send %actor% %self.name%'s firm grip holds you fast!\n
    echoaround %actor% %actor.name% struggles in vain.\n
    growl\n
    if %random.4% == 1\n
      at %guardhouse% notify_offense %actor% resist\n
      delay 2 ' That's resisting arrest, you!\n
    else\n
      delay 2 ' Come along!\n
    end\n
  else\n
    send %actor% You break free!\n
    echoaround %actor% %actor.name% breaks free of %self.name%'s grip!\n
    force %actor% follow self\n
    ' HEY!\n
    hunt %actor%\n
    unset prisoner foundem tocourt\n
    setglobal apprehend %actor%\n
    at %guardhouse% signal room offense %actor% resist\n
    signal zone trouble %self.room%\n
  end\n
else\n
  return 0\n
end\n";
};
#47 = {
	name = "Law enforcement: Shout response";
	arg = "trouble";
	narg = 50;
	mobtriggers = { Signal };
	commands = "if (%destionation == "")\n
  pushfront destination %arg%\n
  global destination\n
  tell Garadon Seeking troublespot %destination.name%\n
  roomseek %destination.head%\n
end\n";
};
#48 = {
	name = "Law enforcement: apprehension";
	narg = 100;
	mobtriggers = { Greet Entry };
	commands = "if %self.fighting%\n
  halt\n
end\n
if %tocourt%\n
  * They're being dragged off, foundem and tocourt are set\n
  if %self.room% == %tocourt%\n
    wait 1s\n
    say %foundem.heshe% stands accused of felony!\n
    signal room court %foundem%\n
    unset tocourt foundem degree\n
  end\n
  halt\n
end\n
if %apprehend%\n
  * Check room for the criminal\n
  if %apprehend.contains(%actor%)%\n
    set foundem %actor%\n
  else\n
    wait 1\n
    set inroom %self.room%\n
    foreach mob i (%inroom.people%)\n
      * gossip Checking mob %i.name%...\n
      if %i% == %apprehend% && %i.canbeseen%\n
        set foundem %i%\n
        break\n
      end\n
    done\n
  end\n
end\n
if %foundem%\n
  * Make the arrest\n
  global foundem\n
  signal zone gotem %foundem%\n
  ' Hey, you!  %foundem.name%!\n
  wait %random.2%s\n
  ' You're under arrest for disturbing the peace.\n
  wait 1s\n
  ' You will come peacefully, or face the consequences!  Follow me!\n
  wait 10s\n
  set temper 0\n
  set targfol %foundem.following%\n
  while !(%targfol% && %targfol.keywords% /= guard)\n
    if %self.fighting%\n
      halt\n
    end\n
    if %self.room% != %foundem.room%\n
      seek %foundem%\n
      halt\n
    end\n
    switch (%temper%)\n
    case 0\n
      switch (%foundem.position%)\n
      case resting\n
      case sitting\n
      case standing\n
      case fighting\n
        shout I said, lay down your weapons and come with me!\n
        break\n
      default\n
        set %foundem% hp 1\n
        force %foundem% follow %self%\n
        break\n
      done\n
      break\n
    default\n
      shout You had your chance, knave!\n
      hunt %foundem%\n
      unset foundem\n
      halt\n
      break\n
    done\n
    wait 10s\n
    eval temper %temper% + 1\n
    set targfol %foundem.following%\n
  done\n
  send %foundem% %self.name% grabs you in a firm grip!\n
  echoaround %foundem% %self.name% grabs %foundem.name% in a firm grip!\n
  force %foundem% follow %self%\n
  setglobal prisoner %foundem%\n
  * Attach prisoner restraint\n
  attach mob %self% 45\n
  setglobal tocourt %guardhouse%\n
  roomseek %tocourt%\n
end\n";
};
#50 = {
	name = "Random weapon inventory/combat profile";
	narg = 101;
	mobtriggers = { Load };
	commands = "switch (%random.4%)\n
case 1\n
* wooden shield, short sword\n
  load obj 95\n
  load obj 33\n
  combat 2 hit\n
  combat 3 shield bash\n
  combat 4 hit\n
  combat 5 shield block\n
  combat 6 right parry\n
  at 2 wield short\n
  break\n
case 2\n
* military pick\n
  load obj 45\n
  combat 1 right swing\n
  combat 2 right parry\n
  break\n
case 3\n
* halberd\n
  load obj 50\n
  combat 1 right swing\n
  combat 2 right thrust\n
  combat 3 right parry\n
  break\n
default\n
* glaive\n
  load obj 51\n
  combat 1 right swing\n
  combat 2 right thrust\n
  combat 3 right parry\n
done\n
at 2 wear all\n";
};
#51 = {
	name = "Random ragtag armor";
	narg = 101;
	mobtriggers = { Load };
	commands = "switch (%random.3%)\n
case 1\n
* cloth cap, chainmail sleeves, chainmail pants\n
  load obj 62\n
  load obj 76\n
  load obj 82\n
  break\n
case 2\n
* scale mail jacket, pot helm, cloth pants\n
  load obj 71\n
  load obj 63\n
  load obj 80\n
  break\n
default\n
* hard leather breastplate, heavy leather boots, padded cloth sleeves,\n
  load obj 68\n
  load obj 89\n
  load obj 74\n
done\n
at 2 wear all\n";
};
#52 = {
	name = "Generic Defensive Combat Profile";
	narg = 101;
	mobtriggers = { Load };
	commands = "combat 8 right parry\n
combat posture evasive\n";
};
#53 = {
	name = "Random Fire Magic";
	narg = 50;
	mobtriggers = { Fight };
	commands = "switch (%random.4%)\n
case 1\n
  cast 'fireball' %actor%\n
  break\n
case 2\n
  cast 'lighting bolt' %actor%\n
  break\n
case 3\n
  cast 'burning hands' %actor%\n
  break\n
case 4\n
  cast 'color spray' %actor%\n
  break\n
end\n
done\n";
};
#54 = {
	name = "Destination handling";
	narg = 100;
	mobtriggers = { Greet Entry };
	commands = "* This script allows a mobile to follow a trail.  Simply set a list of rooms, \n
* and the mobile will roomseek each one in the list, in succession.\n
if %waypoint%\n
  if (%self.room% == %waypoint.head%) && (%self.room% == %self.roomseeking%)\n
    popfront waypoint temp\n
    roomseek 0\n
    if (%waypoint% == "")\n
      unset waypoint\n
    end\n
  end\n
  if (%self.roomseeking% == 0) && %waypoint%\n
    roomseek %waypoint.head%\n
  end\n
end\n";
};
#60 = {
	name = "Shopkeeper charisma reaction.";
	arg = "buy sell list value";
	narg = 100;
	mobtriggers = { Command };
	commands = "if %actor.cha% < 5\n
  tell %actor% Get out of my shop!\n
else\n
  return 0\n
endif\n";
};
#61 = {
	name = "Guildmaster charisma reaction.";
	arg = "practice";
	narg = 100;
	mobtriggers = { Command };
	commands = "if %actor.cha% < 5\n
  tell %actor% Get out of here!\n
else\n
  return 0\n
endif\n";
};
#62 = {
	name = "Generic draw/sheathe weapon";
	narg = 60;
	mobtriggers = { Random Fight };
	commands = "if %self.fighting%\n
  if !(%weapondrawn%)\n
    draw\n
    draw\n
    setglobal weapondrawn 1\n
  end\n
else\n
  if %weapondrawn%\n
    sheath\n
    sheath\n
    unset weapondrawn\n
  end\n
end\n";
};
#74 = {
	name = "new trigger";
	arg = "flush";
	narg = 2;
	objtriggers = { Command };
	wldtriggers = { Command };
	commands = "echo %self.name% executing foreach\n
foreach obj i (%self.contains%)\n
  purge %i%\n
done\n";
};
#75 = {
	name = "Vehicle cockpit";
	arg = "north south east west up down look leave register@";
	narg = 100;
	wldtriggers = { Command };
	commands = "* This is a simple vehicle script meant to be paired with 22001, which is\n
* attached to a mobile.  Script 22001 responds to the "link@ <roomnum>"\n
* command, which makes it register with this script.\n
if %cmd% == register@\n
  setglobal vehicle %actor%\n
elseif %vehicle%\n
  if %cmd% == leave\n
    * "leave" command leaves vehicle\n
    send %actor% You step off of %vehicle.name%.\n
    echoaround %actor% %actor.name% steps off %vehicle.name%.\n
    teleport %actor% %vehicle.room%\n
    echoaround %actor% %actor.name% steps off %vehicle.name%.\n
    force %actor% look\n
  elseif %cmd% == look\n
    if %arg% /= out\n
      * "look outside"\n
      teleport %actor% %vehicle.room%\n
      force %actor% look\n
      teleport %actor% %self.vnum%\n
      halt\n
    else\n
      * Regular look command, work as usual.\n
      return 0\n
      halt\n
    end\n
  else\n
    * Move the vehicle, this step alerts pilot of lack of exits\n
    set inroom %vehicle.room%\n
    force %vehicle% %cmd%\n
    if %inroom% == %vehicle.room%\n
      send %actor% %vehicle.name% cannot go that way.\n
    end\n
  end\n
  return 1\n
else\n
  echo ERROR:  Vehicle not found!\n
end\n";
};
#76 = {
	name = "Vehicle avatar";
	arg = "board enter link@";
	narg = 100;
	mobtriggers = { Command Death Entry };
	commands = "* This script allows the mobile to interact with the cockpit.  This isn't\n
* absolutely necessary, but it allows more detail.\n
if %cmd%\n
  if %cmd% == link@\n
    * Register the mobile as the vehicle's avatar\n
    at %arg% register@\n
    setglobal cockpit %arg%\n
  elseif (%self.name% /= %arg%) && %cockpit%\n
    * Entry command\n
    send %actor% You board %self.name%.\n
    echoaround %actor% %actor.name% boards %self.name%.\n
    teleport %actor% %cockpit%\n
    echoaround %actor% %actor.name% has boarded %self.name%.\n
    force %actor% look\n
  end\n
elseif %cockpit%\n
  if %actor%\n
    * Triggered by death, as there's no cmd variable\n
    set where %self.room%\n
    at %cockpit% echo %self.name% is destroyed!\n
    at %cockpit% at %cockpit% teleport mobs%where%\n
  else\n
    * Okay, this is simple movement.\n
    at %cockpit% echo %self.name% moves %direction%.\n
  end\n
end\n";
};
#80 = {
	name = "GENERIC:  Warrior combat";
	narg = 101;
	mobtriggers = { Load };
	commands = "combat 1 hit\n
combat 2 right parry\n";
};
#90 = {
	name = "Staff EQ Script";
	narg = 100;
	objtriggers = { Get Drop Give Wear };
	commands = "if %actor.trust% < 1\n
   echo %self.shortdesc% returns to the staff bit bucket.\n
   purge self\n
end\n";
};
#99 = {
	name = "Calculator";
	arg = "calculate";
	narg = 3;
	objtriggers = { Command };
	commands = "set name %self.keywords%\n
echoaround %actor% %actor.name% punches some numbers into %name.head%.\n
send %actor% You punch "%arg%" into your %name.head%.\n
eval result %arg%\n
send %actor% The answer of "%result%" appears on the screen.\n";
};
$
