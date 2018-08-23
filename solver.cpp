/* 
 * File:   solver.h
 * Author: Jianwen Li
 * Note: An interface for SAT solver
 * Created on June 26, 2017
 */
 
 #include "solver.h"
 #include <iostream>
 #include <assert.h>
 #include "utility.h"
 
 using namespace std;
 using namespace Minisat;
 
 namespace aalta
 {
	 Solver::Solver (aalta_formula *f, bool verbose, bool partial_on, bool uc_on) :
	 AaltaSolver (verbose), uc_on_ (uc_on), partial_on_ (partial_on), unsat_forever_ (false)
	 {
	 	max_used_id_ = f->id ();
		tail_ = aalta_formula::TAIL ()->id ();
		build_X_map_priliminary (f);
		//tail_ = ++max_used_id_;
		generate_clauses (f);
		coi_set_up (f);
		if (verbose_)
		{
			cout << "id of input formula is " << f->id () << endl;
			cout << "tail_ is " << tail_ << endl;
			cout << "max_used_id_ is " << max_used_id_ << endl;
		  cout << "//////////////////////////////////////////////////////\n";
			print_formula_map ();			
			cout << "//////////////////////////////////////////////////////\n";
			print_x_map ();
			cout << "//////////////////////////////////////////////////////print coi ////////////////////////\n";
			print_coi ();
			cout << "//////////////////////////////////////////////////////\n";
			print_clauses ();
			cout << "//////////////////////////////////////////////////////\n";
		}
	 }
	 
	 //generate clauses of SAT solver
	 void Solver::generate_clauses (aalta_formula* f)
	 {
		 add_clauses_for (f);
		 add_X_conflicts ();
	 }
	 
	 
 	//add clauses for the formula f into SAT solver
 	void Solver::add_clauses_for (aalta_formula *f)
 	{
 		//We assume that 
 		//1) the id of f is greater than that of its any subformula
 		//2) atoms and X subformulas are considered propositions
 		//3) be careful about !a, we should encode as -id (a) rather than id (!a)
 		//4) for a Global formula Gf, we directly add id(Gf) into the clauses, and will not consider it in assumptions
 		//We also build the X_map and formula_map during the process of adding clauses 
 		assert (f->oper () != aalta_formula::WNext);
 		if (clauses_added (f))
 			return; 
 		int id, x_id;
 		switch (f->oper ())
 		{
 			case aalta_formula::True:
 			case aalta_formula::False:
 				break;
 			case aalta_formula::Not:
 				build_formula_map (f);
 				add_clauses_for (f->r_af ());
 				mark_clauses_added (f);
 				break;
 			case aalta_formula::Next:
 				//build_X_map (f);
 				build_formula_map (f);
				//add f ->!Tail 
				//add_clause (-SAT_id (f), -tail_);
				//dout << "adding clause (" << -SAT_id (f) << " " << -tail_ << ")" << endl;

 				add_clauses_for (f->r_af ());
 				mark_clauses_added (f);
 				break;
 			case aalta_formula::Until:
 			//A U B = B \/ (A /\ !Tail /\ X (A U B))
 				build_X_map (f);
 				build_formula_map (f);
 				id = ++max_used_id_;
 				add_equivalence (-SAT_id (f), -SAT_id (f->r_af ()), -id);
 				dout << "adding equivalence " << -SAT_id (f) << " <-> " << -SAT_id (f->r_af ()) << " & " << -id << endl;
 				
 				if (!f->is_future ())
 				{
 					add_equivalence (id, SAT_id (f->l_af ()), -tail_, SAT_id_of_next (f));
 					dout << "adding equivalence " << id << " <-> " << -SAT_id (f->l_af ()) << " & " << -tail_ << " & " << SAT_id_of_next (f) << endl;
				
 					add_clauses_for (f->l_af ());
 					add_clauses_for (f->r_af ());
 				}
 				else //F B = B \/ (!Tail /\ X (F B))
 				{
 					add_equivalence (id, -tail_, SAT_id_of_next (f));
 					dout << "adding equivalence " << id << " <-> " << -tail_ << " & " << SAT_id_of_next (f) << endl;

 					add_clauses_for (f->r_af ());
 				}
 				mark_clauses_added (f);
 				break;
 			case aalta_formula::Release:
 			//A R B = B /\ (A \/ Tail \/ X (A R B))
 				build_X_map (f);
 				build_formula_map (f);
 				id = ++max_used_id_;
 				add_equivalence (SAT_id (f), SAT_id (f->r_af ()), id);
 				dout << "adding equivalence " << SAT_id (f) << " <-> " << SAT_id (f->r_af ()) << " & " << id << endl;
 				
 				if (!f->is_globally ())
 				{
 					add_equivalence (-id, -SAT_id (f->l_af ()), -tail_, -SAT_id_of_next (f));
 					dout << "adding equivalence " << -id << " <-> " << -SAT_id (f->l_af ()) << " & " << -tail_ << " & " << -SAT_id_of_next (f) << endl;
				
 					add_clauses_for (f->l_af ());
 					add_clauses_for (f->r_af ());
 				}
 				else //G B = B /\ (Tail \/ X (G B))
 				{				
 					add_equivalence (-id, -tail_, -SAT_id_of_next (f));
 					dout << "adding equivalence " << -id << " <-> " << -tail_ << " & " << -SAT_id_of_next (f) << endl;

 					add_clauses_for (f->r_af ());
 				}
 				mark_clauses_added (f);
 				break;
 			
 			case aalta_formula::And:
				build_formula_map (f);
				add_equivalence (SAT_id (f), SAT_id (f->l_af ()), SAT_id (f->r_af ()));
				dout << "adding equivalence " << SAT_id (f) << " <-> " << SAT_id (f->l_af ()) << " & " << SAT_id (f->r_af ()) << endl;
				add_clauses_for (f->l_af ());
 				add_clauses_for (f->r_af ());
 				mark_clauses_added (f);
 				break;
 			case aalta_formula::Or:
 				build_formula_map (f);
 				add_equivalence (-SAT_id (f), -SAT_id (f->l_af ()), -SAT_id (f->r_af ()));
 				dout << "adding equivalence " << -SAT_id (f) << " <-> " << -SAT_id (f->l_af ()) << " & " << -SAT_id (f->r_af ()) << endl;
				
				add_clauses_for (f->l_af ());
 				add_clauses_for (f->r_af ());
 				mark_clauses_added (f);
 				break;
 			case aalta_formula::Undefined:
 			{
 				cout << "Solver.cpp::add_clauses_for: Error reach here!\n";
 				exit (0);
 			}
 			default: //atoms
 				build_formula_map (f);
 				mark_clauses_added (f);
	 			break;
 		}
 	}
 	
 	//set up the COI map
 	void Solver::coi_set_up (aalta_formula *f)
 	{
 		std::vector<int> ids;
 		compute_full_coi (f, ids);
 		//only Until, Release, And, Or formulas need to be recorded
 		//delete from coi_map_ all ids in \@ids
 		shrink_coi (ids);
 	}
 	
 	void Solver::compute_full_coi (aalta_formula *f, std::vector<int>& ids)
 	{
 		if (coi_map_.find (f->id ()) != coi_map_.end ())
 			return;
 		//only variables and Nexts need to be recorded
 		std::vector<int> v;
 		coi_map::iterator it;
 		x_map::iterator xit;
 		v.resize (max_used_id_, 0);
 		int id = f->id ();
 		switch (f->oper ())
 		{
 			case aalta_formula::Not:  //id -> id for Literals
 				if (f->r_af () != NULL)
 				{
 					compute_full_coi (f->r_af (), ids);
 					it = coi_map_.find (f->r_af ()->id ());
 					assert (it != coi_map_.end ());
 					coi_merge (v, it->second);
 					coi_map_.insert (std::pair<int, std::vector<int> >(id, v));
 					ids.push_back (id);
 					break;
 				}
 			
 			case aalta_formula::Until:
 			case aalta_formula::Release:
 			//add Xf in COI for Until/Release formula f
 				xit = X_map_.find (SAT_id (f));
 				assert (xit != X_map_.end ());
 				v [xit->second -1] = 1;
 			case aalta_formula::And:
 			case aalta_formula::Or:
 				compute_full_coi (f->l_af (), ids);
 				it = coi_map_.find (f->l_af ()->id ());
 				assert (it != coi_map_.end ());
 				coi_merge (v, it->second);
 				
 				compute_full_coi (f->r_af (), ids);
 				it = coi_map_.find (f->r_af ()->id ());
 				assert (it != coi_map_.end ());
 				coi_merge (v, it->second);
 				coi_map_.insert (std::pair<int, std::vector<int> >(id, v));
 				if (f->oper () == aalta_formula::And)
 					ids.push_back (id);
 				break;
 			case aalta_formula::Undefined:
 			{
 				cout << "solver.cpp: Error reach here!\n";
 				exit (0);
 			}
 			case aalta_formula::Next:
 			default: //atoms
 				if (f->r_af () != NULL)
 					compute_full_coi (f->r_af (), ids);
 				v [id-1] = 1;
 				coi_map_.insert (std::pair<int, std::vector<int> >(id, v));
 				ids.push_back (id);
 				
 				break;
 		}
 		
 	}
 	
 	//delete from coi_map_ all ids in \@ids
 	void Solver::shrink_coi (std::vector<int>& ids)
 	{
 		for (std::vector<int>::iterator it = ids.begin (); it != ids.end (); it ++)
 			coi_map_.erase (*it);
 	}
 	
 	
 	//solve by taking the assumption of the CONJUNCTIVE formula f
	//If \@global is true, take the assumption with only global conjuncts of f
	bool Solver::solve_by_assumption (aalta_formula* f, bool global)
	{	
		assert (!unsat_forever_);
		get_assumption_from (f, global);
		return solve_assumption ();
	}

	
	//check whether the formula \@ f can be the last state (tail)
	bool Solver::check_tail (aalta_formula *f)
	{
		assert (!unsat_forever_);
		get_assumption_from (f);
		assumption_.push (SAT_lit (tail_));
		if (verbose_)
		{
			cout << "check_tail: assumption_ is" << endl;
			for (int i = 0; i < assumption_.size (); i ++)
				cout << lit_id (assumption_[i]) << ", ";
			cout << endl;
		}
		lbool ret = solveLimited (assumption_);
		if (ret == l_True)
		{
			return true;
		}	
   		else if (ret == l_Undef)
     	exit (0);
		
		/*
		if (verbose_)
		{
			std::vector<int> uc = get_uc ();
			cout << "check_tail unsat, uc is " << endl;
			for (int i = 0; i < uc.size (); i ++)
			{
				cout << uc[i] << ", ";
			}
			cout << endl;
		}
		*/
		
   		return false;
	}

	//return a pair of <current, next>, which is extracted from the model of SAT solver
	Transition* Solver::get_transition ()
	{
		std::vector<int> assign = get_model ();
		shrink_model (assign);
		std::vector<aalta_formula*> labels, nexts;
		if (verbose_)
			cout << "get assignment from SAT solver: \n";
		for (std::vector<int>::iterator it = assign.begin (); it != assign.end (); it ++)
		{
			if ((*it) == 0)
				continue;
			if (verbose_)
				cout << *it << ", ";
			aalta_formula *f = formula_of (*it);
			if (f != NULL) 
			{
				if (is_label (f))
					labels.push_back (f);
				else if (is_next (f))
					nexts.push_back (f->r_af ());
			}
			else if ((*it) > 0)//handle the variables created for Next of Unitl, Release formulas
			{
				x_reverse_map::iterator it2 = X_reverse_map_.find (*it);
				if (it2 != X_reverse_map_.end ())
					nexts.push_back (it2->second);
			}
		}
		if (verbose_)
			cout << endl;
		aalta_formula *label = formula_from (labels);
		aalta_formula *next = formula_from (nexts);
		Transition *t = new Transition (label, next);
		return t;
	}
	
	void Solver::shrink_model (std::vector<int>& assign)
	{
		//Shrinking to COI is the MUST, otherwise it may happen that \phi is in the next state of \psi, 
   		//but \phi is not a subformula of \psi.
		shrink_to_coi (assign);
   		if (partial_on_)
			shrink_to_partial (assign);
	}
	
	void Solver::shrink_to_coi (std::vector<int>& assign)
	{
		std::vector<int> coi = coi_of_assumption ();
		//std::vector<int> coi_2 = coi_of_global ();
		//coi_merge (coi, coi_2);
		for (int i = 0; i < assign.size (); i ++)
		{
			if (i < coi.size ())
			{
				if (coi [i] == 0) //is not a coi
					assign [i] = 0; 
			}
			else
				assign [i] = 0;
		}
	}
	
	void Solver::shrink_to_partial (std::vector<int>& assign)
	{
		//@TO BE DONE
	}
	
	std::vector<int> Solver::coi_of_assumption ()
	{
		std::vector<int> res;
		res.resize (nVars (), 0);
		for (int i = 0; i < assumption_.size (); i ++)
		{
			int id = lit_id (assumption_[i]);
			assert (id != 0);	
			coi_of (id, res);
		}
		return res;
	}
	
	
	void Solver::coi_of (int id, std::vector<int>& res)
	{
		coi_map::iterator it = coi_map_.find (abs (id));
		if (it != coi_map_.end ())
			coi_merge (res, it->second);
		else //check whether id represent a literal or Next
		{
			assert (res.size () >= abs (id));
			res[abs(id)-1] = 0;
			aalta_formula* f = formula_of (id);
			if (f != NULL)
			{
				//COI includes only atoms
				
				if (f->oper () > aalta_formula::Undefined || f->oper () == aalta_formula::Not 
					|| f->oper () == aalta_formula::Next)
					res[abs(id)-1] = 1;
					
			}
		}
	}
	
	void Solver::coi_merge (std::vector<int>& to, std::vector<int>& from)
	{
		if (to.size () < from.size ())
			to.resize (from.size (), 0);
		for (int i = 0; i < from.size (); i ++)
		{
			if (from [i] == 1)
				to[i] = 1;
		}
	}
	
	aalta_formula* Solver::formula_of (int id)
	{
		formula_map::iterator it = formula_map_.find (id);
		if (it != formula_map_.end ())
			return it->second;
		return NULL;
	}
	
	//set X_map_ in the input-formula level
	void Solver::build_X_map_priliminary (aalta_formula* f)
	{
		if (f->oper () == aalta_formula::Next)
		{
			if (X_map_.find (f->r_af ()->id ()) == X_map_.end ())
 				X_map_.insert (std::pair<int, int> (f->r_af ()->id (), f->id ()));
 				build_X_map_priliminary (f->r_af ());
		}
		else
		{
			if (f->l_af () != NULL)
				build_X_map_priliminary (f->l_af ());
			if (f->r_af () != NULL)
				build_X_map_priliminary (f->r_af ());
		}
	}
	
 	void Solver::build_X_map (aalta_formula *f)
 	{
 		assert (f->oper () == aalta_formula::Until || f->oper () == aalta_formula::Release);
 		if (X_map_.find (f->id ()) != X_map_.end ())
 			return;
 		X_map_.insert (std::pair<int, int> (f->id (), ++max_used_id_));
 		X_reverse_map_.insert (std::pair<int, aalta_formula*> (max_used_id_, f));
 	}
	
	int Solver::SAT_id_of_next (aalta_formula *f)
	{
		x_map::iterator it = X_map_.find (f->id ());
		assert (it != X_map_.end());
		return it->second;
	}
	
	int Solver::SAT_id_of_weak_next (aalta_formula *f)
	{
		x_map::iterator it = N_map_.find (f->id ());
		assert (it != N_map_.end());
		return it->second;
	}
 	
 	void Solver::block_formula (aalta_formula *f)
 	{
 		af_prt_set ands = f->to_set ();
 		block_elements (ands);
 	}
 	
 	void Solver::block_uc ()
 	{	
 		if (uc_on_)
 		{
 			std::vector<int> uc = get_uc ();
 			if (uc.empty ())
 			{
 				terminate_with_unsat ();
 				return;
 			}
 			af_prt_set ands = formula_set_of (uc);
 			if (ands.empty ())
 			{
 				terminate_with_unsat ();
 				return;
 			}
 			block_elements (ands);
 		}
 	}
 	
 	aalta_formula::af_prt_set Solver::formula_set_of (std::vector<int>& v)
 	{
 		af_prt_set res;
 		for (std::vector<int>::iterator it = v.begin (); it!= v.end (); it ++)
 		{
 			aalta_formula *f = formula_of (*it);
 			if (f != NULL)
 				res.insert (f);
 		}
 		return res;
 	}
 	
 	void Solver::block_elements (const af_prt_set& ands)
 	{
 		//if there is a conjuct A in f such that (A, X A) is not founded in X_map_, then discard blocking f
 		if (block_discard_able (ands))
 			return;
 		std::vector<int> v;
 		for (af_prt_set::const_iterator it = ands.begin (); it != ands.end (); it ++)
 			v.push_back (-SAT_id_of_next (*it));
 		add_clause (v);
 	}
 	
 	bool Solver::block_discard_able (const af_prt_set& ands) 
 	{
 		for (af_prt_set::const_iterator it = ands.begin (); it != ands.end (); it ++)
 		{
 			if (X_map_.find ((*it)->id ()) == X_map_.end ())
 				return true;
 		}
 		return false;
 	}
 	
 	
 	
	//set assumption_ of SAT solver from \@ f. 
	//If \@ global is true, set assumption_ with only global parts of \@ f
 	void Solver::get_assumption_from (aalta_formula* f, bool global)
	{
		assumption_.clear ();
		af_prt_set ands = f->to_set ();
		for (af_prt_set::iterator it = ands.begin (); it != ands.end (); it ++)
		{
			if (global)
			{
				if ((*it)->is_global ())
					assumption_.push (SAT_lit (SAT_id (*it)));
			}
			else
				assumption_.push (SAT_lit (SAT_id (*it)));			
		}
		//don't forget tail!!
		if (global)
			assumption_.push (SAT_lit (tail_));
	}
	
 //for each pair (Xa, X!a), (XXa, XX!a).., generate equivalence Xa<-> !X!a, XXa <-> !XX!a
 void Solver::add_X_conflicts ()
 {
	 std::vector<std::pair<int, int> > pairs = get_conflict_literal_pairs ();
	 for (int i = 0; i < pairs.size (); i ++)
		 add_X_conflict_for_pair (pairs[i]);
 }
 
 //collect all id pairs like (a, !a) from formula_map_
 std::vector<std::pair<int, int> > 
	 Solver::get_conflict_literal_pairs ()
 {
	 std::vector<std::pair<int, int> > res;
	 for (formula_map::iterator it = formula_map_.begin (); it != formula_map_.end (); it ++)
	 {
		 if (it->first < 0)
			 continue;
		 formula_map::iterator it2 = formula_map_.find (-(it->first));
		 if (it2 != formula_map_.end ())
			 res.push_back (std::pair<int, int> (it->second->id (), it2->second->id ()));
	 }
	 return res;
 }
 
 //given \@ pa = (a, !a), add equivalence for Xa <-> !X!a, and recursively XXa <-> !XX!a ...
 void Solver::add_X_conflict_for_pair (std::pair<int, int>& pa)
 {
	 x_map::iterator it, it2;
	 it = X_map_.find (pa.first);
	 it2 = X_map_.find (pa.second);
	 if (it != X_map_.end () && it2 != X_map_.end ())
	 {
		 add_equivalence (it->second, -it2->second);
		 std::pair<int, int> pa = std::pair<int, int> (it->second, it2->second);
		 add_X_conflict_for_pair (pa);
	 }
 }
	
	void Solver::print_x_map ()
	{
		cout << "X_map_ :\n";
		for (x_map::iterator it = X_map_.begin (); it != X_map_.end (); it ++)
			cout << it->first << " -> " << it->second << endl;
		cout << "N_map_ :\n";
		for (x_map::iterator it = N_map_.begin (); it != N_map_.end (); it ++)
			cout << it->first << " -> " << it->second << endl;
	}
	
	void Solver::print_coi ()
	{
		cout << "COI :\n";
		for (coi_map::iterator it = coi_map_.begin (); it != coi_map_.end (); it ++)
		{
			cout << it->first << " -> ";
		 	print_vec (it->second);
			cout << endl;
		}	
	}
	
	void Solver::print_formula_map ()
	{
		cout << "formula_map_ :\n";
		for (formula_map::iterator it = formula_map_.begin (); it != formula_map_.end (); it ++)
			cout << it->first << " -> " << it->second->to_string () << endl;
	}
	
	
 	
 }
