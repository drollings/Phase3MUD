This is a February 2001 snapshot of the Struggle For Nyrilis MUD's Phase 3, a 
heavily rewritten C++ descendant of Jeremy Elson's CircleMUD, a cousin of 
Chris Jacobson's LexiMUD engine, with heavily rewritten and extended pieces 
from Thrytis' Death's Gate MUD.  Ultimately this goes back to DIKU.

This is ancient C++, meant for gcc 2.95.  The STL data structures are
courtesy of Chris Jacobson.  Safe pointers were implemented by Daniel
Rollings AKA Daniel Houghton.  Modern compilers will likely flag a lot of
buffer size errors that were simply not a concern at the time; 8192 byte
shared buffers being cleared and flushed were apparently enough for a few
dozen user's screenfuls of text and entered commands.

LexiMUD and SFN shared snippets and rewrites of the scripting engine, though
they diverged particularly in that SFN used pre-parsed function pointers to
accelerate the scripting language, while LexiMUD integrated a proprietary
JIT engine.

Of particular note was a fusion of AD&D features with a GURPS-like skill
system which made character levels more of a visible score than any
determination of skill.

Crypto support was broken at the time of this snapshot.  You'd have to be
mad to run this online in the same container as anything important
whatsoever.

CVS history omitted.

Oasis OLC was a notable snippet popular for CircleMUD at the time.  It was
incorporated, and vastly rewritten until its interface was all that appeared
the same, as you'd expected for a change from a heavily refactored C to a
C++ codebase.  It relied heavily on a new Editor class which used a vastly
deprecated way to use memory offsets and casting to easily reuse code,
determined by data in compiled literal structs.

After major rewrites from C to C++ and a vast amount of refactoring and
added features, there are still pieces of code recognizably belonging to
Jeremy Elson's CircleMUD, which falls under the DikuMUD license.  The
CircleMUD FAQ and README have been included to respect this.

All code contributed by Daniel Rollings (AKA Daniel Houghton) is under the
MIT license.  Lacking commit history, it will take a while to part this out.

