# Parser Makefile
# need flex bison

PARSERCFILES =	../ltllexer.c ../ltlparser.c
PARSERHFILES =	../ltllexer.h ../ltlparser.h
ALLCFILES   =	$(PARSERCFILES) ../ltl_formula.c ../trans.c main.c
CC	    =   g++
CDEBUG	    =	-g

parser:	$(PARSERCFILES)

../ltllexer.c:	ltllexer.l
		flex ltllexer.l

../ltlparser.c:	ltlparser.y
		bison ltlparser.y

run:	$(ALLCFILES)
	$(CC) $(ALLCFILES) -o parser

run_debug:  $(ALLCFILES)
	    $(CC) $(CDEBUG) $(ALLCFILES) -o parser

run_release:	run

clean:
	rm -f *.o *~ $(PARSERHFILES) $(PARSERCFILES) parser

