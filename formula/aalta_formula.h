/* 
 * ltl_formula变体
 * 表达式中不含<->、->、[]、<>，且！只会出现在原子前
 * 增加tag标记
 * File:   aalta_formula.h
 * Author: yaoyinbo and Jianwen Li
 * Jianwen Li adds the implementation of LTLf
 *
 * Created on October 21, 2013, 12:32 PM
 */

#ifndef AALTA_FORMULA_H
#define	AALTA_FORMULA_H

#include "util/define.h"
#include "util/hash_map.h"
#include "util/hash_set.h"
#include "ltlparser/ltl_formula.h"

#include <list>
#include <vector>
#include <string>
#include <set>

#define MAX_CL 500000

namespace aalta
{

class aalta_formula
{
private:
  typedef std::list<aalta_formula*> tag_t;
  ////////////
  //成员变量//
  //////////////////////////////////////////////////
  int _op; // 操作符
  aalta_formula *_left; // 操作符左端公式
  aalta_formula *_right; // 操作符右端公式
  tag_t *_tag; // 标签，相对于Until的位置信息
  size_t _hash; // hash值
  int _length; //公式长度
  aalta_formula *_unique; // 指向唯一指针标识
  aalta_formula *_simp; // 指向化简后的公式指针
  //////////////////////////////////////////////////

public:

  /* af公式的hash函数 */
  struct af_hash
  {

    size_t operator () (const aalta_formula& af) const
    {
      return af._hash;
    }
  };

  /* af指针的hash函数 */
  struct af_prt_hash
  {

    size_t operator () (const aalta_formula *af_prt) const
    {
      //return size_t (af_prt);
      return af_prt->_id;
    }
  };

  /* af指针的hash函数 */
  struct af_prt_hash2
  {

    size_t operator () (const aalta_formula *af_prt) const
    {
      return af_prt->_hash;
    }
  };
  /* af指针的相等函数 */
  struct af_prt_eq
  {

    bool operator () (const aalta_formula *af_prt1, const aalta_formula *af_prt2) const
    {
      return *af_prt1 == *af_prt2;
    }
  };
private:

  /* tag的hash函数 */
  struct tag_prt_hash
  {

    size_t operator () (const tag_t *tag)const
    {
      size_t hash = HASH_INIT;
      for (tag_t::const_iterator it = tag->begin (); it != tag->end (); it++)
        {
          hash = (hash << 5) ^ (hash >> 27) ^ (size_t) (*it);
        }
      return hash;
    }
  };

  struct tag_prt_eq
  {

    bool operator () (const tag_t *tag1, const tag_t *tag2)const
    {
      if (tag1->size () != tag2->size ()) return false;
      tag_t::const_iterator it1 = tag1->begin ();
      tag_t::const_iterator it2 = tag2->begin ();
      for (; it1 != tag1->end (); it1++, it2++)
        {
          if (*it1 != *it2)
            return false;
        }
      return true;
    }
  };
  
  struct compare 
  {
    bool operator () (const aalta_formula* f1, const aalta_formula* f2) const
    {
      return f1->_id < f2->_id;
    }
  };

  //typedef hash_map<aalta_formula, aalta_formula *, af_hash> af_prt_map;
public:
  typedef hash_set<aalta_formula *, af_prt_hash2, af_prt_eq> afp_set;
  typedef hash_set<aalta_formula *, af_prt_hash> af_prt_set;
  //typedef std::set<aalta_formula *, compare> af_prt_set;
private:
  typedef hash_set<int> int_set;
  typedef hash_set<tag_t *, tag_prt_hash, tag_prt_eq> tag_set;
  //////////////
  //静态成员变量//
  //////////////////////////////////////////////////
private:
  static std::vector<std::string> names; // 存储操作符的名称以及原子变量的名称
  static hash_map<std::string, int> ids; // 名称和对应的位置映射
  //static af_prt_map all_afs; // 所有aalta_formula实体和对应唯一指针的映射
  static afp_set all_afs;
  static tag_set all_tags; // 所有tag的集合
  static aalta_formula *_TRUE;
  static aalta_formula *_FALSE;
  //////////////////////////////////////////////////

public:

  /* 操作符类型 */
  enum opkind
  {
    True,
    False,
    Literal,
    Not,
    Or,
    And,
    Next,
    WNext, //weak Next, for LTLf
    Until,
    Release,
    Undefined
  };
private:

  /* 位置类型，判断公式组成结构时用 */
  enum poskind
  {
    Left, Right, All
  };

public:
  aalta_formula ();
  aalta_formula (const char *input, bool is_ltlf = false);
  aalta_formula (const aalta_formula& orig);
  aalta_formula (const ltl_formula *formula, bool is_not = false, bool is_ltlf = false);
  aalta_formula (int op, aalta_formula *left, aalta_formula *right, tag_t *tag = NULL);
  //for aiger 
  aalta_formula (unsigned);
  aalta_formula* nnf ();
  aalta_formula* nnf_not ();
  //for aiger
  virtual ~aalta_formula ();

  aalta_formula& operator = (const aalta_formula& af);
  bool operator == (const aalta_formula& af)const;
  bool operator< (const aalta_formula& af)const;

  int oper ()const;
  aalta_formula *l_af ()const;
  aalta_formula *r_af ()const;
  int get_length ();
  aalta_formula *af_now (int op);
  aalta_formula *af_next (int op)const;
  bool is_future ()const;
  bool is_globally ()const;
  bool is_until ()const;
  bool is_next ()const;
  bool release_free ()const;
  aalta_formula *clone ()const;
  std::string to_string ()const;
  std::string to_RPN ()const;

  aalta_formula *unique ();
  aalta_formula *simplify ();
  aalta_formula *classify (tag_t *tag = NULL);
  size_t hash () {return _hash;}
  
  static bool contain (af_prt_set, af_prt_set);
	
	inline int id () {return _id;}

private:

  //added for af_prt_set identification, _id is set in unique ()
  int _id;
  static int _max_id;
  //

  void init ();
  void clc_hash ();

  void build (const ltl_formula *formula, bool is_not = false, bool is_ltlf = false);
  void build_atom (const char *name, bool is_not = false);

  void split (int op, std::list<aalta_formula *>& af_list, bool to_simplify = false);
  bool mutex (int op, int_set& pos, int_set& neg);
  bool contain (poskind pos, int op, aalta_formula *af);
  bool contain (poskind pos1, int op1, poskind pos2, int op2, aalta_formula *af);

  static bool mutex (aalta_formula *af, int_set& pos, int_set& neg);
  static bool is_conflict (aalta_formula *af1, aalta_formula *af2);
  static aalta_formula *simplify_next (aalta_formula *af);
  static aalta_formula *simplify_or (aalta_formula *l, aalta_formula *r);
  static aalta_formula *simplify_until (aalta_formula *l, aalta_formula *r);
  static aalta_formula *simplify_release (aalta_formula *l, aalta_formula *r);

public:
  static aalta_formula *simplify_and (aalta_formula *l, aalta_formula *r);
  static aalta_formula *simplify_and_weak (aalta_formula *l, aalta_formula *r);
  static aalta_formula *merge_and (aalta_formula *af1, aalta_formula *af2);
  static std::string get_name (int index);
  static void destroy ();
  static aalta_formula *TRUE();
  static aalta_formula *FALSE();
  
  
  //added by Jianwen Li on July 16, 2017
	//add Tail for Next formulas
	aalta_formula* add_tail ();
  //This function split Next subformula by /\ or \/. i.e. X(a/\ b) -> X a /\ X b
  //It is a necessary preprocessing for SAT-based checking
  aalta_formula* split_next ();
  //replace weak next with next by N f <-> Tail | X f
  aalta_formula* remove_wnext ();
  static aalta_formula* TAIL ();
private:
  static aalta_formula *TAIL_;
  
  
  
/*
 * The followings are added by Jianwen Li on December 7th, 2013
 * For LTLf sat
 *
 */
public:
  typedef hash_map<int, aalta_formula *> int_prt_map;
  typedef hash_map<aalta_formula, int, af_hash> af_int_map;
  aalta_formula* off();        //obligation formula for LTLf formulas.
  aalta_formula* ofr();        //obligation formula for LTLf release formulas.
  aalta_formula* ofg();  //obligation formula for LTLf global formula
  aalta_formula* cf();        //current formula 
  void buildLTLf(const ltl_formula *formula, bool is_not = false);
  
  bool is_global();  //check whether the formula is a global one.
  bool is_wnext_free(); //check whether the formula is weak next free.
  
  bool model(af_prt_set);       // check whether an assignment P models current formula 
  bool model(aalta_formula*);   // handle when the assignment is a formula format 
  aalta_formula* progf(af_prt_set);      //formula progression
  
  
  
//private:
  /*only for boolean formulas*/
  
  void toDIMACS(int_prt_map&, int (&cls)[MAX_CL][3], int&);    
  void plus(int_prt_map&, af_int_map&, int&, int&, int, int (&cls)[MAX_CL][3], int&);
  void minus(int_prt_map&, af_int_map&, int&, int&, int, int (&cls)[MAX_CL][3], int&);
  
  std::vector<std::vector<int> > toDIMACS(int_prt_map&);    
  void plus(int_prt_map&, af_int_map&, int&, int&, int, std::vector<std::vector<int> >&);
  void minus(int_prt_map&, af_int_map&, int&, int&, int, std::vector<std::vector<int> >&);
  af_prt_set get_prop(std::string, int_prt_map);
  af_prt_set SAT();  
  aalta_formula* erase_next_global (aalta_formula::af_prt_set&);
  af_prt_set SAT_core();
          
  bool find (aalta_formula*);          
  aalta_formula* mark_until ();    
  bool model_until (af_prt_set);
  af_prt_set get_until_flags ();
  aalta_formula* normal ();
  bool is_until_marked ();
  aalta_formula* flatted ();
  static hash_map<aalta_formula*, aalta_formula *, af_prt_hash> _until_map;
  static hash_map<aalta_formula*, aalta_formula *, af_prt_hash> _var_until_map;
  static void print_all_formulas ();
  aalta_formula* get_var ();               //for Until formulas only
  aalta_formula* get_until ();             //for the variables representing until formulas only
  //aalta_formula* neg_prop(af_prt_set);   //create the formula for !{a, b, c, ...}
  /*only for boolean formulas*/
  
  static int _sat_count;                // counting SAT invoking 
  static void print_sat_count ();
  bool find_prop_atom (aalta_formula*);
  
   
  bool is_in(std::vector<aalta_formula*>);
  int search(aalta_formula, af_int_map&, int&);
  af_prt_set to_set(); //get the and elements in an And formula
  void to_set (af_prt_set&); //another version
	af_prt_set to_or_set (); //get the or elements in an And formula
  
  hash_set<aalta_formula*> and_to_set();
  af_prt_set get_alphabet();
  void complete (af_prt_set&);
  
  std::string ltlf2ltl(); //translate ltlf to ltl 
  std::string ltlf2ltlTranslate(); //core of translation from ltlf to ltl
  
//end of Jianwen Li


/*
 * The followings are added by Jianwen Li on October 8, 2014
 * For LTL co-safety to dfw
 *
 */
 
bool is_cosafety ();
std::string cosafety2smv ();
 
 
//end of Jianwen Li

};

}

#endif	/* AALTA_FORMULA_H */

