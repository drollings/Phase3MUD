This is a February 2001 snapshot of the Struggle For Nyrilis MUD, a heavily
rewritten C++ descendant of CircleMUD and common patches for it, after
importing features and coding techniques from Thrytis' Death's Gate MUD and
Chris Jacobson's LexiMUD engine.

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
