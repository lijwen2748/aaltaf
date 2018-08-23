/* 
 * File:   evidence.h
 * Author: Jianwen Li
 * Note: Evidence interface for LTLf satisfiability checking
 * Created on August 30, 2017
 */

#ifndef EVIDENCE_H
#define	EVIDENCE_H

#include "formula/aalta_formula.h"
#include "formula/olg_formula.h"
#include <vector>
#include <string>

namespace aalta
{
	class Evidence 
	{
	public:
		//functions
		Evidence () {};
		void print ();
		void push (bool);
		void push (olg_formula&);
		void push (aalta_formula *);
		void pop_back ();
		
	private:
		//member
		std::vector<std::string> traces_;
		
	};
}

#endif
