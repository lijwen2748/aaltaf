/* 
 * File:   dnf_formula.h
 * Author: yaoyinbo
 *
 * DNF(id) = DNF(left) U DNF(right)
 * 
 * 1. DNF(α) = {α ∧ X(True)} where α is a literal;
 * 2. DNF(Xφ) = {True∧X(φ)};
 * 3. DNF(φ1Uφ2) = DNF(φ2) ∪ DNF(φ1 ∧ X(φ1Uφ2));
 * 4. DNF(φ1Rφ2) = DNF(φ1 ∧ φ2) ∪ DNF(φ2 ∧ X(φ1Rφ2));
 * 5. DNF(φ1∨φ2) = DNF(φ1) ∪ DNF(φ2);
 * 6. DNF(φ1∧φ2) = {(α1∧α2)∧X(ψ1∧ψ2) | ∀i = 1,2.αi∧X(ψi) ∈ DNF(φi)};
 * 
 * Created on October 23, 2013, 9:23 PM
 */

#ifndef DNF_FORMULA_H
#define	DNF_FORMULA_H

#include "dnf_clause.h"

namespace aalta
{

class dnf_formula
{
public:
  typedef hash_set<dnf_clause, dnf_clause::dnf_clause_hash> dnf_clause_set;
private:
  typedef hash_set<aalta_formula *> next_set;
  typedef hash_map<aalta_formula *, dnf_formula *> af_dnf_map;
  typedef hash_map<aalta_formula *, dnf_clause_set *> dnf_map;

  ///////////
  //成员变量//
  //////////////////////////////////////////////////
private:
  aalta_formula *_id; // 将公式对应的唯一aalta_formula指针作为dnf的id标识
  dnf_formula *_left; // 并集左节点
  dnf_formula *_right; // 并集右节点
  dnf_formula *_unique; // 对应的dnf唯一标识

  //////////////
  //静态成员变量//
  //////////////////////////////////////////////////
private:
  static af_dnf_map all_dnfs; //记录所有dnf
  static dnf_map atomic_dnfs; //保存dnf叶子id和对应的dnf_clause集合
  static aalta_formula AF; //初始化aalta_formula, 无实质用处
  //////////////////////////////////////////////////


  //------------------------------------------------
  /* 成员方法 */
public:
  dnf_formula ();
  dnf_formula (const char *input);
  dnf_formula (aalta_formula& af);
  dnf_formula (aalta_formula *af);
  dnf_formula (const dnf_formula& orig);
  virtual ~dnf_formula ();

  dnf_formula& operator = (const dnf_formula& dnf);

  dnf_clause_set *get_next ()const;
  dnf_formula *unique ();
  dnf_formula *clone ()const;
  std::string to_string ()const;

  static void destroy ();
  static dnf_formula *get_dnf (aalta_formula *af);

private:
  void init ();
  void build ();
  dnf_clause_set *find_next ();
  void get_next (dnf_clause_set *dc_set)const;
  void cross (const dnf_formula *dnf1, const dnf_formula *dnf2, dnf_clause_set *s);
};

}

#endif	/* DNF_FORMULA_H */

