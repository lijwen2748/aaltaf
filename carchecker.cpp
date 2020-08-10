/* 
 * File:   carchecker.cpp
 * Author: Jianwen Li
 * Note: SAT-based LTLf satisfiability checking based on CAR
 * Created on August 10, 2017
 */
 
 #include "carchecker.h"
 #include "formula/olg_formula.h"
 #include <iostream>
 using namespace std;
 using namespace Minisat;
 
 namespace aalta
 {
 
 	 void InvSolver::create_flag_for_frame (int frame_level)
	 {
		assert (frame_flags_.size () == frame_level);
		frame_flags_.push_back (++flag_id_);
	 }
 	 void InvSolver::add_clauses_for_frame (std::vector<int>& uc, int frame_level)
 	 {
 	 	assert (frame_level < frame_flags_.size ());
 	 	std::vector<int> v;
 	 	//if v is (a1, a2, ..., an), create clause (flag, -a1, -a2, ..., -an)
 	 	v.push_back (frame_flags_[frame_level]);
 	 	for (int i = 0; i < uc.size (); i ++)
 			v.push_back (-uc[i]);
 		add_clause (v);
 		
 		//create clauses (-flag, a1),(-flag, a2) ... (-flag, an)
 		v.clear ();
 		for (int i = 0; i < uc.size (); i ++)
 			add_clause (-frame_flags_[frame_level], uc[i]);
 	 }
 	 
 	 
 	 
 	 bool InvSolver::solve_with_assumption (int frame_level)
 	 {
 	 	assumption_.clear ();
 	 	for (int i = 0; i < frame_level; i ++)
 	 		assumption_.push (SAT_lit (frame_flags_[i]));
 	 	assumption_.push (SAT_lit (-frame_flags_[frame_level]));
 	 	print_clauses ();
 	 	return solve_assumption ();
 	 }
 	 
 	void InvSolver::update_assumption_for_constraint (int id)
 	{
 		assumption_.push (SAT_lit (id));
 	}
 	
 	void InvSolver::disable_frame_and ()
 	{
 		Lit l = assumption_.last ();
 		assumption_.pop ();
 		assumption_.push (~l);
 	}
	
	LTLfChecker::RES CARChecker::check_with_heuristics ()
	{
  		if(to_check_->is_wnext_free()) //weak Next free
  		{  
			if (verbose_)
				cout << "Heuristics for LTL unsatisfiability checking\n";
			if (olg_unsat (to_check_, false) == SAT)
				return UNSAT;
  		}
		
		if (partial_unsat ())
		{
			if (verbose_)
			{
				cout << "return from partial_unsat\n";
			}
			return UNSAT;
		}
		
		return UNKNOW;
	}
	
	aalta_formula* CARChecker::target_atom (aalta_formula *g)
	{
		if (!g->is_global ())
			return NULL;
		aalta_formula *f = NULL;
		af_prt_set formula_set = g->r_af ()->to_or_set ();
		int count = 0;
		for (af_prt_set::iterator it = formula_set.begin (); it != formula_set.end (); it ++)
		{
			if ((*it)->oper () == aalta_formula::Not)
			{
				count ++;
				f = *it;
			}			
			else if ((*it)->oper () > aalta_formula::Undefined)	
				return NULL;
		}
		if (count == 1)
			return f->r_af ();
		return NULL;
	}
	aalta_formula* CARChecker::extract_for_partial_unsat ()
	{
		af_prt_set formula_set = to_check_->to_set ();
		for (af_prt_set::iterator it = formula_set.begin (); it != formula_set.end (); it ++)
		{
			aalta_formula *f = target_atom (*it);
			if (f != NULL)
			{
				if (formula_set.find (f) != formula_set.end ())
					return aalta_formula (aalta_formula::And, f, *it).unique ();
			}
		}
		return NULL;
	}
	
	bool CARChecker::partial_unsat ()
	{
		aalta_formula *f = extract_for_partial_unsat ();
		if (f == NULL)
			return false;
		CARChecker checker (f, verbose_);
		if (!checker.car_check (f))
			return true;
		return false;
	}
 
 	 bool CARChecker::check ()
 	 {
		 if (verbose_)
		 {
		 	cout << "Checking formula: \n" << to_check_->to_string() << endl;
		 	print_formulas_id (to_check_);
		 }
		
		 if (to_check_->oper () == aalta_formula::True)
		 {
			if (evidence_ != NULL)
				evidence_->push (true);
			return true;
		 }
		 if (to_check_->oper () == aalta_formula::False)
		 	return false;
		
		
		 RES ret = check_with_heuristics ();
		 if (ret != UNKNOW)
		 	return (ret == SAT ? true : false);
		 
		
		 return car_check (to_check_);
 	 }
 	
 	bool CARChecker::car_check (aalta_formula* f)
 	{
 		if (sat_once (f))
		{
			if (verbose_)
				cout << "sat once is true, return from here\n";
			return true;
		}
		else if (f->is_global ())
		{
			push_formula_to_explored (f);
			return false;
		}
 		
 		//initialize the first frame	
 		std::vector<int> uc = get_selected_uc ();
 		tmp_frame_.push_back (uc);
 		add_new_frame ();
 		
 		int frame_level = 0;
 		while (true)
 		{
 			tmp_frame_.clear ();
 			if (try_satisfy (f, frame_level))
 				return true;
 			if (inv_found (frame_level))
 				return false;
 			add_new_frame ();
 			frame_level ++;
 		}
 		return false;	
 	}
 	
 	void CARChecker::add_new_frame ()
 	{
 		frames_.push_back (tmp_frame_);
 		solver_add_new_frame ();
 		if (verbose_)
 		{
 			cout << "New frame:\n";
 			print_frames ();
 			//print_solver_clauses ();
 		}
 	}
 	
 	void CARChecker::solver_add_new_frame ()
 	{
 		int frame_level = frames_.size () - 1;
 		solver_->create_flag_for_frame (frame_level);
 		//inv_solver_->create_flag_for_frame (frame_level);
 		for (int i = 0; i < tmp_frame_.size (); i ++)
 			solver_add_frame_element (tmp_frame_[i], frame_level);
 		
 	}
 	
 	bool CARChecker::try_satisfy (aalta_formula *f, int frame_level)
 	{
 		while (try_satisfy_at (f, frame_level))
 		{
 			Transition *t = get_transition ();
 			if (evidence_ != NULL)
 				evidence_ -> push (t->label ());
 			if (frame_level == 0)
 			{
 				if (sat_once (t->next ()))
 					return true;
 				else
 				{
 					std::vector<int> uc = get_selected_uc ();  
 					add_frame_element (frame_level, uc);
 					continue;
 				}
 			}
  			if (try_satisfy (t->next(), frame_level-1))
 				return true;
 			if (evidence_ != NULL)
 				evidence_ -> pop_back ();
 		}
 		std::vector<int> uc = get_selected_uc (); 
 		add_frame_element (frame_level+1, uc);
 		return false;
 	}
 	
 	
 	void CARChecker::add_frame_element (int frame_level, std::vector<int>& uc)
 	{
 		assert (!uc.empty ());
 		if (frame_level == frames_.size()){
  			tmp_frame_.push_back (uc);
 		}
 		else
 		{
 			if (verbose_)
 			{
 				cout << "Update Frame " << frame_level << endl;
 			}
 			frames_[frame_level].push_back (uc);
 			solver_add_frame_element (uc, frame_level);
 		}
 	}
 	
 	
 	
 	bool CARChecker::inv_found (int frame_level)
 	{
 		bool res = false;
 		inv_solver_ = new InvSolver (to_check_->id (), verbose_);
 		for (int i = 0; i < frames_.size (); i ++)
 		{
 			if (inv_found_at (i))
 			{
 				if (verbose_)
 					cout << "Inv found at level " << i << endl << "frames_ size is " << frames_.size () << endl;
 				res = true;
 				break;
 			}
 		}
 		delete inv_solver_;
 		return res;
 	}
 	
 	bool CARChecker::inv_found_at (int frame_level)
 	{
 		if (frame_level == 0)
 		{
 			add_clauses_to_inv_solver_for_frame_or (frames_[frame_level]);
 			return false;
 		}
 		return solve_inv_at (frame_level);
 	}
 	
 	bool CARChecker::solve_inv_at (int frame_level)
 	{
 		add_clauses_to_inv_solver_for_frame_and (frames_[frame_level]);
 		/*
 		if (verbose_)
 		{
 			cout << "Clauses in inv_solver_\n";
 			inv_solver_->print_clauses ();
 		}
 		*/
 		bool res = !(inv_solver_-> solve_assumption ());
 		inv_solver_-> disable_frame_and ();
 		add_clauses_to_inv_solver_for_frame_or (frames_[frame_level]);
 		return res;
 	}
 	
 	
 	void CARChecker::add_clauses_to_inv_solver_for_frame_or (Frame& frame)
 	{
 		std::vector<int> v;
 		for (int i = 0; i < frame.size (); i ++)
 		{
 			int clause_flag = inv_solver_->new_var ();
 			v.push_back (clause_flag);
 			for (int j = 0; j < frame[i].size (); j ++)
 				inv_solver_->add_clause (-clause_flag, frame[i][j]);
 		}
 		inv_solver_->add_clause (v);
 	}
 	
 	void CARChecker::add_clauses_to_inv_solver_for_frame_and (Frame& frame)
 	{
 		int frame_flag = inv_solver_->new_var ();
 		for (int i = 0; i < frame.size (); i ++)
 		{
 			std::vector<int> v;
 			for (int j = 0; j < frame[i].size (); j ++)
 				v.push_back (-frame[i][j]);
 			v.push_back (-frame_flag);
 			inv_solver_->add_clause (v);
 		}
 		inv_solver_-> update_assumption_for_constraint (frame_flag);
 	}
 	
 	bool CARChecker::sat_once (aalta_formula* f)
 	{
 		if (solver_->check_final (f))
		{
			if (evidence_ != NULL)
			{
				Transition *t = solver_->get_transition ();
				assert (t != NULL);
				evidence_->push (t->label ());
				delete t;
			}
			return true;
		}
		return false;
 	}
 	
 	
 	void CARChecker::print_solver_clauses ()
 	{
 		cout << "Clauses in solver_\n";
 		solver_->print_clauses ();
 	}
 		
 	void CARChecker::print_frames ()
 	{
 		for (int i = 0; i < frames_.size (); i ++)
 		{
 			cout << "/////frame " << i << "//////" << endl;
 			print_frame (i);
 		}
 	}
 	
 	void CARChecker::print_frame (int level)
 	{
 		Frame& frame = frames_[level];
 		for (int i = 0; i < frame.size (); i ++)
 		{
 			for (int j = 0; j < frame[i].size (); j ++)
 				cout << frame[i][j] << ", ";
 			cout << endl;
 		}
 	}
 	
 	
 	
 }
