/* 
 * File:   aaltasolver.cpp
 * Author: Jianwen Li
 * Note: An inheritance class from Minisat::Solver for Aalta use 
 * Created on August 15, 2017
 */
 
#include "aaltasolver.h"
#include <iostream>
#include <vector>
using namespace std;
using namespace Minisat;

namespace aalta
{
 	
 	Lit AaltaSolver::SAT_lit (int id)
 	{
 		assert (id != 0);
        int var = abs(id)-1;
        while (var >= nVars()) newVar();
        return ( (id > 0) ? mkLit(var) : ~mkLit(var) );
 	}
 	
 	int AaltaSolver::lit_id (Lit l)
    {
    	if (sign(l)) 
            return -(var(l) + 1);
        else 
            return var(l) + 1;
    }
 	
 	bool AaltaSolver::solve_assumption ()
	{
		lbool ret = solveLimited (assumption_);
		if (verbose_)
		{
			cout << "solve_with_assumption: assumption_ is" << endl;
			for (int i = 0; i < assumption_.size (); i ++)
				cout << lit_id (assumption_[i]) << ", ";
			cout << endl;
		}
		if (ret == l_True)
     		return true;
   		else if (ret == l_Undef)
     		exit (0);
   		return false;
	}
	
	//return the model from SAT solver when it provides SAT
	std::vector<int> AaltaSolver::get_model ()
	{
		std::vector<int> res;
		res.resize (nVars (), 0);
   		for (int i = 0; i < nVars (); i ++)
   		{
     		if (model[i] == l_True)
       			res[i] = i+1;
     		else if (model[i] == l_False)
       			res[i] = -(i+1);
   		}
   		if (verbose_)
   		{
   			cout << "original model from SAT solver is" << endl;
   			for (int i = 0; i < res.size (); i ++)
   				cout << res[i] << ", ";
   			cout << endl;
   		}
   		return res;
	}
	
	//return the UC from SAT solver when it provides UNSAT
 	std::vector<int> AaltaSolver::get_uc ()
 	{
 		std::vector<int> reason;
		if (verbose_)
			cout << "get uc: \n";
 		for (int k = 0; k < conflict.size(); k++) 
 		{
        	Lit l = conflict[k];
        	reason.push_back (-lit_id (l));
			if (verbose_)
				cout << -lit_id (l) << ", ";
    	}
		if (verbose_)
			cout << endl;
    	return reason;
  	}
	
	void AaltaSolver::add_clause (std::vector<int>& v)
 	{
 		vec<Lit> lits;
 		for (std::vector<int>::iterator it = v.begin (); it != v.end (); it ++)
 			lits.push (SAT_lit (*it));
 		/*
 		if (verbose_)
 		{
 			cout << "Adding clause " << endl << "(";
 			for (int i = 0; i < lits.size (); i ++)
 				cout << lit_id (lits[i]) << ", ";
 			cout << ")" << endl;
 			cout << "Before adding, size of clauses is " << clauses.size () << endl;
 		}
 		*/
 		addClause (lits);
 		/*
 		if (verbose_)
 			cout << "After adding, size of clauses is " << clauses.size () << endl;
 		*/
 	}
 	
 	void AaltaSolver::add_clause (int id)
 	{
 		std::vector<int> v;
 		v.push_back (id);
 		add_clause (v);
 	}
 	
 	void AaltaSolver::add_clause (int id1, int id2)
 	{
 		std::vector<int> v;
 		v.push_back (id1);
 		v.push_back (id2);
 		add_clause (v);
 	}
 	
 	void AaltaSolver::add_clause (int id1, int id2, int id3)
 	{
 		std::vector<int> v;
 		v.push_back (id1);
 		v.push_back (id2);
 		v.push_back (id3);
 		add_clause (v);
 	}
 	
 	void AaltaSolver::add_clause (int id1, int id2, int id3, int id4)
 	{
 		std::vector<int> v;
 		v.push_back (id1);
 		v.push_back (id2);
 		v.push_back (id3);
 		v.push_back (id4);
 		add_clause (v);
 	}
 	
 	void AaltaSolver::print_clauses ()
	{
		cout << "clauses in SAT solver: \n";
		for (int i = 0; i < clauses.size (); i ++)
		{
			Clause& c = ca[clauses[i]];
			for (int j = 0; j < c.size (); j ++)
				cout << lit_id (c[j]) << ", ";
			cout << endl;
		}
	}
}
