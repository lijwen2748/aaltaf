
FORMULAFILES =	formula/aalta_formula.cpp formula/olg_formula.cpp formula/olg_item.cpp
	
PARSERFILES  =	ltlparser/ltl_formula.c ltlparser/ltllexer.c ltlparser/ltlparser.c ltlparser/trans.c 

UTILFILES    =	util/utility.cpp utility.cpp

SOLVER		= minisat/core/Solver.cc aaltasolver.cpp solver.cpp carsolver.cpp 

CHECKING	= ltlfchecker.cpp carchecker.cpp  

OTHER		= evidence.cpp


ALLFILES     =	$(CHECKING) $(SOLVER) $(FORMULAFILES) $(PARSERFILES) $(UTILFILES) $(OTHER) main.cpp


CC	    =   g++
FLAG    = -I./  -I./minisat/  -D __STDC_LIMIT_MACROS -D __STDC_FORMAT_MACROS 
DEBUGFLAG   =	-D DEBUG -g -pg
RELEASEFLAG =	-O2 

aaltaf :	release

ltlparser/ltllexer.c :
	ltlparser/grammar/ltllexer.l
	flex ltlparser/grammar/ltllexer.l

ltlparser/ltlparser.c :
	ltlparser/grammar/ltlparser.y
	bison ltlparser/grammar/ltlparser.y
	
	

.PHONY :    release debug clean

release :   $(ALLFILES)
	    $(CC) $(FLAG) $(RELEASEFLAG) $(ALLFILES) -lz -o aaltaf

debug :	$(ALLFILES)
	$(CC) $(FLAG) $(DEBUGFLAG) $(ALLFILES) -lz -o aaltaf

clean :
	rm -f *.o *~ aaltaf
