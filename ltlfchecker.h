/* 
 * File:   ltlfchecker.h
 * Author: Jianwen Li
 * Note: SAT-based LTLf satisfiability checking
 * Created on June 26, 2017
 */
 
#ifndef LTLF_CHECKER_H
#define	LTLF_CHECKER_H

#include "formula/aalta_formula.h"
#include "solver.h"
#include "carsolver.h"
#include "debug.h"
#include "evidence.h"

namespace aalta
{

class LTLfChecker
{
public:
	LTLfChecker () {};
	LTLfChecker (aalta_formula *f, bool verbose = false, bool evidence = false) : to_check_ (f), verbose_ (verbose) 
	{
		solver_ = new CARSolver (f, verbose);
		if (evidence)
			evidence_ = new Evidence ();
		else
		    evidence_ = NULL;
	}
	void create_solver () {}
	~LTLfChecker () 
	{
		if (solver_ != NULL) delete solver_;
		if (evidence_ != NULL) delete evidence_;
	}
	bool check ();
	void print_evidence ();
	
protected:
	//flags
	bool verbose_;  //default is false
	CARSolver* solver_;  //SAT solver for computing next states
					  //Note: Currently we only use one solver
	
	enum RES {UNKNOW = -1, UNSAT, SAT};
	aalta_formula* to_check_;
	//visited_/traces_ is updated during the search process. 
	//If the checking result is SAT, traces_ finally store a counterexample
	std::vector<aalta_formula*> visited_;
	Evidence* evidence_;
	
	
	
	
	//////////functions
	bool sat_once (aalta_formula* f);   //check whether the formula can be satisfied in one step (the terminating condition of checking)
	RES check_with_heuristics ();
	bool contain_global (aalta_formula *);
	bool global_part_unsat (aalta_formula*);
	bool dfs_check (aalta_formula *f);
	Transition* get_one_transition_from (aalta_formula*);
	void push_formula_to_explored (aalta_formula* f);
	void push_uc_to_explored ();
	//check the satisfiability via olg_formula heuristics
	RES olg_sat (aalta_formula* f, bool keep_evidence);
	//check the unsatisfiability via olg_formula heuristics
	RES olg_unsat (aalta_formula* f, bool keep_evidence);
	
	void print_formulas_id (aalta_formula *);
	
	inline bool detect_unsat () {return solver_->unsat_forever ();}
	
	
};

}

#endif

