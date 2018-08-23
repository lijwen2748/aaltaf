/* 
 * File:   utility.cpp
 * Author: Jianwen Li
 * Note: An interface for utility functions
 * Created on July 2, 2017
 */
 
 #ifndef UTILITY_H
 #define UTILITY_H
 
 #include "formula/aalta_formula.h"
 #include <vector>
 
 namespace aalta
 {
 	aalta_formula* formula_from (std::vector<aalta_formula*>& ands);
	void print_vec (const std::vector<int>&);
 }
 
 #endif
