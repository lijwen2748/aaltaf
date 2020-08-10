/* 
 * File:   evidence.h
 * Author: Jianwen Li
 * Note: Evidence interface for LTLf satisfiability checking
 * Created on August 30, 2017
 */
#include "evidence.h"
#include "formula/olg_formula.h"
#include "formula/aalta_formula.h"
#include "util/hash_map.h"
#include <iostream>
#include <vector>
#include <string>
#include <assert.h>
using namespace std;

namespace aalta
{
	void Evidence::push (bool tt)
	{
		assert (tt);
		traces_.push_back ("True");
	}
	
	void Evidence::push (olg_formula& olg)
	{
		hash_map<int, bool>& e = olg._evidence;
		vector<int> v;
		string s = "(";
		for (hash_map<int, bool>::iterator it = e.begin (); it != e.end (); it ++)
		{
			string tmp = aalta_formula::get_name (it->first);
			if (tmp == "Tail") continue;
			if (it->second)
				s += tmp + ", ";
			else
				s += "-" + tmp + ", ";
		}
		s += ")";
		traces_.push_back (s);
	}
	
	void Evidence::push (aalta_formula* f)
	{
		aalta_formula::af_prt_set p = f->to_set ();
		string s = "(";
		for (aalta_formula::af_prt_set::iterator it = p.begin (); it != p.end (); it ++)
		{
			string tmp = (*it)->to_string ();
			if (tmp.find ("Tail") != string::npos) continue;
			s += tmp + ", ";
		}
		s += ")";
		traces_.push_back (s);
	}
	
	void Evidence::pop_back ()
	{
		traces_.pop_back ();
	}
	
	void Evidence::print ()
	{
		//cout << "Find satisfying model: \n";
		for (int i = 0; i < traces_.size (); i ++)
		{
			cout << traces_[i] << endl;
		}
	}
	
}
