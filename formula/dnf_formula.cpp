/* 
 * File:   dnf_formula.cpp
 * Author: yaoyinbo
 * 
 * Created on October 23, 2013, 9:23 PM
 */

#include "dnf_formula.h"
#include "../util/utility.h"

#include <stdio.h>

namespace aalta
{

///////////////////////////////////////////
// 开始静态部分
/* 初始化静态变量 */
dnf_formula::af_dnf_map dnf_formula::all_dnfs;
dnf_formula::dnf_map dnf_formula::atomic_dnfs;
aalta_formula dnf_formula::AF;

/**
 * 销毁静态变量, 释放资源
 */
void
dnf_formula::destroy ()
{
  aalta_formula::destroy ();
  for (af_dnf_map::iterator it = all_dnfs.begin (); it != all_dnfs.end (); it++)
    delete it->second;
  all_dnfs.clear ();
  for (dnf_map::iterator it = atomic_dnfs.begin (); it != atomic_dnfs.end (); it++)
    delete it->second;
  atomic_dnfs.clear ();
}

/**
 * 通过af指针标识得到对应的dnf
 * @param af
 * @return 
 */
dnf_formula *
dnf_formula::get_dnf (aalta_formula *af)
{

  af_dnf_map::const_iterator it = all_dnfs.find (af);
  if (it != all_dnfs.end ())
    return it->second;
  return NULL;
}




// 结束静态部分
///////////////////////////////////////////

/**
 * 初始化成员变量
 */
void
dnf_formula::init ()
{
  _left = _right = _unique = NULL;
  _id = NULL;
}

dnf_formula::dnf_formula ()
{
  this->init ();
}

dnf_formula::dnf_formula (const dnf_formula& orig)
{
  *this = orig;
}

dnf_formula::dnf_formula (const char *input)
{
  this->init ();
  _id = aalta_formula (input).unique ();
  build ();
}

dnf_formula::dnf_formula (aalta_formula& af)
{
  this->init ();
  _id = af.unique ();
  build ();
}

dnf_formula::dnf_formula (aalta_formula *af)
{
  this->init ();
  _id = af->unique ();
  build ();
}

dnf_formula::~dnf_formula () { }

/**
 * 重载赋值操作符
 * @param dnf
 * @return 
 */
dnf_formula& dnf_formula::operator = (const dnf_formula& dnf)
{
  if (this != &dnf)
    {
      this->_id = dnf._id;
      this->_left = dnf._left;
      this->_right = dnf._right;
      this->_unique = dnf._unique;
    }
  return *this;
}

void
dnf_formula::build ()
{
  if (_id == NULL) return;
  af_dnf_map::const_iterator it = all_dnfs.find (_id);
  if (it != all_dnfs.end ())
    {
      *this = *(it->second);
      return;
    }
  switch (_id->oper ())
    {
    case aalta_formula::Next:
    case aalta_formula::WNext:
      { // DNF(X φ)={True ∧ X(φ)}; the same for weak next 
        dnf_clause_set *dc_set = this->find_next ();
        if (dc_set->empty ())
          dc_set->insert (dnf_clause (aalta_formula::TRUE(), _id->r_af ()));
        break;
      }
    case aalta_formula::Until:
      { // DNF(φ1 U φ2) = DNF(φ1 ∧ X(φ1 U φ2)) ∪ DNF(φ2)
        aalta_formula *afp = aalta_formula (aalta_formula::Next, NULL, _id).simplify ();
        //@ TODO: is it necessary???
        afp = aalta_formula::simplify_and_weak (_id->l_af (), afp);
        _left = dnf_formula (afp).unique ();
        _right = dnf_formula (_id->r_af ()).unique ();
        break;
      }
    case aalta_formula::Release:
      { // DNF(φ1 R φ2) = DNF(φ1 ∧ φ2) ∪ DNF(φ2 ∧ X(φ1 R φ2))
        //@ TODO: is it necessary???
        aalta_formula *l_afp = aalta_formula::simplify_and_weak (_id->l_af ()->unique (),
                                                                 _id->r_af ()->unique ());
        aalta_formula *r_afp = aalta_formula (aalta_formula::Next,
                                              NULL,
                                              _id->unique ()).simplify ();
        //@ TODO: is it necessary???
        r_afp = aalta_formula::simplify_and_weak (_id->r_af (), r_afp);
        _left = dnf_formula (l_afp).unique ();
        _right = dnf_formula (r_afp).unique ();
        break;
      }
    case aalta_formula::Or:
      { // DNF(φ1 ∨ φ2) = DNF(φ1) ∪ DNF(φ2)
        _left = dnf_formula (_id->l_af ()->unique ()).unique ();
        _right = dnf_formula (_id->r_af ()->unique ()).unique ();
        break;
      }
    case aalta_formula::And:
      { // DNF(φ1∧φ2) = {(α1∧α2)∧X(ψ1∧ψ2) | ∀i = 1,2.αi∧X(ψi) ∈ DNF(φi)}
        dnf_clause_set *dc_set = this->find_next ();
        if (dc_set->empty ())
          {
            dnf_formula *l_dnf = dnf_formula (_id->l_af ()).unique ();
            dnf_formula *r_dnf = dnf_formula (_id->r_af ()).unique ();
            cross (l_dnf, r_dnf, dc_set);
          }

        break;
      }
    case aalta_formula::Literal:
    case aalta_formula::Undefined:
      {
        print_error ("the formula cannot be transformed to dnf_formula");
        break;
      }
    default:
      { // DNF(α) = {α ∧ X(True)}
        dnf_clause_set *dc_set = this->find_next ();
        if (dc_set->empty () && _id->oper () != aalta_formula::False)
          dc_set->insert (dnf_clause (_id->unique (), aalta_formula::TRUE()));
        break;
      }
    }
}

/**
 * 通过aalta_formula指针标识寻找next集合 
 * 若之前未生成过该标识的next集合, 将new一个空集合加入到atomic_dnfs中去
 * @param id
 * @return 
 */
dnf_formula::dnf_clause_set *
dnf_formula::find_next ()
{
  dnf_map::const_iterator it = atomic_dnfs.find (_id);
  if (it == atomic_dnfs.end ())
    {
      dnf_clause_set *tmp = new dnf_clause_set ();
      atomic_dnfs[_id] = tmp;
      _left = _right = NULL;
      return tmp;
    }
  return it->second;
}

/**
 * DNF(φ1∧φ2) = {(α1∧α2)∧X(ψ1∧ψ2) | ∀i = 1,2.αi∧X(ψi) ∈ DNF(φi)}
 * @param dnf1
 * @param dnf2
 * @param s
 * //@ TODO: 考虑优化
 */
void
dnf_formula::cross (const dnf_formula *dnf1, const dnf_formula *dnf2, dnf_clause_set *s)
{
  if (dnf1 == NULL || dnf2 == NULL)
    {
      print_error ("formula in cross is NULL!");
      return;
    }
  if (dnf1->_left != NULL)
    {
      cross (dnf1->_left, dnf2, s);
      cross (dnf1->_right, dnf2, s);
    }
  else if (dnf2->_left != NULL)
    {
      cross (dnf1, dnf2->_left, s);
      cross (dnf1, dnf2->_right, s);
    }
  else
    {
      dnf_clause_set *s1 = atomic_dnfs[dnf1->_id];
      dnf_clause_set *s2 = atomic_dnfs[dnf2->_id];
      dnf_clause_set::const_iterator it1, it2;
      for (it1 = s1->begin (); it1 != s1->end (); it1++)
        for (it2 = s2->begin (); it2 != s2->end (); it2++)
          {
            aalta_formula *caf = aalta_formula::simplify_and_weak (it1->current, it2->current);
            if (caf->oper () == aalta_formula::False)
              continue;
            aalta_formula *naf = aalta_formula::simplify_and_weak (it1->next, it2->next);
            s->insert (dnf_clause (caf, naf));
          }
    }
}

/**
 * 获取next集合
 * @return 
 */
dnf_formula::dnf_clause_set *
dnf_formula::get_next () const
{
  dnf_map::iterator it = atomic_dnfs.find (_id);
  if (it == atomic_dnfs.end ())
    {
      atomic_dnfs[_id] = new dnf_clause_set ();
      it = atomic_dnfs.find (_id);
    }
  if (it->second->empty ())
    {
      if (_left != NULL)
        _left->get_next (it->second);
      if (_right != NULL)
        _right->get_next (it->second);
    }
  return it->second;
}

/**
 * 获取next集合
 * @param dc_set
 */
void
dnf_formula::get_next (dnf_clause_set *dc_set) const
{
  dnf_map::iterator it = atomic_dnfs.find (_id);
  if (it == atomic_dnfs.end ())
    {
      _left->get_next (dc_set);
      _right->get_next (dc_set);
    }
  else
    {
      for (dnf_clause_set::const_iterator iter = it->second->begin (); iter != it->second->end (); iter++)
        dc_set->insert (*iter);
    }
}

/**
 * 返回该dnf_formula对应的唯一指针标识
 * @return 
 */
dnf_formula *
dnf_formula::unique ()
{
  if (_unique == NULL)
    {
      af_dnf_map::const_iterator it = all_dnfs.find (_id);
      if (it != all_dnfs.end ())
        _unique = it->second;
      else
        {
          _unique = this->clone ();
          all_dnfs[_id] = _unique;
        }
    }
  return _unique;
}

/**
 * 克隆出该对象的副本（需要在外部显式delete）
 * @return 
 */
dnf_formula *
dnf_formula::clone () const
{
  return new dnf_formula (*this);
}

std::string
dnf_formula::to_string () const
{
  dnf_clause_set *dc_set = this->get_next ();

  std::string ret;
  ret += _id->to_string () + " [" + convert_to_string (dc_set->size ()) + "]";


  if (dc_set->size () > 0)
    {
      ret += "\n{";
      for (dnf_clause_set::const_iterator it = dc_set->begin (); it != dc_set->end (); it++)
        ret += "\n\t" + it->to_string ();
      ret += "\n}";
    }

  ret += "\n-----";

  return ret;
}

}
