/* 
 * File:   aaltasolver.h
 * Author: Jianwen Li
 * Note: An inheritance class from Minisat::Solver for Aalta use 
 * Created on August 15, 2017
 */
 
#ifndef AALTA_SOLVER_H
#define	AALTA_SOLVER_H

#include "minisat/core/Solver.h"
#include <vector>

namespace aalta
{
	class AaltaSolver : public Minisat::Solver 
	{
	public:
		AaltaSolver () {}
		AaltaSolver (bool verbose) : verbose_ (verbose) {} 
		bool verbose_;
		
		Minisat::vec<Minisat::Lit> assumption_;  //Assumption for SAT solver
		
		//functions
		bool solve_assumption ();
		std::vector<int> get_model ();    //get the model from SAT solver
 		std::vector<int> get_uc ();       //get UC from SAT solver
		
		void add_clause (int);
 		void add_clause (int, int);
 		void add_clause (int, int, int);
 		void add_clause (int, int, int, int);
 		void add_clause (std::vector<int>&);
 	
 		Minisat::Lit SAT_lit (int id); //create the Lit used in SAT solver for the id.
 		int lit_id (Minisat::Lit);  //return the id of SAT lit
 		
 		
 		//printers
 		void print_clauses ();
 		
 		//l <-> r
 		inline void add_equivalence (int l, int r)
 		{
 			add_clause (-l, r);
 			add_clause (l, -r);
 		}
 	
 		//l <-> r1 /\ r2
 		inline void add_equivalence (int l, int r1, int r2)
 		{
 			add_clause (-l, r1);
 			add_clause (-l, r2);
 			add_clause (l, -r1, -r2);
 		}
 	
 		//l<-> r1 /\ r2 /\ r3
 		inline void add_equivalence (int l, int r1, int r2, int r3)
 		{
 			add_clause (-l, r1);
 			add_clause (-l, r2);
 			add_clause (-l, r3);
 			add_clause (l, -r1, -r2, -r3);
 		}
	};
}

#endif
