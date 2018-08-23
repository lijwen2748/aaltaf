# Parser Makefile
# need flex bison

PARSERCFILES =	../ltllexer.c ../ltlparser.c
PARSERHFILES =	../ltllexer.h ../ltlparser.h
ALLCFILES   =	$(PARSERCFILES) ../ltl_formula.c ../trans.c ../utility.cpp ltl2smvspec.cpp
CC	    =   g++
CDEBUG	    =	-g
INCLUDE	    =   -I../

ltl2smvspec:	$(PARSERCFILES)

ltllexer.c:	../grammar/ltllexer.l
		flex ../grammar/ltllexer.l

ltlparser.c:	../grammar/ltlparser.y
		bison ../grammar/ltlparser.y

run:	$(ALLCFILES)
	$(CC) $(INCLUDE) $(ALLCFILES) -o ltl2smvspec

run_debug:  $(ALLCFILES)
	    $(CC) $(INCLUDE) $(CDEBUG) $(ALLCFILES) -o ltl2smvspec

run_release:	run

clean:
	rm -f *.o *~ $(PARSERHFILES) $(PARSERCFILES) ltl2smvspec

