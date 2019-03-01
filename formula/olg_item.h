/* 
 * olg_formula的节点定义
 * File:   olg_item.h
 * Author: yaoyinbo
 *
 * Created on October 26, 2013, 1:42 PM
 */

#ifndef OLG_ITEM_H
#define	OLG_ITEM_H

#include "aalta_formula.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>


namespace aalta
{

struct olg_atom
{ // olg原子节点类型

  enum freqkind
  { //频率类型
    Once, More, Inf
  };

  int _id; // 变量id
  int _pos; //变量位置
  freqkind _freq; //频率
  bool _isDisjunct; // to record whether it is caused by disjunction when _pos is undertermine. 
  int _orig_pos; // to record the pos before off_pos by \/

  olg_atom (int id, int pos, freqkind freq, bool isDisjunct = false, int orig_pos = -1);

  std::string to_olg_string ()const;
  std::string to_string ()const;
};

struct olg_item
{ // olg非叶子节点类型
  int _id; //for Dimacs construction, added by Jianwen Li
  static int _varNum; //the var number in cnf
  static int _clNum;  // the clause number in cnf
  static std::vector<int> _vars; //the variables in _root
  static hash_map<int, int> _vMap; // the map between variable ids and encodings in dimacs. 
                                   //For example, if variable id is 10 and it encodings
                                   // in dimacs is 1, then _vMap[10]=1 holds.
  hash_map<int, bool> _evidence;  //show the evidence, the second element represents its evaluation.
  
  aalta_formula::opkind _op;      // operator
  olg_atom *_atom; //
  olg_item *_left;                //left child
  olg_item *_right;               //right child
  int _compos; /*added by Jianwen Li on Sept. 19th, 2013
               * record the common pos of all its sub atoms, if the pos exists. 
               * Otherwise compos = -1
                * PS：修改定义，-1表示两边子公式都不确定，0表示两边子公式compos不统一，其余compos从1开始
               */

  olg_item (aalta_formula::opkind op, int compos = 0, olg_item *left = NULL, olg_item *right = NULL, olg_atom *atom = NULL);
  olg_item (aalta_formula*, bool); //for propositional formulas only

  void off_pos (int);  //0 for \/; and 1 for U
  void plus_pos ();
  void unonce_freq ();

  bool is_more ()const;
  std::string get_olg_string ()const;
  std::string get_olg_more_string (int pos = INT_MAX)const;
  std::string get_id_string (int id)const;

  std::string to_olg_string ()const;
  std::string to_string ()const;
  
  /*added by Jianwen Li on April 29, 2014
   * creat the Dimacs format for Minisat from _root
   *
   */
  std::string toDimacs();
  void toDimacsPlus(FILE*); // Invoked by toDimacs();
  void setId(int&); //set the _id for Dimacs construction
  void getVars(int &); //set _vars
  void initial(); // reset the static variables
  bool SATCall(); //SAT solver invoking
  //for projection
  olg_item* proj(int, int); //the first parameter: 0 for position i; 1 for i and all; 
                            //2 for nondeter and all; 3 for Inf and all.
  hash_set<int> get_pos(int); // 0 for each position i; 1 for nondeter; 2 for Inf
  int get_max_loc(int, int); //the first parameter: 0 for nondeter; 1 for Inf
  olg_item* proj_loc(int, int, int&);//projection by distinguishing different copies
  
  bool unsat(); //checking unsatisfiability
  bool unsat2(); //another checking unsatisfiability
  olg_item* replace_inf_item(int, bool); //assign the atom of id to be false and get the formula
  hash_set<int> get_atoms(); //get the atom ids appearing in current formula
  void change_freq(); //change _freq for atoms which are not in an Or branch anymore

  static std::vector<olg_item*> _items; //store the created olg_item
  static std::vector<olg_atom*> _atoms; //store the created olg_atom
  static void destroy(); // delete elements in _items;
  
  
};

}

#endif	/* OLG_ITEM_H */

