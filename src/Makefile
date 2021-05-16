# Generated automatically from Makefile.in by configure.
# CircleMUD 3.0 Makefile.in - Makefile template used by 'configure'
#

# C compiler to use
CC = ccache gcc
# if you don't have g++, try cpp or c++ or cxx
CC = ccache g++

# Any special flags you want to pass to the compiler
MYFLAGS = -Wall -pg
MYFLAGS += -Wno-unused -fno-inline
# MYFLAGS += -fexternal-templates
# MYFLAGS += -O6
# MYFLAGS += -O2
# MYFLAGS += -nostdinc++ -I~/egcs/include -L~/egcs/lib
# MYFLAGS += -I~/egcs/include -L~/egcs/lib

DEFINES   = -DREDHAT
# DEFINES   = -DFREEBSD
# DEFINES += -DTLO_DEBUG_MESSAGES

ETC = -lcrypt

#flags for profiling (see hacker.doc for more information)
PROFILE = 

##############################################################################
# Do Not Modify Anything Below This Line (unless you know what you're doing) #
##############################################################################

BINDIR = ../bin

#CFLAGS = -g -O2 $(MYFLAGS) $(PROFILE)
#CFLAGS = -g $(MYFLAGS) $(PROFILE) -DTLO_DEBUG_MESSAGES
CFLAGS = -g $(MYFLAGS) $(PROFILE) $(DEFINES)

OBJFILES = property.o \
	olc.o aedit.o cedit.o hedit.o medit.o oedit.o redit.o \
	scriptedit.o sedit.o zedit.o maputils.o \
	magic.o spells.o spell_parser.o \
	act.comm.o act.informative.o act.item.o act.movement.o \
        act.offensive.o act.other.o act.social.o act.wizard.o \
        affects.o ban.o boards.o buffer.o characters.o clans.o \
        cmdtab.o combat.o comm.o config.o constants.o db.o \
        descriptors.o event.o events.o files.o \
        find.o handler.o help.o house.o ident.o \
        index.o interpreter.o limits.o log.o mail.o \
        mobact.o editor.o objects.o \
        objsave.o opinion.o parser.o queue.o races.o \
        random.o rooms.o scriptcmd.o scriptengine.o \
        scripts.o shop.o skills.o sound.o \
        spec_assign.o spec_procs.o sstring.o track.o triggers.o \
        utils.o vehicles.o weather.o worldparser.o

CXREF_FILES = property.cc \
	olc.cc aedit.cc cedit.cc hedit.cc medit.cc oedit.cc redit.cc \
	scriptedit.cc sedit.cc zedit.cc maputils.cc \
	magic.cc spells.cc spell_parser.cc \
	act.comm.cc act.informative.cc act.item.cc act.movement.cc \
        act.offensive.cc act.other.cc act.social.cc act.wizard.cc \
        affects.cc ban.cc boards.cc buffer.cc characters.cc clans.cc \
        cmdtab.cc combat.cc comm.cc config.cc constants.cc \
	db.cc descriptors.cc event.cc events.cc files.cc \
        find.cc handler.cc help.cc house.cc ident.cc \
        index.cc interpreter.cc limits.cc log.cc mail.cc  \
        mobact.cc editor.cc objects.cc \
        objsave.cc opinion.cc parser.cc queue.cc races.cc \
        random.cc rooms.cc scriptcmd.cc scriptengine.cc \
        scripts.cc shop.cc skills.cc sound.cc \
        spec_assign.cc spec_procs.cc sstring.cc track.cc triggers.cc \
        utils.cc vehicles.cc weather.cc worldparser.cc

default: all

all: .accepted
	$(MAKE) $(BINDIR)/circle.new
#	$(MAKE) utils

.accepted:
#	@./licheck more

utils: .accepted

circle:
	$(MAKE) $(BINDIR)/circle.new

$(BINDIR)/circle.new : $(OBJFILES)
	$(CC) -o $(BINDIR)/circle.new $(PROFILE) $(OBJFILES) $(ETC)

# remove all the compiled objects for a clean compile
clean:
	rm -f *.o *~ *.rej *.orig

# recreate the dependency file 'Depend'
depend:
	$(CC) -MM *.cc > Depend

# default rule to make all of the .cc files
%.o: %.cc
	@echo $<
	@$(CC) -c $(CFLAGS) $<

ref:
#
# Create the cross reference files
# Note, this is not meant to be used unless you've installed cxref...
#
	@for file in $(CXREF_FILES) ; do \
	  echo Cross referencing $$file ; \
	  /home/jelson/cxref/cxref -D__CXREF__ -xref -Odoc -Ncircle $$file ; \
	done
#
# Create the source files using /home/jelson/cxref/cxref
#
	@for file in $(CXREF_FILES) ; do \
	   echo Documenting $$file ; \
	   ( cd . ; /home/jelson/cxref/cxref -D__CXREF__ -warn-xref -xref -Odoc -Ncircle -html $$file ) ; \
	   rm -f $(DOCS) ; \
	done
#
# Create the index using /home/jelson/cxref/cxref
#
	@echo Indexing
	@( cd . ; /home/jelson/cxref/cxref -D__CXREF__ -index-all -Odoc -Ncircle -html )
	@rm -f $(DOCS)
#
# Make html files for the .h files
#
	@echo Creating .h.html files...
	@for file in *.h ; do \
	  echo $$file ; \
	  cat /home/jelson/mud/htmlh-head $$file /home/jelson/mud/htmlh-tail > doc/$$file.html ; \
	done
# Copy over to the html directory
	cp doc/*.html /home/jelson/public_html/circle/cdp/cxref
	chmod 644 /home/jelson/public_html/circle/cdp/cxref/*.html

# Dependencies for the object files (automagically generated with
# gcc -MM)

include Depend
