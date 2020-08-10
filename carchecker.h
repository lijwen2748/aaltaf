/* 
 * File:   carchecker.h
 * Author: Jianwen Li
 * Note: SAT-based LTLf satisfiability checking based on CAR
 * Created on August 10, 2017
 */
 
#ifndef CAR_CHECKER_H
#define	CAR_CHECKER_H

#include "ltlfchecker.h"
#include "carsolver.h"
#include "aaltasolver.h"
#include "formula/aalta_formula.h"
#include <vector>

namespace aalta
{
	class InvSolver : public AaltaSolver 
	{
	public:
		InvSolver (int id, bool verbose = false) : AaltaSolver (verbose), flag_id_ (id) {}
		//functions
		void create_flag_for_frame (int frame_level);
		void add_clauses_for_frame (std::vector<int>& uc, int frame_level);
		bool solve_with_assumption (int frame_level);
		inline int new_var () {return ++flag_id_;}
		void update_assumption_for_constraint (int id);
 		void disable_frame_and ();
		
	protected:
		//ids in SAT solver to represent each frame, i.e. frame_flags[i] represents frame i;
		std::vector<int> frame_flags_; 
		//the flag id to represent the flags of each frame
		int flag_id_;  
		
		
		
		
	
	};
	
	class CARChecker : public LTLfChecker 
	{
	public:
		
		CARChecker (aalta_formula *f, bool verbose = false, bool evidence = false) : LTLfChecker (f, verbose, evidence), inv_solver_ (NULL)
		{	
			//inv_solver_ = new InvSolver (to_check_->id(), verbose_);
		}

		~CARChecker () 
		{
			//if (inv_solver_ != NULL) delete inv_solver_;
		}
		
		bool check ();
		
		//printers
 		void print_solver_clauses ();
 		
 		void print_frames ();
		
	private:
		//members
		typedef std::vector<std::vector<int> > Frame;
		std::vector<Frame> frames_;  //frame sequence
		Frame tmp_frame_;  //temporal frame to store the UCs before it is pushed into frames_
		InvSolver *inv_solver_; //SAT solver to check invariant
		//CARSolver *solver_;

		
		//functions
		//main checking function
		bool car_check (aalta_formula *f);
		//try to find a model with the length of \@frame_level
		bool try_satisfy (aalta_formula *f, int frame_level);
		//add \@uc to frame \@frame_level
		void add_frame_element (int frame_level, std::vector<int>& uc);
		//check whether an invariant can be found in up to \@frame_level steps.
		bool inv_found (int frame_level);
		//add a new frame to frames_
		void add_new_frame ();
		//add a new frame to SAT solver
		void solver_add_new_frame ();
		//check whether an invariant is found at frame \@ i
		bool inv_found_at (int i);
		
		//specilized heursitics
		typedef aalta_formula::af_prt_set af_prt_set;
		RES check_with_heuristics ();
		bool partial_unsat ();
		aalta_formula* target_atom (aalta_formula *g);
		aalta_formula* extract_for_partial_unsat ();
		
		//get the UC of solver_
		inline std::vector<int> get_selected_uc () 
		{
			return solver_->get_selected_uc ();
		}
		
		//check whether \@f has a next state that can block constraints at level \@frame_level
		inline bool try_satisfy_at (aalta_formula *f, int frame_level)
 		{
 			return solver_->solve_with_assumption (f, frame_level); //need specialized?
 		}
 	
 		//get a transition from SAT solver.
 		inline Transition* get_transition ()
 		{
 			return solver_->get_transition ();  //need specialized?
 		}
 		
 		//add the frame \@frame_level a new element to block \@uc in next states
 		inline void solver_add_frame_element (std::vector<int>& uc, int frame_level)
 		{
 			solver_->add_clause_for_frame (uc, frame_level);
 			//inv_solver_->add_clauses_for_frame (uc, frame_level);
 		}
 		
 		//check whether \@ f can be a final state
 		bool sat_once (aalta_formula* f);
 		
 		//handle inv_solver_
 		bool solve_inv_at (int frame_level);
 		void add_clauses_to_inv_solver (int level);
 		void add_clauses_to_inv_solver_for_frame_or (Frame& frame);
 		void add_clauses_to_inv_solver_for_frame_and (Frame& frame);
 		
 		
 		void print_frame (int);

	};
}

#endif
