/* 
 * File:   dnf_clause.h
 * Author: yaoyinbo
 *
 * Created on October 23, 2013, 9:06 PM
 */

#ifndef DNF_CLAUSE_H
#define	DNF_CLAUSE_H

#include "aalta_formula.h"

namespace aalta
{

class dnf_clause
{
public:

  /* dnf_clause的hash函数 */
  struct dnf_clause_hash
  {

    size_t operator () (const dnf_clause& clause) const
    {
      return clause._hash;
    }
  };

  ///////////
  //成员变量//
  //////////////////////////////////////////////////
private:
  size_t _hash; // hash值
public:
  aalta_formula *current; // 当前aalta_formula
  aalta_formula *next; //下一个节点aalta_formula
  //////////////////////////////////////////////////

  //------------------------------------------------
  /* 成员方法 */
public:

  dnf_clause (aalta_formula *current = NULL, aalta_formula *next = NULL);
  dnf_clause (const dnf_clause& orig);
  virtual ~dnf_clause ();

  dnf_clause& operator = (const dnf_clause& dc);
  bool operator == (const dnf_clause& dc)const;
  bool operator< (const dnf_clause& dc)const;

  std::string to_string ()const;

private:
  void clc_hash ();
};

}

#endif	/* DNF_CLAUSE_H */
