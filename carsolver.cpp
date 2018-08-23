/* 
 * File:   carsolver.cpp
 * Author: Jianwen Li
 * Note: An interface for SAT solver
 * Created on August 14, 2017
 */
 
 #include "carsolver.h"
 #include <iostream>
 #include <assert.h>
 using namespace std;
 
 
 namespace aalta
 {
 	bool CARSolver::solve_with_assumption (aalta_formula* f, int frame_level)
 	{
 		assert (frame_level < frame_flags_.size ());
 		assert (!unsat_forever_);
 		set_selected_assumption (f);
		get_assumption_from (f, false);
		assumption_.push (SAT_lit (frame_flags_[frame_level]));
		return solve_assumption ();
 	}
 	
 	void CARSolver::add_clause_for_frame (std::vector<int>& uc, int frame_level)
 	{
 		assert (frame_level < frame_flags_.size ());
 		af_prt_set ands = formula_set_of (uc);
 		//if there is a conjuct A in f such that (A, X A) is not founded in X_map_, then discard blocking f
 		if (block_discard_able (ands))
 			return;
 		std::vector<int> v;
 		for (af_prt_set::const_iterator it = ands.begin (); it != ands.end (); it ++)
 			v.push_back (-SAT_id_of_next (*it));
 		v.push_back (-(frame_flags_[frame_level]));
 		add_clause (v);
 	}
 	
 	void CARSolver::create_flag_for_frame (int frame_level)
	{
		assert (frame_flags_.size () == frame_level);
		frame_flags_.push_back (++max_used_id_);
	}
	
	bool CARSolver::check_final (aalta_formula *f)
	{
		set_selected_assumption (f);
		return Solver::check_tail (f);
	}
	
	std::vector<int> CARSolver::get_selected_uc ()
	{
		std::vector<int> uc = get_uc ();
		std::vector<int> res;
		for (int i = 0; i < uc.size (); i ++)
		{
			if (selected_assumption_.find (uc[i]) != selected_assumption_.end ())
				res.push_back (uc[i]);
		}
		if (verbose_)
		{
			cout << "get selected uc: " << endl;
			for (int i = 0; i < res.size (); i ++)
				cout << res[i] << ", ";
			cout << endl;
		}
		assert (!res.empty ());
		return res;
	} 
	
	void CARSolver::set_selected_assumption (aalta_formula *f)
	{
		selected_assumption_.clear ();
		af_prt_set ands = f->to_set ();
		for (af_prt_set::iterator it = ands.begin (); it != ands.end (); it ++)
		{
			selected_assumption_.insert (SAT_id (*it));		
		}
	}
	
	
 }
