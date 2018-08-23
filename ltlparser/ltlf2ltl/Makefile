# Parser Makefile
# need flex bison

PARSERCFILES =	../ltllexer.c ../ltlparser.c
PARSERHFILES =	../ltllexer.h ../ltlparser.h
ALLCFILES   =	$(PARSERCFILES) ../ltl_formula.c ../trans.c ../utility.cpp ltlf2ltl.cpp
CC	    =   g++
CDEBUG	    =	-g
INCLUDE	    =   -I../

ltlf2ltl:	$(PARSERCFILES)

ltllexer.c:	../grammar/ltllexer.l
		flex ../grammar/ltllexer.l

ltlparser.c:	../grammar/ltlparser.y
		bison ../grammar/ltlparser.y

run:	$(ALLCFILES)
	$(CC) $(INCLUDE) $(ALLCFILES) -o ltlf2ltl

run_debug:  $(ALLCFILES)
	    $(CC) $(INCLUDE) $(CDEBUG) $(ALLCFILES) -o ltl2smvspec

run_release:	run

clean:
	rm -f *.o *~ $(PARSERHFILES) $(PARSERCFILES) ltl2smvspec

