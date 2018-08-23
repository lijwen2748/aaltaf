/* 
 * File:   utility.cpp
 * Author: Jianwen Li
 * Note: An interface for utility functions
 * Created on July 2, 2017
 */
 
 #include "utility.h"
 #include <iostream>

 using namespace std;
 
 namespace aalta{
 
 aalta_formula* formula_from (std::vector<aalta_formula*>& ands)
 {
 	if (ands.empty ())
 		return aalta_formula::TRUE ();
 	aalta_formula *res = NULL;
 	for (std::vector<aalta_formula*>::iterator it = ands.begin (); it != ands.end (); it ++)
 	{
 		if (res == NULL)
 			res = *it;
 		else
 			res = aalta_formula (aalta_formula::And, res, *it).unique ();
 	}
 	return res;
 }
 
 void print_vec (const std::vector<int>& v)
 {
	 cout << "(";
	 for (std::vector<int>::const_iterator it = v.begin (); it != v.end (); it ++)
		 cout << *it << ", ";
	 cout << ")";
 }
 
 }
 
