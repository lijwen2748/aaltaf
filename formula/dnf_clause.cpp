/* 
 * File:   dnf_clause.cpp
 * Author: yaoyinbo
 * 
 * Created on October 23, 2013, 9:06 PM
 */

#include "dnf_clause.h"

namespace aalta
{

dnf_clause::dnf_clause (aalta_formula* current, aalta_formula* next)
{
  this->current = current;
  this->next = next;
  clc_hash ();
}

dnf_clause::dnf_clause (const dnf_clause& orig)
{
  *this = orig;
}

dnf_clause::~dnf_clause () { }

/**
 * 重载赋值操作符
 * @param dc
 * @return 
 */
dnf_clause& dnf_clause::operator = (const dnf_clause& dc)
{
  if (this != &dc)
    {
      this->current = dc.current;
      this->next = dc.next;
      this->_hash = dc._hash;
    }
  return *this;
}

/**
 * 重载等于符号
 * @param dc
 * @return 
 */
bool dnf_clause::operator == (const dnf_clause& dc) const
{
  return current == dc.current && next == dc.next;
}

/**
 * 重载小于号，stl_map中用
 * @param dc
 * @return 
 */
bool dnf_clause::operator< (const dnf_clause& dc) const
{
  return next->get_length () < dc.next->get_length ();
  if (next->get_length () != dc.next->get_length ())
    return next->get_length () < dc.next->get_length ();
  if (current == dc.current)
    return next < dc.next;
  return current < dc.current;
}

/**
 * To String
 * @return 
 */
std::string
dnf_clause::to_string () const
{
  return "[ " + current->to_string () + " -> " + next->to_string () + " ]";
}

/**
 * 计算hash值
 */
void
dnf_clause::clc_hash ()
{
  _hash = HASH_INIT;
  _hash = (_hash << 5) ^ (_hash >> 27) ^ (size_t) current;
  _hash = (_hash << 5) ^ (_hash >> 27) ^ (size_t) next;
}

}
