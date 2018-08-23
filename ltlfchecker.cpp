/* 
 * File:   ltlfchecker.cpp
 * Author: Jianwen Li
 * Note: SAT-based LTLf satisfiability checking
 * Created on June 26, 2017
 */

#include "ltlfchecker.h"
#include "formula/olg_formula.h"
#include <iostream>

using namespace std;

namespace aalta
{
	bool LTLfChecker::check ()
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
		
		return dfs_check (to_check_);
	}
	
	//check sat by olg heuristics
	LTLfChecker::RES LTLfChecker::olg_sat (aalta_formula* f, bool keep_evidence)
	{
  		olg_formula olg (f);
		if (olg.sat ())
		{
			if (evidence_ != NULL && keep_evidence)
				evidence_->push (olg);
			return SAT;
		}
  		return UNSAT;
	}
	
	//check unsat by olg heuristics
	LTLfChecker::RES LTLfChecker::olg_unsat (aalta_formula* f, bool keep_evidence)
	{
  		olg_formula olg (f);
		if (olg.unsat ())
			return SAT;
  		return UNSAT;
	}
	
	
	LTLfChecker::RES LTLfChecker::check_with_heuristics ()
	{
  		if(to_check_->is_global()) //for global formulas
  		{
  			aalta_formula *afg = to_check_->ofg ();
			if (verbose_)
				cout << "Heuristics for Global formulas:\n";
			return olg_sat (afg, true);
  		}
	
	
		if (contain_global (to_check_))
		{
			if (verbose_)
				cout << "Heuristics for global part unsat:\n";
			if (global_part_unsat (to_check_))
				return UNSAT;
		}
	/*
		if (verbose_)
			cout << "Heuristics for computing off:\n";
    	aalta_formula *aaf = to_check_->off();
		if (olg_sat (aaf))
			return SAT;
  */
  		if(to_check_->is_wnext_free()) //weak Next free
  		{
			if (verbose_)
				cout << "Heuristics for LTL unsatisfiability checking\n";
			if (olg_unsat (to_check_, false) == SAT)
				return UNSAT;
  		}
  		return UNKNOW;
	}
	
	bool LTLfChecker::dfs_check (aalta_formula* f)
	{
		
		visited_.push_back (f);
		if (detect_unsat ())
			return false;
		if (sat_once (f))
		{
			if (verbose_)
				cout << "sat once is true, return from here\n";
			return true;
		}
		else if (f->is_global ())
		{
			visited_.pop_back ();
			push_formula_to_explored (f);
			return false;;
		}
		
		//heuristics: if the global parts of f is unsat, then f is unsat
	/*
	if(contain_global (f))
	{
		if (global_part_unsat (f))
		{
			if (verbose_)
				cout << "Global parts are unsat" << endl;
			visited_.pop_back ();
			push_uc_to_explored ();
			return false;
		}
	}
	*/
		
		
		//The SAT solver cannot return f as well
		push_formula_to_explored (f);
		
		while (true)
		{
			if (detect_unsat ())
				return false;
			Transition* t = get_one_transition_from (f);
			
			if (t != NULL)
			{
				if (verbose_)
					cout << "getting transition:\n" << t->label ()->to_string() << " -> " << t->next ()->to_string () << endl;
				if (evidence_ != NULL)
					evidence_->push (t->label ());
				if (dfs_check (t->next ()))
				{
					delete t;
					return true;
				}
				if (evidence_ != NULL)
					evidence_ ->pop_back ();
			}
			else //cannot get new states, that means f is not used anymore
			{
				if (verbose_)
					cout << "get a null transition\n";
				visited_.pop_back ();
				push_uc_to_explored ();
				delete t;
				return false;
			}
		}
		visited_.pop_back ();
		return false;
	}
        
	//check whether there is a conjunct of \@ f is global
	bool LTLfChecker::contain_global (aalta_formula *f)
	{
		if (f->is_global ())
			return true;
		else if (f->oper () == aalta_formula::And)
			return contain_global (f->l_af ()) || contain_global (f->r_af ());
		return false;
	}
	
	bool LTLfChecker::global_part_unsat (aalta_formula *f)
	{
		bool ret = solver_->solve_with_global_assumption (f);
		if (!ret)
			return true;
		return false;
	}
	
	Transition* LTLfChecker::get_one_transition_from (aalta_formula* f)
	{
		bool ret = solver_->solve_by_assumption (f);
		if (ret)
		{
			Transition* res = solver_->get_transition ();
			return res;
		}
		return NULL;
	}
	
	void LTLfChecker::push_formula_to_explored (aalta_formula* f)
	{
		solver_->block_formula (f);
	}
	
	void LTLfChecker::push_uc_to_explored ()
	{
		solver_->block_uc ();
	}
	
	bool LTLfChecker::sat_once (aalta_formula *f)
	{
		if (solver_->check_tail (f))
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
	

	void LTLfChecker::print_evidence ()
	{
		assert (evidence_ != NULL);
		evidence_->print ();
	}
	
	void LTLfChecker::print_formulas_id (aalta_formula* f)
	{
		if (f == NULL)
			return;
		cout << f->id () << " : " << f->to_string () << endl;
		print_formulas_id (f->l_af ());
		print_formulas_id (f->r_af ());
	}
	
	
	
	
}
 
