/* 
 * File:   aalta_formula.cpp
 * Author: yaoyinbo
 * 
 * Created on October 21, 2013, 12:32 PM
 */

#include "aalta_formula.h"
#include "util/utility.h"
#include "ltlparser/trans.h"

#include <string.h>
#include <algorithm>
#include <vector>
#include <iostream>
#include <fstream>
#include <zlib.h>
#include "minisat/core/Dimacs.h"
#include "minisat/core/Solver.h"
#include "minisat/core/SolverTypes.h"
using namespace std;
using namespace Minisat;

namespace aalta
{

///////////////////////////////////////////
// 开始静态部分
/* 初始化静态变量 */
std::vector<std::string> aalta_formula::names;
hash_map<std::string, int> aalta_formula::ids;
//aalta_formula::af_prt_map aalta_formula::all_afs;
aalta_formula::afp_set aalta_formula::all_afs;
aalta_formula::tag_set aalta_formula::all_tags;
aalta_formula *aalta_formula::_TRUE = NULL;
aalta_formula *aalta_formula::_FALSE = NULL;



aalta_formula *
aalta_formula::TRUE ()
{
  if (_TRUE == NULL)
    {
      _TRUE = aalta_formula (True, NULL, NULL).unique ();
      _TRUE->_unique = _TRUE;
    }
  return _TRUE;
}

aalta_formula *
aalta_formula::FALSE ()
{
  if (_FALSE == NULL)
    {
      _FALSE = aalta_formula (False, NULL, NULL).unique ();
      _FALSE->_unique = _FALSE;
    }
  return _FALSE;
}

/**
 * 获取符号对应名称或变量id对应的变量名
 * @param index
 * @return 
 */
std::string
aalta_formula::get_name (int index)
{
  return names[index];
}

/**
 * 销毁静态变量, 释放资源
 */
void
aalta_formula::destroy ()
{
  std::list<tag_t *> l;
  for (tag_set::iterator it = all_tags.begin (); it != all_tags.end (); it++)
    l.push_back (*it);
  for (std::list<tag_t *>::iterator it = l.begin (); it != l.end (); it++)
    delete *it;
  all_tags.clear ();
  //  for (af_prt_map::iterator it = all_afs.begin (); it != all_afs.end (); it++)
  //    delete it->second;

  for (afp_set::iterator it = all_afs.begin (); it != all_afs.end (); it ++)
  {
    //if ((*it) == NULL)
      //printf ("i=%d, address=%d\n", i, (int)(*it));
    delete (*it);

  }
  all_afs.clear ();
  ids.clear ();
  names.clear ();
  _TRUE = NULL;
  _FALSE = NULL;
}

void 
aalta_formula::print_all_formulas ()
{

  afp_set::iterator it;
  for (it = all_afs.begin (); it != all_afs.end (); it ++)
    printf ("%s : address=%p\n", (*it)->to_string ().c_str (), (*it));
    
}

/**
 * 判断af是否与集合中的元素互斥, 即是否存在 a 和 !a
 * @param af
 * @param pos
 * @param neg
 * @return 
 */
bool
aalta_formula::mutex (aalta_formula *af, int_set& pos, int_set& neg)
{
  if (af == NULL) return false;
  switch (af->_op)
    {
    case False:
      return true;
    case Not:
      if (af->_right->_op <= Undefined) return false;
      if (pos.find (af->_right->_op) != pos.end ())
        return true;
      neg.insert (af->_right->_op);
      break;
    default:
      if (af->_op <= Undefined) return false;
      if (neg.find (af->_op) != neg.end ())
        return true;
      pos.insert (af->_op);
      break;
    }
  return false;
}

/**
 * 判两个公式是否冲突
 * @param af1
 * @param af2
 * @return 
 */
bool
aalta_formula::is_conflict (aalta_formula *af1, aalta_formula *af2)
{
  //@ TODO: 好想重构化简代码啊啊啊啊啊，可是好难重构啊啊啊啊啊啊   %>_<%
  std::list<aalta_formula *> *af_list = NULL;
  switch (af1->_op)
    {
    case False:
      return true;
    case Next:
      switch (af2->_op)
        {
        case False:
          return true;
        case Next:
          return is_conflict (af1->_right, af2->_right);
        case Until:
        case Or:
          return is_conflict (af1, af2->_left) && is_conflict (af1, af2->_right);
        case Release:
          return is_conflict (af1, af2->_right);
        case And:
          return is_conflict (af1, af2->_left) || is_conflict (af1, af2->_right);
        default:
          return false;
        }
    case Until:
      switch (af2->_op)
        {
        case False:
          return true;
        case Until:
        case Or:
          return is_conflict (af1, af2->_left) && is_conflict (af1, af2->_right);
        case Release:
          return is_conflict (af1, af2->_right);
        case And:
          af_list = new std::list<aalta_formula *>();
          af_list->push_back (af1);
          af2->split (And, *af_list, false);
          break;
        case Next:
        default:
          return is_conflict (af2, af1);
        }
      break;
    case Release:
      switch (af2->_op)
        {
        case False:
          return true;
        case Or:
          return is_conflict (af1->_right, af2->_left) && is_conflict (af1->_right, af2->_right);
        case Release:
          return is_conflict (af1->_right, af2->_right);
        case And:
          return is_conflict (af1->_right, af2->_left) || is_conflict (af1->_right, af2->_right);
        case Until:
        case Next:
        default:
          return is_conflict (af2, af1->_right);
        }
    case Or:
      switch (af2->_op)
        {
        case False:
          return true;
        case Or:
          return is_conflict (af1, af2->_left) && is_conflict (af1, af2->_right);
        case And:
          return is_conflict (af1, af2->_left) || is_conflict (af1, af2->_right);
        case Release:
          return is_conflict (af1, af2->_right);
        case Until:
        case Next:
        default:
          return is_conflict (af2, af1);
        }
    case And:
      switch (af2->_op)
        {
        case False:
          return true;
        case And:
          af_list = new std::list<aalta_formula *>();
          af1->split (And, *af_list, false);
          af2->split (And, *af_list, false);
          break;
        case Release:
          return is_conflict (af1, af2->_right);
        case Or:
        case Until:
        case Next:
        default:
          return is_conflict (af2, af1);
        }
      break;
    default:
      switch (af2->_op)
        {
        case False:
          return true;
        case Next:
          return false;
        case Until:
        case Or:
          return is_conflict (af1, af2->_left) && is_conflict (af1, af2->_right);
        case Release:
          return is_conflict (af1, af2->_right);
        case And:
          af_list = new std::list<aalta_formula *>();
          af_list->push_back (af1);
          af2->split (And, *af_list, false);
          break;
        default:
          //          return (af1->_op == Not && af1->_right->unique () == af2->unique ())
          //                  || (af2->_op == Not && af2->_right->unique () == af1->unique ());
          return (af1->_op == Not && af1->_right->_op == af2->_op)
                  || (af2->_op == Not && af2->_right->_op == af1->_op);
        }
      break;
    }
  if (af_list != NULL)
    {
      std::list<aalta_formula *>::iterator it1, it2;
      for (it1 = af_list->begin (); it1 != af_list->end ();)
        {
          aalta_formula *af = *it1;
          for (it2 = ++it1; it2 != af_list->end (); it2++)
            if (is_conflict (af, *it2))
              {
                delete af_list;
                return true;
              }
        }
      delete af_list;
    }
  return false;
}

/**
 * 对 (X a) 做化简
 * @param af
 * @return 
 */
aalta_formula *
aalta_formula::simplify_next (aalta_formula *af)
{
  aalta_formula *s = af->simplify ();
  aalta_formula *simp;
  switch (s->_op)
    {
    case True: // X True = True
      simp = aalta_formula::TRUE ();
      break;
    case False: // X False = False
      simp = aalta_formula::FALSE ();
      break;
    default:
      simp = aalta_formula (Next, NULL, s).unique ();
      break;
    }
  return simp;
}

/**
 * 对 (l | r) 做化简
 * @param l
 * @param r
 * @return 
 */
aalta_formula *
aalta_formula::simplify_or (aalta_formula *l, aalta_formula *r)
{
  std::list<aalta_formula *> af_list;
  l->split (Or, af_list, true);
  r->split (Or, af_list, true);

  // 增加一个1是为了sort用
  aalta_formula **afp = new aalta_formula*[af_list.size () + 1];
  if (afp == NULL)
    {
      print_error ("Memory error!");
      exit (1);
    }
  int n = 0;
  int_set pos, neg;
  for (std::list<aalta_formula *>::iterator it = af_list.begin (); it != af_list.end (); it++)
    {
      switch ((*it)->_op)
        {
        case False: // False | a... = a...
          break;
        case True: // True | a... = True
          delete[] afp;
          return aalta_formula::TRUE ();
        case Until:
          // a | b U (!a | .. ) = True
          // b U (a | .. ) | c U (!a | ..) = True;
          if ((*it)->_right->mutex (Or, pos, neg))
            {
              delete[] afp;
              return aalta_formula::TRUE ();
            }
        default:
          // a | !a = True
          if (aalta_formula::mutex (*it, pos, neg))
            {
              delete[] afp;
              return aalta_formula::TRUE ();
            }
          afp[n++] = *it;
          break;
        }
    }
  aalta_formula *ret;
  if (n == 0)
    ret = aalta_formula::FALSE ();
  else
    {
      std::sort (afp, afp + n);
      int i, j;
      //a | b R a = a;
      //a | b U a = b U a;
      af_prt_set s1, s2;
      s1.insert (afp[0]);
      if (afp[0]->_op == Until) s2.insert (afp[0]->_right);
      for (i = 0, j = 1; j < n; ++j)
        if (afp[i] != afp[j])
          { // 去重
            afp[++i] = afp[j];
            s1.insert (afp[j]);
            if (afp[j]->_op == Until)
              s2.insert (afp[j]->_right);
          }
      for (n = i, i = -1, j = 0; j <= n; ++j)
        if ((afp[j]->_op != Release || s1.find (afp[j]->_right) == s1.end ())
            && s2.find (afp[j]) == s2.end ())
          afp[++i] = afp[j];
      n = i + 1;
      if (n == 1)
        ret = afp[0];
      else
        {
          l = afp[0];
          r = afp[--n];
          while (--n)
            {
              r = aalta_formula (Or, afp[n], r).unique ();
              r->_simp = r;
            }
          ret = aalta_formula (Or, l, r).unique ();
          ret->_simp = ret;
        }
    }

  delete[] afp;
  return ret;
}

/**
 * 对 (l & r) 做化简，其中l和r未化简
 * @param l
 * @param r
 * @return 
 */
aalta_formula *
aalta_formula::simplify_and (aalta_formula *l, aalta_formula *r)
{
  if (l->_simp != NULL && r->_simp != NULL)
    return simplify_and_weak (l, r);
  std::list<aalta_formula *> af_list;
  l->split (And, af_list, true);
  r->split (And, af_list, true);

  // 增加一个1是为了sort用
  aalta_formula **afp = new aalta_formula*[af_list.size () + 1];
  if (afp == NULL)
    {
      print_error ("Memory error!");
      exit (1);
    }
  int n = 0;
  for (std::list<aalta_formula *>::iterator it = af_list.begin (); it != af_list.end (); it++)
    {
      switch ((*it)->_op)
        {
        case True:
          break;
        case False:
          delete[] afp;
          return aalta_formula::FALSE ();
        default:
          afp[n++] = *it;
          break;
        }
    }
  aalta_formula *ret = NULL;
  if (n == 0)
    ret = aalta_formula::TRUE ();
  else
    {
      std::sort (afp, afp + n);
      int i, j;
      for (i = 0, j = 1; j < n; ++j)
        if (afp[i] != afp[j]) // 去重
          afp[++i] = afp[j];
      n = i + 1;
      for (i = 0; i < n; ++i)
        for (j = i + 1; j < n; ++j)
          if (aalta_formula::is_conflict (afp[i], afp[j]))
            {
              delete[] afp;
              return aalta_formula::FALSE ();
            }

      if (n == 1)
        ret = afp[0];
      else
        {
          l = afp[0];
          r = afp[--n];
          while (--n)
            {
              r = aalta_formula (And, afp[n], r).unique ();
              r->_simp = r;
            }
          ret = aalta_formula (And, l, r).unique ();
          ret->_simp = ret;
        }
    }
  delete[] afp;
  return ret->unique ();
}

/**
 * 对 (l & r) 做化简，其中l和r已化简
 * @param l
 * @param r
 * @return 
 */
aalta_formula *
aalta_formula::simplify_and_weak (aalta_formula *l, aalta_formula *r)
{
  //@ TODO: 2、能否去除list的使用？
  aalta_formula *l_next, *r_next;
  aalta_formula *l_now, *r_now;
  for (l_next = l->simplify (); l_next != NULL; l_next = l_next->af_next (And))
    {
      l_now = l_next->af_now (And);
      for (r_next = r->simplify (); r_next != NULL; r_next = r_next->af_next (And))
        if (aalta_formula::is_conflict (l_now, r_next->af_now (And)))
          return aalta_formula::FALSE ();
    }

  l_next = l->simplify ();
  r_next = r->simplify ();
  l_now = l_next->af_now (And);
  r_now = r_next->af_now (And);
  std::list<aalta_formula *> af_list;
  while (l_next != NULL && r_next != NULL)
    {
      if (l_now < r_now)
        {
          if (l_now->_op != True)
            af_list.push_back (l_now);
          l_next = l_next->af_next (And);
          l_now = l_next == NULL ? NULL : l_next->af_now (And);
        }
      else if (l_now > r_now)
        {
          if (r_now->_op != True)
            af_list.push_back (r_now);
          r_next = r_next->af_next (And);
          r_now = r_next == NULL ? NULL : r_next->af_now (And);
        }
      else
        {
          if (l_now->_op != True)
            af_list.push_back (l_now);
          l_next = l_next->af_next (And);
          l_now = l_next == NULL ? NULL : l_next->af_now (And);
          r_next = r_next->af_next (And);
          r_now = r_next == NULL ? NULL : r_next->af_now (And);
        }
    }

  if (l_next != NULL && l_next->_op != True)
    af_list.push_back (l_next);
  if (r_next != NULL && r_next->_op != True)
    af_list.push_back (r_next);

  aalta_formula *ret;
  switch (af_list.size ())
    {
    case 0:
      return aalta_formula::TRUE ();
    case 1:
      return af_list.front ()->unique ();
    default:
      l = af_list.front ()->unique ();
      af_list.pop_front ();
      r = af_list.back ()->unique ();
      af_list.pop_back ();
      for (; !af_list.empty (); af_list.pop_back ())
        {
          r = aalta_formula (And, af_list.back (), r).unique ();
          r->_simp = r;
        }
      ret = aalta_formula (And, l, r).unique ();
      ret->_simp = ret;
      return ret;
    }
  return NULL;
}

/**
 * 合并两个formula而不做化简
 * @param af1
 * @param af2
 * @return 
 */
aalta_formula *
aalta_formula::merge_and (aalta_formula* af1, aalta_formula* af2)
{
  aalta_formula *next1 = af1, *next2 = af2;
  aalta_formula *now1 = next1 == NULL ? NULL : next1->af_now (And);
  aalta_formula *now2 = next2 == NULL ? NULL : next2->af_now (And);
  std::list<aalta_formula *> af_list;
  while (next1 != NULL && next2 != NULL)
    {
      if (now1 < now2)
        {
          if (now1->_op != True)
            af_list.push_back (now1);
          next1 = next1->af_next (And);
          now1 = next1 == NULL ? NULL : next1->af_now (And);
        }
      else if (now1 > now2)
        {
          if (now2->_op != True)
            af_list.push_back (now2);
          next2 = next2->af_next (And);
          now2 = next2 == NULL ? NULL : next2->af_now (And);
        }
      else
        {
          if (now1->_op != True)
            af_list.push_back (now1);
          next1 = next1->af_next (And);
          now1 = next1 == NULL ? NULL : next1->af_now (And);
          next2 = next2->af_next (And);
          now2 = next2 == NULL ? NULL : next2->af_now (And);
        }
    }

  if (next1 != NULL && next1->_op != True)
    af_list.push_back (next1);
  if (next2 != NULL && next2->_op != True)
    af_list.push_back (next2);

  switch (af_list.size ())
    {
    case 0:
      return aalta_formula::TRUE ();
    case 1:
      return af_list.front ()->unique ();
    default:
      now1 = af_list.front ()->unique ();
      af_list.pop_front ();
      now2 = af_list.back ()->unique ();
      af_list.pop_back ();
      for (; !af_list.empty (); af_list.pop_back ())
        now2 = aalta_formula (And, af_list.back (), now2).unique ();
      return aalta_formula (And, now1, now2).unique ();
    }
}

/**
 * 对 (l U r) 做化简
 * @param l
 * @param r
 * @return 
 */
aalta_formula *
aalta_formula::simplify_until (aalta_formula *l, aalta_formula *r)
{
  aalta_formula *l_s = l->simplify ();
  aalta_formula *r_s = r->simplify ();
  aalta_formula *simp;
  if (l_s->_op == False // False U a = a
      || r_s->_op == False // a U False = False
      || r_s->_op == True // a U True = True
      || r_s->contain (All, Or, l_s) // a U (a | ...) = a | ...
      || r_s->contain (Left, Until, l_s) // a U (a U b) = a U b
      || r_s->contain (Right, Until, l_s) // a U (b U a) = b U a
      || r_s->contain (Right, Release, l_s) // a U (b R a) = b R a
      || l_s->contain (Right, Release, r_s) // (b R a) U a = a;
      )
    simp = r_s;
  else if (l_s->contain (Left, Until, r_s)) // (a U b) U a = b U a
    simp = aalta_formula (Until, l_s->_right, l_s->_left).simplify ();
  else if (l_s->contain (Right, Until, r_s)) // (b U a) U a = b U a
    simp = l_s;
  else if (l_s->contain (Right, Next, r_s)) // X a U a = X a | a
    simp = aalta_formula (Or, l_s, r_s).simplify ();
  else if (l_s->_op == Next && r_s->_op == Next) // X a U X b = X ( a U b )
    simp = aalta_formula (Next, NULL, aalta_formula (Until, l_s->_right, r_s->_right).simplify ()).simplify ();
  else
    simp = aalta_formula (Until, l_s, r_s).unique ();
  return simp;
}

/**
 * 对 (l R r) 做化简
 * @param l
 * @param r
 * @return 
 */
aalta_formula *
aalta_formula::simplify_release (aalta_formula *l, aalta_formula *r)
{
  aalta_formula *l_s = l->simplify ();
  aalta_formula *r_s = r->simplify ();
  aalta_formula *simp;
  if (l_s->_op == True // True R a = a
      || r_s->_op == False // a R False = False
      || r_s->_op == True // a R True = True
      || r_s->contain (All, And, l_s) // a R (a & ...) = a & ...
      || l_s->contain (All, Or, r_s) // (a | ...) R a = a
      || r_s->contain (Left, Release, l_s) // a R (a R b) = a R b
      || r_s->contain (Right, Release, l_s) // a R (b R a) = b R a
      || r_s->contain (Right, Until, l_s) // a R (b U a) = b U a
      || l_s->contain (All, Or, Right, Until, r_s) // (b U a | ... ) R a = a;
      )
    simp = r_s;
  else if (l_s->contain (Left, Release, r_s)) // (a R b) R a = b R a
    simp = aalta_formula (Release, l_s->_right, l_s->_left).simplify ();
  else if (l_s->contain (Right, Release, r_s)) // (b R a) R a = b R a
    simp = l_s;
  else if (l_s->_op == Next && r_s->_op == Next) // X a R X b = X ( a R b )
    simp = aalta_formula (Next, NULL, aalta_formula (Release, l_s->_right, r_s->_right).simplify ()).simplify ();
  else if ((l_s->_op == Not && l_s->_right == r_s)
           || (r_s->_op == Not && r_s->_right == l_s)) // !a R a = False R a
    //@ TODO: 这里只处理了原子，是否扩展为普通公式？ AND 似乎最后没必要再simplify
    simp = aalta_formula (Release, aalta_formula::FALSE (), r_s).simplify ();
  else
    simp = aalta_formula (Release, l_s, r_s).unique ();
  //@ TODO: (b R (!a & ..) & ..) R a = False R a

  return simp;
}
// 结束静态部分
///////////////////////////////////////////

/**
 * 初始化成员变量
 */
inline void
aalta_formula::init ()
{
  _left = _right = _unique = _simp = NULL;
  _tag = NULL;
  _op = Undefined;
  _length = 0;
  if (names.empty ())
    { //按顺序
      names.push_back ("true");
      names.push_back ("false");
      names.push_back ("Literal");
      names.push_back ("!");
      names.push_back ("|");
      names.push_back ("&");
      names.push_back ("X");
      names.push_back ("N"); //weak Next, for LTLf
      names.push_back ("U");
      names.push_back ("R");
      names.push_back ("Undefined");
    }
}

/**
 * 计算hash值
 */
inline void
aalta_formula::clc_hash ()
{
  _hash = HASH_INIT;
  _hash = (_hash << 5) ^ (_hash >> 27) ^ _op;
  //_hash = (_hash << 5) ^ (_hash >> 27) ^ (size_t) _left;
  //_hash = (_hash << 5) ^ (_hash >> 27) ^ (size_t) _right;
  
  if (_left != NULL)
    _hash = (_hash << 5) ^ (_hash >> 27) ^ _left->_hash;
  if (_right != NULL)
    _hash = (_hash << 5) ^ (_hash >> 27) ^ _right->_hash;
  
  _hash = (_hash << 5) ^ (_hash >> 27) ^ (size_t) _tag;

}

aalta_formula::aalta_formula ()
{
  init ();
}

aalta_formula::aalta_formula (const aalta_formula& orig)
{
  *this = orig;
}

aalta_formula::aalta_formula (int op, aalta_formula *left, aalta_formula *right, tag_t *tag)
: _op (op), _tag (tag), _length (0), _unique (NULL), _simp (NULL)
{
/*
  //modified by Jianwen Li for construction of of and cf fromulas
  if(op == And)
  {
    if(left->oper() == True)
    {
      *this = *(right->unique());
    }
    else
    {
      if(left->oper() == False)
      {
        *this = *(left->unique());
      }
      else
      {
        if(right->oper() == True)
        {
          *this = *(left->unique());
        }
        else
        {
          if(right->oper() == False)
          {
            *this = *(right->unique());
          }
          else
          {
            _left = left == NULL ? NULL : left->unique ();
            _right = right == NULL ? NULL : right->unique ();
            clc_hash ();
          }
        }
      }
    }
  }
  else
  if(op == Or)
  {
    if(left->oper() == True)
    {
      *this = *(left->unique());
    }
    else
    {
      if(left->oper() == False)
      {
        *this = *(right->unique());
      }
      else
      {
        if(right->oper() == True)
        {
          *this = *(right->unique());
        }
        else
        {
          if(right->oper() == False)
          {
            *this = *(left->unique());
          }
          else
          {
            _left = left == NULL ? NULL : left->unique ();
            _right = right == NULL ? NULL : right->unique ();
            clc_hash ();
          }
        }
      }
    }
  }
  else
  {
    _left = left == NULL ? NULL : left->unique ();
    _right = right == NULL ? NULL : right->unique ();
    clc_hash ();
  }
  */
  
  //_left = left == NULL ? NULL : left->unique ();
   // _right = right == NULL ? NULL : right->unique ();
   if (left == NULL)
     _left = NULL;
   else 
     _left = left->unique ();
   if (right == NULL)
     _right = NULL;
   else
     _right = right->unique ();
    clc_hash ();
  
}

aalta_formula::aalta_formula (const char *input, bool is_ltlf)
{
  init ();
  ltl_formula *formula = getAST (input);
  if(is_ltlf)
    build (formula, false, true);
  else
    build (formula);
  destroy_formula (formula);
  clc_hash ();
  /*
  if(!is_ltlf)
    *this = *simplify ();
  */
  //*this = *classify ();
}

aalta_formula::aalta_formula (const ltl_formula *formula, bool is_not, bool is_ltlf)
{
  init ();
  build (formula, is_not, is_ltlf);
  clc_hash ();
}

aalta_formula::aalta_formula (unsigned index)
{
  init ();
  std::string str = std::string ("var") + convert_to_string (index);
  
  int id;
  hash_map<std::string, int>::const_iterator it = ids.find (str);
  if (it == ids.end ())
    { // 此变量名未出现过，添加之
      id = names.size ();
      ids[str] = id;
      names.push_back (str);
    }
  else
    {
      id = it->second;
    }
  _op = id;
  
  _left = NULL;
  _right = NULL;
  clc_hash ();
}

aalta_formula::~aalta_formula () { }

aalta_formula * 
aalta_formula::nnf ()
{
  aalta_formula *result, *l, *r;
  if (oper () == Not)
  {
    if (_right->oper () == Not)
      result = _right->_right->nnf ();
    else
      result = _right->nnf_not ();
  }
  else
  {
    l = NULL;
    r = NULL;
    if (_left != NULL)
      l = _left->nnf ();
    if (_right != NULL)
      r = _right->nnf ();
    result = aalta_formula (oper (), l, r).unique ();
  }
  return result;
}

aalta_formula* 
aalta_formula::nnf_not ()
{
  aalta_formula *result, *l, *r;
  switch (oper ())
  {
    case True:
      result = FALSE ();
      break;
    case False:
      result = TRUE ();
      break;
    case Not:
      result = _right->nnf ();
      break;
    case Next:
      r = _right->nnf_not ();
      result = aalta_formula (WNext, NULL, r).unique ();
      break;
    case WNext:
      r = _right->nnf_not ();
      result = aalta_formula (Next, NULL, r).unique ();
      break;
    case And:
      l = _left->nnf_not ();
      r = _right->nnf_not ();
      result = aalta_formula (Or, l, r).unique ();
      break;
    case Or:
      l = _left->nnf_not ();
      r = _right->nnf_not ();
      result = aalta_formula (And, l, r).unique ();
      break;
    case Until:
      l = _left->nnf_not ();
      r = _right->nnf_not ();
      result = aalta_formula (Release, l, r).unique ();
      break;
    case Release:
      l = _left->nnf_not ();
      r = _right->nnf_not ();
      result = aalta_formula (Until, l, r).unique ();
      break;
    default:
      result = aalta_formula (Not, NULL, this).unique ();
      break;
  }
  return result;
}

/**
 * 重载赋值操作符
 * @param af
 * @return 
 */
aalta_formula& aalta_formula::operator = (const aalta_formula& af)
{
  if (this != &af)
    {
      this->_left = af._left;
      this->_right = af._right;
      this->_tag = af._tag;
      this->_op = af._op;
      this->_hash = af._hash;
      this->_length = af._length;
      this->_unique = af._unique;
      this->_simp = af._simp;
    }
  return *this;
}

/**
 * 重载等于符号
 * @param af
 * @return 
 */
bool aalta_formula::operator == (const aalta_formula& af) const
{
  return _left == af._left && _right == af._right
          && _op == af._op && _tag == af._tag;
}

/**
 * 重载小于号，stl_map中用
 * @param af
 * @return 
 */
bool aalta_formula::operator< (const aalta_formula& af) const
{
  if (_left != af._left)
    return _left < af._left;
  if (_right != af._right)
    return _right < af._right;
  if (_op != af._op)
    return _op < af._op;
  if (_tag != af._tag)
    return _tag < af._tag;
  return false;
}

/**
 * 将ltl_formula转成aalta_formula结构，
 * 并处理！运算，使其只会出现在原子前
 * @param formula
 * @param is_not 标记此公式前是否有！
 */
void
aalta_formula::build (const ltl_formula *formula, bool is_not, bool is_ltlf)
{
	aalta_formula *tmp_left, *tmp_right;
  if (formula == NULL) return;
  switch (formula->_type)
    {
    case eTRUE: // True - [! True = False]
      if (is_not) _op = False;
      else _op = True;
      break;
    case eFALSE: // False - [! False = True]
      if (is_not) _op = True;
      else _op = False;
      break;
    case eLITERAL: // 原子
      build_atom (formula->_var, is_not);
      break;
    case eNOT: // ! -- [!!a = a]
      build (formula->_right, is_not ^ 1, is_ltlf); //complement is_not
      break;
    case eNEXT: // X -- [!(Xa) = X(!a)] for LTL; [!(Xa) = N(!a)] for LTLf
      if(is_ltlf && is_not)
        _op = WNext;
      else
        _op = Next;
      _right = aalta_formula (formula->_right, is_not, is_ltlf).unique ();
      break;
    case eWNEXT: // N -- [!(Na) = X(!a)] for LTLf only
      assert (is_ltlf);
      if(is_not)
        _op = Next;
      else
        _op = WNext;
      _right = aalta_formula (formula->_right, is_not, is_ltlf).unique ();
      break;
    case eGLOBALLY: // G a = False R a -- [!(G a) = True U !a]
      if (is_not) _op = Until, _left = TRUE ();
      else _op = Release, _left = FALSE ();
      _right = aalta_formula (formula->_right, is_not, is_ltlf).unique ();
      break;
    case eFUTURE: // F a = True U a -- [!(F a) = False R !a]
      if (is_not) _op = Release, _left = FALSE ();
      else _op = Until, _left = TRUE ();
      _right = aalta_formula (formula->_right, is_not, is_ltlf).unique ();
      break;
    case eUNTIL: // a U b -- [!(a U b) = !a R !b]
      _op = is_not ? Release : Until;
      _left = aalta_formula (formula->_left, is_not, is_ltlf).unique ();
      _right = aalta_formula (formula->_right, is_not, is_ltlf).unique ();
      break;
    case eWUNTIL: // a W b = (G a) | (a U b) -- [!(a W b) = F !a /\ (!a R !b)]
      tmp_left = aalta_formula (formula->_left, is_not, is_ltlf).unique ();
      tmp_right = aalta_formula (formula->_right, is_not, is_ltlf).unique ();
      if (is_not)
      {
      	_op = And;
      	_left = aalta_formula (Until, TRUE (), tmp_left).unique ();
      	_right = aalta_formula (Release, tmp_left, tmp_right).unique ();
      }
      else
      {
      	_op = Or;
      	_left = aalta_formula (Release, FALSE (), tmp_left).unique ();
      	_right = aalta_formula (Until, tmp_left, tmp_right).unique ();
      }
      break;
    case eRELEASE: // a R b -- [!(a R b) = !a U !b]
      _op = is_not ? Until : Release;
      _left = aalta_formula (formula->_left, is_not, is_ltlf).unique ();
      _right = aalta_formula (formula->_right, is_not, is_ltlf).unique ();
      break;
    case eAND: // a & b -- [!(a & b) = !a | !b ]
      _op = is_not ? Or : And;
      _left = aalta_formula (formula->_left, is_not, is_ltlf).unique ();
      _right = aalta_formula (formula->_right, is_not, is_ltlf).unique ();
      break;
    case eOR: // a | b -- [!(a | b) = !a & !b]
      _op = is_not ? And : Or;
      _left = aalta_formula (formula->_left, is_not, is_ltlf).unique ();
      _right = aalta_formula (formula->_right, is_not, is_ltlf).unique ();
      break;
    case eIMPLIES: // a->b = !a | b -- [!(a->b) = a & !b]
      _op = is_not ? And : Or;
      _left = aalta_formula (formula->_left, is_not ^ 1, is_ltlf).unique ();
      _right = aalta_formula (formula->_right, is_not, is_ltlf).unique ();
      break;
    case eEQUIV:
      { // a<->b = (!a | b)&(!b | a) -- [!(a<->b) = (a & !b)|(!a & b)]
        ltl_formula *not_a = create_operation (eNOT, NULL, formula->_left);
        ltl_formula *not_b = create_operation (eNOT, NULL, formula->_right);
        ltl_formula *new_left = create_operation (eOR, not_a, formula->_right);
        ltl_formula *new_right = create_operation (eOR, not_b, formula->_left);
        ltl_formula *now = create_operation (eAND, new_left, new_right);
        *this = *(aalta_formula (now, is_not, is_ltlf).unique ());
        destroy_node (not_a);
        destroy_node (not_b);
        destroy_node (new_left);
        destroy_node (new_right);
        destroy_node (now);
        break;
      }
    default:
      print_error ("the formula cannot be recognized by aalta!");
      exit (1);
      break;
    }
}



/**
 * 添加原子变量
 * @param name
 * @param is_not
 */
inline void
aalta_formula::build_atom (const char *name, bool is_not)
{
  int id;
  hash_map<std::string, int>::const_iterator it = ids.find (name);
  if (it == ids.end ())
    { // 此变量名未出现过，添加之
      id = names.size ();
      ids[name] = id;
      names.push_back (name);
    }
  else
    {
      id = it->second;
    }
  if (is_not) _op = Not, _right = aalta_formula (id, NULL, NULL).unique ();
  else _op = id;
}

aalta_formula *
aalta_formula::classify (tag_t *tag)
{
  bool null = false;
  if (tag == NULL)
    {
      null = true;
      tag = new tag_t ();
    }
  aalta_formula ret = *this;
  tag_set::iterator iter = all_tags.find (tag);
  if (iter == all_tags.end ())
    {
      ret._tag = new tag_t (*tag);
      all_tags.insert (ret._tag);
    }
  else
    ret._tag = *iter;
  if (ret._left != NULL)
    ret._left = ret._left->classify (tag);
  bool until = false;
  if (ret._op == Until)
    {
      until = true;
      tag->push_back (this->unique ());
    }
  if (ret._right != NULL)
    ret._right = ret._right->classify (tag);
  ret._unique = NULL;
  if (ret._op == True || ret._op == False)
    ret._tag = NULL;
  ret.clc_hash ();
  if (until) tag->pop_back ();
  if (null) delete tag;
  ret._simp = ret.unique ();
  ret._simp->_simp = ret._simp;
  return ret.unique ();
}

/**
 * 简化并规划公式，简化的公式不包含tag信息
 * 使得And和Or的结构为链状结构而非树结构
 * 只修改this中的_simp值
 * @return 
 */
aalta_formula *
aalta_formula::simplify ()
{
  if (_simp != NULL) return _simp;

  switch (_op)
    {
    case And: // &
      _simp = aalta_formula::simplify_and (_left, _right);
      break;
    case Or: // |
      _simp = aalta_formula::simplify_or (_left, _right);
      break;
    case Next: // X
      _simp = aalta_formula::simplify_next (_right);
      break;
    case Until: // U
      _simp = aalta_formula::simplify_until (_left, _right);
      break;
    case Release: // R﻿
      _simp = aalta_formula::simplify_release (_left, _right);
      break;
    case Not: // ! 只会出现在原子前，因此可不做处理
      //break;
    default: //atom
      _simp = unique ();
      break;
    }

  _simp->_unique = _simp->_simp = _simp;
  return _simp;
}

/**
 * 将公式按op分离，并分别做simplify，最后将结果放在af_list中
 * @param op
 * @param af_list
 */
inline void
aalta_formula::split (int op, std::list<aalta_formula *>& af_list, bool to_simplify)
{
  std::list<aalta_formula *> store;
  aalta_formula *af = this;
  /* 非递归遍历公式树 */
  while (af != NULL)
    {
      if (af->_op == op)
        {
          store.push_back (af->_right);
          af = af->_left;
        }
      else
        {
          if (to_simplify) af = af->simplify ();
          if (af->_op == op) continue;
          af_list.push_back (af);
          if (store.empty ()) break;
          af = store.front ();
          store.pop_front ();
        }
    }
}

/**
 * 判断当前公式的op子公式是否与集合中的元素互斥, 即是否存在 a 和 !a
 * @param op
 * @param pos
 * @param neg
 * @return 
 */
bool
aalta_formula::mutex (int op, int_set& pos, int_set& neg)
{
  if (_op == op)
    {
      aalta_formula *af = this;
      while (af != NULL)
        {
          if (aalta_formula::mutex (af->af_now (op), pos, neg))
            return true;
          af = af->af_next (op);
        }
      return false;
    }
  return aalta_formula::mutex (this, pos, neg);
}

/**
 * 判断公式的构造形式, 判断制定位置的公式是否为af
 * @param pos 位置信息
 * @param op 当前公式的操作符
 * @param af
 * @return 
 */
bool
aalta_formula::contain (poskind pos, int op, aalta_formula *af)
{
  switch (pos)
    {
    case Left: // 公式操作符为op，且左边节点公式为af
      return _op == op && _left->unique () == af->unique ();
    case Right: // 公式操作符为op，且右边节点公式为af
      return _op == op && _right->unique () == af->unique ();
    case All: // 或和且操作符，判断里面是否存在一个af
      if (_op != op) return this->unique () == af->unique ();
      return (_left == NULL ? false : _left->contain (All, op, af)) ||
              (_right == NULL ? false : _right->contain (All, op, af));
    default:
      print_error ("can't analyse this position!");
    }
  return false;
}

/**
 * 同 contain (poskind, int, aalta_formula*)
 * 再往下一层
 * @param pos1
 * @param op1
 * @param pos2
 * @param op2
 * @param af
 * @return 
 */
bool
aalta_formula::contain (poskind pos1, int op1, poskind pos2, int op2, aalta_formula *af)
{
  switch (pos1)
    {
    case Left:
      return _op == op1 && _left->contain (pos2, op2, af);
    case Right:
      return _op == op1 && _right->contain (pos2, op2, af);
    case All:
      if (_op != op1) return contain (pos2, op2, af);
      return (_left == NULL ? false : _left->contain (All, op1, pos2, op2, af)) ||
              (_right == NULL ? false : _right->contain (All, op1, pos2, op2, af));
    default:
      print_error ("can't analyse this position!");
    }
  return false;
}

/**
 * 获取op操作符
 * @return 
 */
int
aalta_formula::oper () const
{
  return _op;
}

/**
 * 获取left节点
 * @return 
 */
aalta_formula *
aalta_formula::l_af () const
{
  return _left;
}

/**
 * 获取right节点
 * @return 
 */
aalta_formula *
aalta_formula::r_af () const
{
  return _right;
}

/**
 * 获取公式长度
 * @return 
 */
int
aalta_formula::get_length ()
{
  if (_length <= 0)
    {
      _length = 1;
      if (_left != NULL) _length += _left->get_length ();
      if (_right != NULL) _length += _right->get_length ();
    }
  return _length;
}

/**
 * 介于aalta_formula中对于与或的存储，若为与或，返回左节点, 为当前节点
 * @param op
 * @return 
 */
aalta_formula *
aalta_formula::af_now (int op)
{
  if (_op == op)
    return _left;
  return unique ();
}

/**
 * 介于aalta_formula中对于与或的存储，若为与或，返回右节点, 为下一个节点
 * @param op
 * @return 
 */
aalta_formula *
aalta_formula::af_next (int op) const
{
  if (_op == op)
    return _right;
  return NULL;
}

/**
 * 判是否为future
 * @return 
 */
//@ TODO: 倒是变成inline看看啊。。。

bool
aalta_formula::is_future () const
{
  return _op == Until && _left->_op == True;
}

/**
 * 判是否为globally
 * @return 
 */
//@ TODO: 倒是变成inline看看啊。。。

bool
aalta_formula::is_globally () const
{
  return _op == Release && _left->_op == False;
}

bool 
aalta_formula::is_until () const
{
  if (_op == Until && _left->_op != True)
    return true;
  if (_left != NULL && _left->is_until ())
    return true;
  if (_right != NULL && _right->is_until ())
    return true;
  return false;
}

bool 
aalta_formula::is_next () const
{
  if (_op == Next)
    return true;
  if (_left != NULL && _left->is_next ())
    return true;
  if (_right != NULL && _right->is_next ())
    return true;
  return false;

}

/**
 * 判公式中没有release算子
 * @return 
 */
bool
aalta_formula::release_free () const
{
  if (_op == Release)
    return false;
  if (_left != NULL && !_left->release_free ())
    return false;
  if (_right != NULL && !_right->release_free ())
    return false;
  return true;
}

/**
 * 克隆出该对象的副本（需要在外部显式delete）
 * @return 
 */
aalta_formula *
aalta_formula::clone () const
{
  aalta_formula *new_formula = new aalta_formula (*this);
  if (new_formula == NULL)
    {
      destroy ();
      print_error ("Memory error!");
      exit (1);
    }
  return new_formula;
}

int aalta_formula::_max_id = 1;

/**
 * 返回该aalta_formula对应的唯一指针
 * @return 
 */
aalta_formula *
aalta_formula::unique ()
{
  if (_unique == NULL)
    {
      afp_set::const_iterator iter = all_afs.find (this);
      if (iter != all_afs.end ())
        _unique = (*iter);
      else
        {
        
          _unique = clone ();
          _unique->_id = _max_id ++;
          all_afs.insert (_unique);
          
          //all_afs.insert (_unique = clone ());
          _unique->_unique = _unique;
        }

      //      af_prt_map::const_iterator iter = all_afs.find (*this);
      //      if (iter != all_afs.end ())
      //        _unique = iter->second;
      //      else
      //        all_afs[*this] = (_unique = clone ());
    }
  if (_unique->_simp == NULL)
    _unique->_simp = _simp;
  return _unique;
}

/**
 * 以逆波兰形式输出
 * @return 
 */
std::string
aalta_formula::to_RPN () const
{
  if (_left == NULL && _right == NULL)
    return names[_op];
  if (_left != NULL && _right != NULL)
    return names[_op] + " " + _left->to_RPN () + " " + _right->to_RPN ();
  return names[_op] + " " + (_left == NULL ? _right->to_RPN () : _left->to_RPN ());
}

/**
 * To String
 * @return 
 */
std::string
aalta_formula::to_string () const
{
  if (_left == NULL && _right == NULL)
    return names[_op];// + "[" + convert_to_string(_id) + "]";
  if (_left == NULL)
    return "(" + names[_op] + " " + _right->to_string () + ")";// + "[" + convert_to_string(_id) + "]";
  if (_right == NULL)
    return "(" + _left->to_string () + " " + names[_op] + ")";// + "[" + convert_to_string(_id) + "]";
  return "(" + _left->to_string () + " " + names[_op] + " " + _right->to_string () + ")";// + "[" + convert_to_string(_id) + "]";
}


aalta_formula* aalta_formula::add_tail ()
{
	aalta_formula *res, *l, *r;
	if (oper () == Next)
	{
		r = _right->add_tail ();
		res = aalta_formula (Next, NULL, r).unique ();
		aalta_formula *not_tail = aalta_formula (Not, NULL, TAIL ()).unique ();
		res = aalta_formula (And, not_tail, res).unique ();
	}
	else
	{
		if (_left != NULL)
			l = _left->add_tail ();
		else 
			l = NULL;
		if (_right != NULL)
			r = _right->add_tail ();
		else
			r = NULL;
		res = aalta_formula (oper (), l, r).unique ();
	}
	return res;
}


aalta_formula* 
aalta_formula::split_next ()
{
	aalta_formula *l, *r, *res;
	if (oper () == Next || oper () == WNext)
	{
		if (_right->oper () == And || _right->oper () == Or)
		{
			l = aalta_formula (oper (), NULL, _right->l_af ()).unique ();
			r = aalta_formula (oper (), NULL, _right->r_af ()).unique ();
			l = l->split_next ();
			r = r->split_next ();
			res = aalta_formula (_right->oper (), l, r).unique ();
		}
		else
		{
			r = _right->split_next ();
			res = aalta_formula (oper (), NULL, r).unique ();
			if (r->oper () != And && r->oper () != Or)
				return res;
			return res->split_next ();
		}
	}
	else if (oper () > Undefined)
		return this;
	else
	{
		if (_left != NULL)
			l = _left->split_next ();
		else
			l = NULL;
		if (_right != NULL)
			r = _right->split_next ();
		else
			r = NULL;
		res = aalta_formula (oper (), l, r).unique ();
	}
	return res;
}

aalta_formula* 
aalta_formula::remove_wnext ()
{
	
	aalta_formula *res, *l, *r;
	// N f <-> Tail | X f
	if (oper () == WNext)
	{
		r = _right->remove_wnext ();
		aalta_formula* tail = aalta_formula ("Tail").unique ();
		aalta_formula* nf = aalta_formula (Next, NULL, r).unique ();
		res = aalta_formula (Or, tail, nf).unique ();
	}
	else 
	{
		if (_left != NULL)
			l = _left->remove_wnext ();
		else
			l = NULL;
		if (_right != NULL)
			r = _right->remove_wnext ();
		else
			r = NULL;
		res = aalta_formula (oper (), l, r).unique ();
	}
	return res;
}

aalta_formula* aalta_formula::TAIL_ = NULL;
aalta_formula* 
aalta_formula::TAIL ()
{
	if (TAIL_ == NULL)
		TAIL_ = aalta_formula ("Tail").unique ();
	return TAIL_;
}






/*
 * The followings are modified by Jianwen Li on December 7th, 2013
 *
 */
bool
aalta_formula::is_global()
{
  if(oper() == Release && _left->oper() == False)
    return true;
  else
  {
    if(oper() == And || oper() == Or)
      return _left->is_global() && _right->is_global();
  }
  return false;
}

bool 
aalta_formula::is_wnext_free()
{
  if(oper() == WNext)
    return false;
  if(_left != NULL)
  {
    if(!_left->is_wnext_free())
      return false;
  }
  if(_right != NULL)
  {
    if(!_right->is_wnext_free())
      return false;
  }
  return true;
}

aalta_formula*
aalta_formula::ofg()
{
  aalta_formula *result;
  switch(oper())
  {
    case Or:
      result = aalta_formula(Or, _left->ofg(), _right->ofg()).unique();
      break;
    case And:
      result = aalta_formula(And, _left->ofg(), _right->ofg()).unique();
      break;
    case Next:
      result = FALSE();
      break;
    case WNext:
      result = TRUE();
      break;
    case Until:
    case Release:
      result = _right->ofg();
      break;
    case Undefined:
      result = NULL;
      break;
    default:
      result = this;
      break;
  }
  return result->simplify();
}

aalta_formula*
aalta_formula::off()
{
  aalta_formula *result;
  switch(oper())
  {
    case Or:
      result = aalta_formula(Or, _left->off(), _right->off()).unique();
      break;
    case And:
      result = aalta_formula(And, _left->off(), _right->off()).unique();
      break;
    case Next:
    case WNext:
    case Until:
      result = _right->off();
      break;
    case Release:
      result = _right->ofr();
      break;
    case Undefined:
      result = NULL;
      break;
    default:
      result = this;
      break;
  }
  
  return result->simplify();
}

aalta_formula*
aalta_formula::ofr()
{
  aalta_formula *result;
  switch(oper())
  {
    case Or:
      result = aalta_formula(Or, _left->ofr(), _right->ofr()).unique();
      break;
    case And:
      result = aalta_formula(And, _left->ofr(), _right->ofr()).unique();
      break;
    case Next:
    case WNext:
      result = FALSE ();
      break;
    case Until:
    case Release:
      result = _right->ofr();
      break;
    case Undefined:
      result = NULL;
      break;
    default:
      result = this;
      break;
  }
  return result;
}

aalta_formula*
aalta_formula::cf()
{
  aalta_formula *result, *f1, *f2;
  switch(oper())
  {
    //case Until:
    case Or:
      f1 = _left->cf ();
      f2 = _right->cf ();
      if (f1->_op == True)
        result = f1;
      else if (f1->_op == False)
        result = f2;
      else if (f2->_op == True)
        result = f2;
      else if (f2->_op == False)
        result = f1;
      else
        result = aalta_formula(Or, f1, f2).unique();
      break;
    case And:
      f1 = _left->cf ();
      f2 = _right->cf ();
      if (f1->_op == True)
        result = f2;
      else if (f1->_op == False)
        result = f1;
      else if (f2->_op == True)
        result = f1;
      else if (f2->_op == False)
        result = f2;
      else
        result = aalta_formula(And, f1, f2).unique();
      break;
    case Next:
      result = TRUE();
      break;
    
    case Until:
      f1 = _left->cf ();
      f2 = _right->cf ();
      if (f2 == aalta_formula::FALSE ())
        result = f2;
      else if (f1->_op == True)
        result = f1;
      else if (f1->_op == False)
        result = f2;
      else if (f2->_op == True)
        result = f2;
      else
        result = aalta_formula(Or, f1, f2).unique();
      break;
    
    case Release:
      result = _right->cf ();
      break;
    case Undefined:
      result = NULL;
      break;
    default:
      result = this;
      break;
  }
  //printf ("%s\n", result->to_string().c_str ());
  return result;
}

bool 
aalta_formula::model(aalta_formula *af)
{
  af_prt_set P = af->to_set();
  return model(P);
}

bool 
aalta_formula::model(af_prt_set P)
{
  switch(oper())
  {
    case True:
      return true;
    case False:
      return false;
    case Literal:
      if(P.find(this) != P.end())
        return true;
      else
        return false;
    case Or:
      return _left->model(P) || _right->model(P);
    case And:
      return _left->model(P) && _right->model(P);
    case Next:
      return false;
    case WNext:
      return true;
    case Until:
    case Release:
      return _right->model(P);
    case Undefined:
      return false;
    default:
      if(P.find(this) != P.end())
        return true;
      else
        return false;
  }
}

//formula progression algorithm
aalta_formula*
aalta_formula::progf(af_prt_set P)
{
  aalta_formula *result = NULL, *until = NULL, *l = NULL, *r = NULL;
  switch(oper())
  {
    case True:
    case False:
      result = this;
      break;
    case Or:  
      l = _left->progf(P);
      r = _right->progf(P);
      if(l == r)
        result = l;
      else if (l == TRUE ())
        result = l;
      else if (l == FALSE ())
        result = r;
      else if (r == TRUE ())
        result = r;
      else if (r == FALSE ())
        result = l;
      else 
        result = aalta_formula(Or, l, r).unique();
      break;
    case And:
      l = _left->progf(P);
      r = _right->progf(P);
      if(l == r)
        result = l;
      else if (l == TRUE ())
        result = r;
      else if (l == FALSE ())
        result = l;
      else if (r == TRUE ())
        result = l;
      else if (r == FALSE ())
        result = r;
      else 
        result = aalta_formula(And, l, r).unique();
      break;
    case Next:
      result = _right;
      break;
    case Until:
      l = _left->progf (P);
      r = _right->progf (P);
      until = mark_until ();
      //printf ("TRUE is : %d\n", (size_t)TRUE());
      if (r == TRUE ())
        r = until;
      else if (r != FALSE ())
        r = aalta_formula (And, r, until).unique ();
      if (l == TRUE ())
        l = this;
      else if (l != FALSE ())
        l = aalta_formula (And, l, this).unique ();
      if (l == TRUE ())
        result = l;
      else if (l == FALSE ())
        result = r;
      else if (r == until)
        result = r;
      else if (r == FALSE ())
        result = l;
      else
        result = aalta_formula (Or, r, l).unique ();
      break;
    case Release:
      l = _left->progf (P);
      r = _right->progf (P);
      if (l == FALSE ())
        l = this;
      else if (l != TRUE ())
        l = aalta_formula (Or, l, this).unique ();

      if (l == TRUE ())
        result = r;
      else if (l == FALSE ())
        result = l;
      else if (r == TRUE ())
        result = l;
      else if (r == FALSE ())
        result = r;
      else 
        result = aalta_formula (And, r, l).unique ();
        
      break;
    case Undefined:
      printf ("aalta_formula::progf: Undefined types!\n");
      exit (0);
      
    default:
      if(P.find(this) != P.end())
        result = TRUE();
      else
        result = FALSE();
      break;
  }
  return result;
}

hash_map<aalta_formula*, aalta_formula *, aalta_formula::af_prt_hash> aalta_formula::_until_map;
hash_map<aalta_formula*, aalta_formula *, aalta_formula::af_prt_hash> aalta_formula::_var_until_map;

aalta_formula* 
aalta_formula::get_var ()
{
  if (oper () != Until)
  {
     printf ("aalta_formula::get_var: the formula is not Until!\n");
     exit (0);
  }
  hash_map<aalta_formula*, aalta_formula*, af_prt_hash>::iterator it;
  it = _until_map.find (this);
  if (it != _until_map.end ())
    return it->second;
  else
  {
    aalta_formula *res = mark_until ();
    return res;
    //printf ("aalta_formula::get_var: Cannot find the variable for the until formula\n%s\n", to_string().c_str ());
    //exit (0);
  }
  return NULL;
}

aalta_formula* 
aalta_formula::get_until ()
{
  hash_map<aalta_formula*, aalta_formula*, af_prt_hash>::iterator it;
  it = _var_until_map.find (this);
  if (it != _var_until_map.end ())
    return it->second;
  else
  {
    printf ("aalta_formula::get_until: Cannot find the until formula for the variable %s!\n", to_string ().c_str ());
    exit (0);
  }
  return NULL;
}

aalta_formula* 
aalta_formula::mark_until ()
{
  if (oper () != Until)
  {
     printf ("aalta_formula::mark_until: the formula is not Until!\n");
     exit (0);
  }
  aalta_formula *result;
  hash_map<aalta_formula*, aalta_formula*, af_prt_hash, af_prt_eq>::iterator it;
  it = _until_map.find (this);
  if (it != _until_map.end ())
    result = it->second;
  else
  {
    std::string name = "FOR_UNTIL_";
    int pos = _until_map.size ();
    name += convert_to_string (pos);
    hash_map<std::string, int>::const_iterator it = ids.find (name);
    int id;
    if (it == ids.end ())
    { // the variable is not exisited
      id = names.size ();
      ids[name] = id;
      names.push_back (name);
    }
    else
    {
      id = it->second;
    }
    result = aalta_formula (id, NULL, NULL).unique ();
    _until_map.insert (pair<aalta_formula*, aalta_formula*> (this, result));
    _var_until_map.insert (pair<aalta_formula*, aalta_formula*> (result, this));
  }
  return result;
}

bool 
aalta_formula::model_until (aalta_formula::af_prt_set P)
{
  hash_map<aalta_formula*, aalta_formula *, af_prt_hash>::iterator it;
  switch (oper ())
  {
    case Until:
      it = _until_map.find (this);
      if (it == _until_map.end ())
      {
        printf ("aalta_formula::model_until: the until formula is not found in _until_map!\n");
        exit (0);
      }
      if (P.find (it->second) != P.end ())
        return true;
      else 
        return false;
    case Release:
      return true;
    case And:
      return _left->model_until (P) && _right->model_until (P);
    case Or:
      return _left->model_until (P) || _right->model_until (P);
    case Undefined:
      return false;
    default:
      return true;
  }
  return false;
}

aalta_formula::af_prt_set 
aalta_formula::get_until_flags ()
{
  aalta_formula::af_prt_set result, P;
  if (oper () == And)
  {
    P = _left->get_until_flags ();
    result.insert (P.begin (), P.end ());
    P = _right->get_until_flags ();
    result.insert (P.begin (), P.end ());
  }
  else
  {
    if (oper () > Undefined)
    {
      string str = names[oper ()];
      if (str.find ("FOR_UNTIL_") != string::npos)
        result.insert (this);
    }
  }
  return result;
}

aalta_formula* 
aalta_formula::normal ()
{
  aalta_formula *result = NULL, *f1 = NULL, *f2 = NULL;
  if (oper () == And)
  {
    f1 = _left->normal ();
    f2 = _right->normal ();
    if (f1 == TRUE ())
      result = f2;
    else if (f2 == TRUE ())
      result = f1;
    else
      result = aalta_formula (And, f1, f2).unique ();
  }
  else
  {
    if (oper () > Undefined)
    {
      string str = names[oper ()];
      if (str.find ("FOR_UNTIL_") != string::npos)
        result = TRUE ();
      else
        result = unique ();
    }
    else
      result = unique ();
  }
  return result;
}

bool 
aalta_formula::is_until_marked ()
{
  if (oper () <= Undefined)
    return false;
  string str = names[oper ()];
  if (str.find ("FOR_UNTIL_") != string::npos)
    return true;
  else
    return false;
}

aalta_formula* 
aalta_formula::flatted ()
{
  aalta_formula *result, *l, *r, *nx, *until, *not_until;
  switch (oper ())
  {
    case Until:
      until = mark_until ();
      l = _right->flatted ();
      if (l == TRUE ())
        l = until;
      else if (l != FALSE ())
        l = aalta_formula (And, until, l).unique ();
        
      r = _left->flatted ();
      not_until = aalta_formula (Not, NULL, until).unique ();
      if (r == TRUE ())
        r = not_until;
      else if (r != FALSE ())
        r = aalta_formula (And, not_until, r).unique ();
      nx = aalta_formula (Next, NULL, this).unique ();
      if (r != FALSE ())
        r = aalta_formula (And, r, nx).unique ();
        
      if (l == TRUE ())
        result = l;
      else if (l == FALSE ())
        result = r;
      else if (r == TRUE ())
        result = r;
      else if (r == FALSE ())
        result = l;
      else
        result = aalta_formula (Or, r, l).unique ();
      break;
    case Release:
      l = _right->flatted ();
      r = _left->flatted ();
      nx = aalta_formula (Next, NULL, this).unique ();
      if (r == FALSE ())
        r = nx;
      else if (r != TRUE ())
        r = aalta_formula (Or, r, nx).unique ();
        
      if (l == TRUE ())
        result = r;
      else if (l == FALSE ())
        result = l;
      else if (r == TRUE ())
        result = l;
      else if (r == FALSE ())
        result = r;
      else
        result = aalta_formula (And, l, r).unique ();
      break;
    case And:
      l = _right->flatted ();
      r = _left->flatted ();
      if (l == TRUE ())
        result = r;
      else if (l == FALSE ())
        result = l;
      else if (r == TRUE ())
        result = l;
      else if (r == FALSE ())
        result = r;
      else
        result = aalta_formula (And, l, r).unique ();
      break;
    case Or:
      l = _right->flatted ();
      r = _left->flatted ();
      if (l == TRUE ())
        result = l;
      else if (l == FALSE ())
        result = r;
      else if (r == TRUE ())
        result = r;
      else if (r == FALSE ())
        result = l;
      else
        result = aalta_formula (Or, r, l).unique ();
      break;
    default:
      result = this;
  }
  return result;
}

bool 
aalta_formula::contain (af_prt_set P1, af_prt_set P2)
{
  af_prt_set::iterator it;
  for (it = P2.begin (); it != P2.end (); it ++)
  {
    if (P1.find (*it) == P1.end ())
      return false;
  }
  return true;
}


int aalta_formula::_sat_count = 0;

void 
aalta_formula::print_sat_count ()
{
  printf ("Total SAT invoking: %d\n", _sat_count);
}

aalta_formula::af_prt_set 
aalta_formula::SAT()    //true or false cannot be the input!!!
{
  //assert (_op != True);
  //assert (_op != False);
  af_prt_set P;
  
  //Be careful when the formula is true or false
  if (_op == True)
  {
    P.insert (TRUE ());
    return P;
  }
  if (_op == False)
    return P;
  /*
  aalta_formula* f = erase_next_global (P);
  if (f != this)
  {
    if (f != NULL)
    {
      af_prt_set P2 = f->SAT_core ();
      if (!P2.empty ())
        P2.insert (P.begin (), P.end ());
      return P2;
    }
    else
      return P;
  }
  else
    return SAT_core ();
  */
  
  return SAT_core ();
   
  
}

aalta_formula*
aalta_formula::erase_next_global (aalta_formula::af_prt_set& P)
{
  aalta_formula *l = NULL, *r = NULL, *res = NULL;
  if (oper () != And)
  {
    if (oper () == Next && _right->is_globally ())
    {
      P.insert (this);
      return NULL;
    }
    else
      return this;
  }
  else
  {
    l = _left->erase_next_global (P);
    r = _right->erase_next_global (P);
    if (l == NULL)
      res = r;
    else if (r == NULL)
      res = l;
    else
      res = aalta_formula (aalta_formula::And, l, r).unique ();
    return res;
  }
}

aalta_formula::af_prt_set 
aalta_formula::SAT_core()
{
  af_prt_set P;
  _sat_count ++;
  int_prt_map prop_map;
  int var_num;
  Minisat::Solver S;
  
  /*
  std::vector<std::vector<int> > cls = toDIMACS (prop_map);
  int var;
  for (int i = 0; i < cls.size (); i ++)
  {
    vec<Lit> lits;
    for (int j = 0; j < cls[i].size (); j ++)
    {
      //printf ("%d ", cls[i][j]);
      assert (cls[i][j] != 0); 
      var = abs(cls[i][j])-1;
      while (var >= S.nVars()) S.newVar();
      if (cls[i][j] > 0)
        lits.push (mkLit (var));
      else
        lits.push (~mkLit (var));
    }
    //printf ("\n");
    S.addClause_(lits);
  */
  
  
  int cls[MAX_CL][3];
  int cls_num = 0;
  toDIMACS (prop_map, cls, cls_num);
  
  //printf ("SAT formula is\n%s\n", to_string().c_str ());
  
  int var;
  for (int i = 0; i < cls_num; i ++)
  {
    vec<Lit> lits;
    for (int j = 0; j < 3; j ++)
    {
      //printf ("%d ", cls[i][j]);
      if (cls[i][j] == 0)   break; 
      var = abs(cls[i][j])-1;
      while (var >= S.nVars()) S.newVar();
      if (cls[i][j] > 0)
        lits.push (mkLit (var));
      else
        lits.push (~mkLit (var));
    }
    //printf ("\n");
    S.addClause_(lits);
  }
  
  
  if (!S.simplify())
  {
    return P;             //false
  }
  Minisat::vec<Minisat::Lit> dummy;
  Minisat::lbool ret;
  ret = S.solveLimited(dummy);
  
  //handle the result from Minisat
  if(ret == l_True)
  {

    int_prt_map::iterator it;

    for(int i = 0; i < S.nVars(); i++)
    {
      it = prop_map.find (i+1);
      if(it != prop_map.end())
      {
        if(S.model[i] == l_True)
          P.insert (it->second);
        else
          P.insert (aalta_formula (Not, NULL, it->second).unique ());
      }

    }
  }
 
  return P;
}

void 
aalta_formula::toDIMACS(int_prt_map& prop_map, int (&cls)[MAX_CL][3], int& cls_num)
{
  int var_num = 1;
  int cl_num = 1;
  af_int_map int_map;
  if(oper() != And && oper() != Or)
  {
    if(oper () == Not)
    {
      assert (_right->oper () != And && _right->oper () != Or);
      prop_map[1] = _right;
      //return "p cnf 1 1 \n-1 0\n";
      cls[cls_num][0] = -1;
      cls[cls_num][1] = 0;
      cls_num ++;
    }
    else
    {
      prop_map[1] = this;
      //return "p cnf 1 1 \n1 0\n";
      cls[cls_num][0] = 1;
      cls[cls_num][1] = 0;
      cls_num ++;
    }
  }
  else
  {
    if (oper () == Not)
    {
      cls[cls_num][0] = -1;
      cls[cls_num][1] = 0;
      cls_num ++;
    }
    else
    {
      cls[cls_num][0] = 1;
      cls[cls_num][1] = 0;
      cls_num ++;
    }
    
    plus(prop_map, int_map, var_num, cl_num, 1, cls, cls_num);

  }
}

std::vector<std::vector<int> > 
aalta_formula::toDIMACS(int_prt_map& prop_map)
{
  std::vector<std::vector<int> > res;
  std::vector<int> lits;
  int var_num = 1;
  int cl_num = 1;
  af_int_map int_map;
  if(oper() != And && oper() != Or)
  {
    if(oper () == Not)
    {
      assert (_right->oper () != And && _right->oper () != Or);
      prop_map[1] = _right;
      //return "p cnf 1 1 \n-1 0\n";
      lits.push_back (-1);
      lits.push_back (0);
      res.push_back (lits);
      return res;
    }
    else
    {
      prop_map[1] = this;
      //return "p cnf 1 1 \n1 0\n";
      lits.push_back (1);
      lits.push_back (0);
      res.push_back (lits);
      return res;
    }
  }
  else
  {
    //std::string result = "";
    //result = plus(prop_map, int_map, var_num, cl_num, 1);
    std::vector<int> lits;
    if (oper () == Not)
    {
      lits.push_back (-1);
      res.push_back (lits);
    }
    else
    {
      lits.push_back (1);
      res.push_back (lits);
    }
    
    plus(prop_map, int_map, var_num, cl_num, 1, res);
    /*
    if (oper () == Not)
      result = "p cnf " + convert_to_string(var_num) + " " + convert_to_string(cl_num) + "\n-1 0\n" + result;
    else
      result = "p cnf " + convert_to_string(var_num) + " " + convert_to_string(cl_num) + "\n1 0\n" + result;
    return result;
    */
    return res;
  }
}

int 
aalta_formula::search(aalta_formula af, af_int_map& int_map, int& var_num)
{
  int result;
  af_int_map::iterator it = int_map.find(af);
  if(it != int_map.end())
  {
    result = it->second;
  }
  else
  {
    result = ++var_num;
    int_map[af] = result;
  }
  return result;
}

void 
aalta_formula::plus(int_prt_map& prop_map, af_int_map& int_map, int& var_num, int& cl_num, int cur, int (&cls)[MAX_CL][3], int &cls_num)
{
  
  int findex, f1index, f2index;
  findex = cur;
  //std::string result = "";
  switch (oper ())
  {
    case Not:
    {
      //result = _right->minus (prop_map, int_map, var_num, cl_num, findex);
      _right->minus (prop_map, int_map, var_num, cl_num, findex, cls, cls_num);
      break;
    }
    case And:
    {
      switch (_left->oper ())
      {
        case Not:
          if (_left->_right->oper () == And || _left->_right->oper () == Or)
            f1index = ++var_num;
          else if (_left->_right->oper () == Not)
          {
            printf ("Error: two Not together in aalta_formula::TODAMICS-AND\n");
            exit (0);
          }
          else
          {
            f1index = search (*(_left->_right), int_map, var_num);
            prop_map[f1index] = _left->_right;
          }
          f1index = -f1index;
          break;
        case And:
        case Or:
          f1index = ++var_num;
          break;
        default:
          f1index = search (*_left, int_map, var_num);
          prop_map[f1index] = _left;
          break;
      }
      
      switch (_right->oper ())
      {
        case Not:
          if (_right->_right->oper () == And || _right->_right->oper () == Or)
            f2index = ++var_num;
          else if (_right->_right->oper () == Not)
          {
            printf ("Error: two Not together in aalta_formula::TODIMACS-AND\n");
            exit (0);
          }
          else
          {
            f2index = search (*(_right->_right), int_map, var_num);
            prop_map[f2index] = _right->_right;
          }

          f2index = -f2index;
          break;
        case And:
        case Or:
          f2index = ++var_num;
          break;
        default:
          f2index = search (*_right, int_map, var_num);
          prop_map[f2index] = _right;
          break;
      }
      
      //result = convert_to_string(-findex) + " " + convert_to_string(f1index) + " 0\n" 
      //         + convert_to_string(-findex) + " " + convert_to_string(f2index) + " 0\n";
      cls[cls_num][0] = -findex;
      cls[cls_num][1] = f1index;
      cls[cls_num][2] = 0;
      cls_num ++;
      assert (cls_num < MAX_CL);
      
      cls[cls_num][0] = -findex;
      cls[cls_num][1] = f2index;
      cls[cls_num][2] = 0;
      cls_num ++;
      assert (cls_num < MAX_CL);
      
      /*
      int ints[] = {-findex, f1index};
      std::vector<int> lits (ints, ints + sizeof (ints)/sizeof (int));
      //lits.push_back (-findex);
      //lits.push_back (f1index);
      res.push_back (lits);
      int ints2[] = {-findex, f2index};
      std::vector<int> lits2 (ints2, ints2 + sizeof (ints2)/sizeof (int));
      //lits2.push_back (-findex);
      //lits2.push_back (f2index);
      res.push_back (lits2);
      */
      
      cl_num += 2;
      //result += _left->plus (prop_map, int_map, var_num, cl_num, f1index);
      //result += _right->plus (prop_map, int_map, var_num, cl_num, f2index);
      _left->plus (prop_map, int_map, var_num, cl_num, f1index, cls, cls_num);
      _right->plus (prop_map, int_map, var_num, cl_num, f2index, cls, cls_num);
      break;
    }  
    case Or:
    {
      switch (_left->oper ())
      {
        case Not:
          if (_left->_right->oper () == And || _left->_right->oper () == Or)
            f1index = ++var_num;
          else if (_left->_right->oper () == Not)
          {
            printf ("Error: two Not together in aalta_formula::TODAMICS-OR\n");
            exit (0);
          }
          else
          {
            f1index = search (*(_left->_right), int_map, var_num);
            prop_map[f1index] = _left->_right;
          }
          f1index = -f1index;
          break;
        case And:
        case Or:
          f1index = ++var_num;
          break;
        default:
          f1index = search (*_left, int_map, var_num);
          prop_map[f1index] = _left;
          break;
      }
      
      switch (_right->oper ())
      {
        case Not:
          if (_right->_right->oper () == And || _right->_right->oper () == Or)
            f2index = ++var_num;
          else if (_right->_right->oper () == Not)
          {
            printf ("Error: two Not together in aalta_formula::TODAMICS-OR\n");
            exit (0);
          }
          else
          {
            f2index = search (*(_right->_right), int_map, var_num);
            prop_map[f2index] = _right->_right;
          }

          f2index = -f2index;
          break;
        case And:
        case Or:
          f2index = ++var_num;
          break;
        default:
          f2index = search (*_right, int_map, var_num);
          prop_map[f2index] = _right;
          break;
      }
      
      //result = convert_to_string(-findex) + " " + convert_to_string(f1index) + " "
      //         + convert_to_string(f2index) + " 0\n" ;
      cls[cls_num][0] = -findex;
      cls[cls_num][1] = f1index;
      cls[cls_num][2] = f2index;
      cls_num ++;
      assert (cls_num < MAX_CL);
      
      /*
      int ints[] = {-findex, f1index, f2index};
      std::vector<int> lits (ints, ints + sizeof (ints)/sizeof (int));
      //lits.push_back (-findex);
      //lits.push_back (f1index);
      //lits.push_back (f2index);
      res.push_back (lits);
      */
      
      cl_num += 1;
      //result += _left->plus (prop_map, int_map, var_num, cl_num, f1index);
      //result += _right->plus (prop_map, int_map, var_num, cl_num, f2index);
      _left->plus (prop_map, int_map, var_num, cl_num, f1index, cls, cls_num);
      _right->plus (prop_map, int_map, var_num, cl_num, f2index, cls, cls_num);
      break;
    }
    default:
      break;
      
  }
  //return result;
}

//std::string 
void 
aalta_formula::minus (int_prt_map& prop_map, af_int_map& int_map, int& var_num, int& cl_num, int cur, int (&cls)[MAX_CL][3], int &cls_num)
{
  int findex, f1index, f2index;
  findex = cur;
  std::string result = "";
  switch (oper ())
  {
    case Not:
      //result = _right->plus (prop_map, int_map, var_num, cl_num, findex);
      _right->plus (prop_map, int_map, var_num, cl_num, findex, cls, cls_num);
      break;
    case And:
    {
      switch (_left->oper ())
      {
        case Not:
          if (_left->_right->oper () == And || _left->_right->oper () == Or)
            f1index = ++var_num;
          else if (_left->_right->oper () == Not)
          {
            printf ("Error: two Not together in aalta_formula::TODAMICS-AND\n");
            exit (0);
          }
          else
          {
            f1index = search (*(_left->_right), int_map, var_num);
            prop_map[f1index] = _left->_right;
          }
          //f1index = -f1index;
          break;
        case And:
        case Or:
          f1index = ++var_num;
          f1index = -f1index;
          break;
        default:
          f1index = search (*_left, int_map, var_num);
          prop_map[f1index] = _left;
          f1index = -f1index;
          break;
      }
      
      switch (_right->oper ())
      {
        case Not:
          if (_right->_right->oper () == And || _right->_right->oper () == Or)
            f2index = ++var_num;
          else if (_right->_right->oper () == Not)
          {
            printf ("Error: two Not together in aalta_formula::TODIMACS-AND\n");
            exit (0);
          }
          else
          {
            f2index = search (*(_right->_right), int_map, var_num);
            prop_map[f2index] = _right->_right;
          }

          //f2index = -f2index;
          break;
        case And:
        case Or:
          f2index = ++var_num;
          f2index = -f2index;
          break;
        default:
          f2index = search (*_right, int_map, var_num);
          prop_map[f2index] = _right;
          f2index = -f2index;
          break;
      }
      
      //result = convert_to_string(-findex) + " " + convert_to_string(f1index) + " "
      //         + convert_to_string(f2index) + " 0\n" ;
      
      cls[cls_num][0] = -findex;
      cls[cls_num][1] = f1index;
      cls[cls_num][2] = f2index;
      cls_num ++;
      assert (cls_num < MAX_CL);
      
      /*
      int ints[] = {-findex, f1index, f2index};
      std::vector<int> lits (ints, ints + sizeof (ints)/sizeof (int));
      //lits.push_back (-findex);
      //lits.push_back (f1index);
      //lits.push_back (f2index);
      res.push_back (lits);
      */
      
      cl_num += 1;
      //result += _left->minus (prop_map, int_map, var_num, cl_num, f1index);
      //result += _right->minus (prop_map, int_map, var_num, cl_num, f2index);
      _left->minus (prop_map, int_map, var_num, cl_num, f1index, cls, cls_num);
      _right->minus (prop_map, int_map, var_num, cl_num, f2index, cls, cls_num);
      break;
    }  
    case Or:
    {
      switch (_left->oper ())
      {
        case Not:
          if (_left->_right->oper () == And || _left->_right->oper () == Or)
            f1index = ++var_num;
          else if (_left->_right->oper () == Not)
          {
            printf ("Error: two Not together in aalta_formula::TODAMICS-OR\n");
            exit (0);
          }
          else
          {
            f1index = search (*(_left->_right), int_map, var_num);
            prop_map[f1index] = _left->_right;
          }
          //f1index = -f1index;
          break;
        case And:
        case Or:
          f1index = ++var_num;
          f1index = -f1index;
          break;
        default:
          f1index = search (*_left, int_map, var_num);
          prop_map[f1index] = _left;
          f1index = -f1index;
          break;
      }
      
      switch (_right->oper ())
      {
        case Not:
          if (_right->_right->oper () == And || _right->_right->oper () == Or)
            f2index = ++var_num;
          else if (_right->_right->oper () == Not)
          {
            printf ("Error: two Not together in aalta_formula::TODAMICS-OR\n");
            exit (0);
          }
          else
          {
            f2index = search (*(_right->_right), int_map, var_num);
            prop_map[f2index] = _right->_right;
          }

          //f2index = -f2index;
          break;
        case And:
        case Or:
          f2index = ++var_num;
          f2index = -f2index;
          break;
        default:
          f2index = search (*_right, int_map, var_num);
          prop_map[f2index] = _right;
          f2index = -f2index;
          break;
      }
      
      cls[cls_num][0] = -findex;
      cls[cls_num][1] = -f1index;
      cls[cls_num][2] = f2index;
      cls_num ++;
      assert (cls_num < MAX_CL);
      
      cls[cls_num][0] = -findex;
      cls[cls_num][1] = f1index;
      cls[cls_num][2] = -f2index;
      cls_num ++;
      assert (cls_num < MAX_CL);
      
      cls[cls_num][0] = -findex;
      cls[cls_num][1] = f1index;
      cls[cls_num][2] = f2index;
      cls_num ++;
      assert (cls_num < MAX_CL);
      
      /*
      int ints[] = {-findex, -f1index, f2index};
      std::vector<int> lits (ints, ints + sizeof (ints)/sizeof (int));
      //lits.push_back (-findex);
      //lits.push_back (-f1index);
      //lits.push_back (f2index);
      res.push_back (lits);
      int ints2[] = {-findex, f1index, -f2index};
      std::vector<int> lits2 (ints2, ints2 + sizeof (ints2)/sizeof (int));
      //lits2.push_back (-findex);
      //lits2.push_back (f1index);
      //lits2.push_back (-f2index);
      res.push_back (lits2);
      int ints3[] = {-findex, f1index, f2index};
      std::vector<int> lits3 (ints3, ints3 + sizeof(ints3)/sizeof(int));
      //lits3.push_back (-findex);
      //lits3.push_back (f1index);
      //lits3.push_back (f2index);
      res.push_back (lits3);
      */
       
      cl_num += 3;
      //result += _left->minus (prop_map, int_map, var_num, cl_num, f1index);
      //result += _right->minus (prop_map, int_map, var_num, cl_num, f2index);
      _left->minus (prop_map, int_map, var_num, cl_num, f1index, cls, cls_num);
      _right->minus (prop_map, int_map, var_num, cl_num, f2index, cls, cls_num);
      break;
    }
    default:
      break;
      
  }
  //return result;
}


void 
aalta_formula::plus(int_prt_map& prop_map, af_int_map& int_map, int& var_num, int& cl_num, int cur, std::vector<std::vector<int> >& res)
{
  
  int findex, f1index, f2index;
  findex = cur;
  //std::string result = "";
  switch (oper ())
  {
    case Not:
    {
      //result = _right->minus (prop_map, int_map, var_num, cl_num, findex);
      _right->minus (prop_map, int_map, var_num, cl_num, findex, res);
      break;
    }
    case And:
    {
      switch (_left->oper ())
      {
        case Not:
          if (_left->_right->oper () == And || _left->_right->oper () == Or)
            f1index = ++var_num;
          else if (_left->_right->oper () == Not)
          {
            printf ("Error: two Not together in aalta_formula::TODAMICS-AND\n");
            exit (0);
          }
          else
          {
            f1index = search (*(_left->_right), int_map, var_num);
            prop_map[f1index] = _left->_right;
          }
          f1index = -f1index;
          break;
        case And:
        case Or:
          f1index = ++var_num;
          break;
        default:
          f1index = search (*_left, int_map, var_num);
          prop_map[f1index] = _left;
          break;
      }
      
      switch (_right->oper ())
      {
        case Not:
          if (_right->_right->oper () == And || _right->_right->oper () == Or)
            f2index = ++var_num;
          else if (_right->_right->oper () == Not)
          {
            printf ("Error: two Not together in aalta_formula::TODIMACS-AND\n");
            exit (0);
          }
          else
          {
            f2index = search (*(_right->_right), int_map, var_num);
            prop_map[f2index] = _right->_right;
          }

          f2index = -f2index;
          break;
        case And:
        case Or:
          f2index = ++var_num;
          break;
        default:
          f2index = search (*_right, int_map, var_num);
          prop_map[f2index] = _right;
          break;
      }
      
      //result = convert_to_string(-findex) + " " + convert_to_string(f1index) + " 0\n" 
      //         + convert_to_string(-findex) + " " + convert_to_string(f2index) + " 0\n";
      int ints[] = {-findex, f1index};
      std::vector<int> lits (ints, ints + sizeof (ints)/sizeof (int));
      //lits.push_back (-findex);
      //lits.push_back (f1index);
      res.push_back (lits);
      int ints2[] = {-findex, f2index};
      std::vector<int> lits2 (ints2, ints2 + sizeof (ints2)/sizeof (int));
      //lits2.push_back (-findex);
      //lits2.push_back (f2index);
      res.push_back (lits2);
      
      cl_num += 2;
      //result += _left->plus (prop_map, int_map, var_num, cl_num, f1index);
      //result += _right->plus (prop_map, int_map, var_num, cl_num, f2index);
      _left->plus (prop_map, int_map, var_num, cl_num, f1index, res);
      _right->plus (prop_map, int_map, var_num, cl_num, f2index, res);
      break;
    }  
    case Or:
    {
      switch (_left->oper ())
      {
        case Not:
          if (_left->_right->oper () == And || _left->_right->oper () == Or)
            f1index = ++var_num;
          else if (_left->_right->oper () == Not)
          {
            printf ("Error: two Not together in aalta_formula::TODAMICS-OR\n");
            exit (0);
          }
          else
          {
            f1index = search (*(_left->_right), int_map, var_num);
            prop_map[f1index] = _left->_right;
          }
          f1index = -f1index;
          break;
        case And:
        case Or:
          f1index = ++var_num;
          break;
        default:
          f1index = search (*_left, int_map, var_num);
          prop_map[f1index] = _left;
          break;
      }
      
      switch (_right->oper ())
      {
        case Not:
          if (_right->_right->oper () == And || _right->_right->oper () == Or)
            f2index = ++var_num;
          else if (_right->_right->oper () == Not)
          {
            printf ("Error: two Not together in aalta_formula::TODAMICS-OR\n");
            exit (0);
          }
          else
          {
            f2index = search (*(_right->_right), int_map, var_num);
            prop_map[f2index] = _right->_right;
          }

          f2index = -f2index;
          break;
        case And:
        case Or:
          f2index = ++var_num;
          break;
        default:
          f2index = search (*_right, int_map, var_num);
          prop_map[f2index] = _right;
          break;
      }
      
      //result = convert_to_string(-findex) + " " + convert_to_string(f1index) + " "
      //         + convert_to_string(f2index) + " 0\n" ;
      int ints[] = {-findex, f1index, f2index};
      std::vector<int> lits (ints, ints + sizeof (ints)/sizeof (int));
      //lits.push_back (-findex);
      //lits.push_back (f1index);
      //lits.push_back (f2index);
      res.push_back (lits);
      
      cl_num += 1;
      //result += _left->plus (prop_map, int_map, var_num, cl_num, f1index);
      //result += _right->plus (prop_map, int_map, var_num, cl_num, f2index);
      _left->plus (prop_map, int_map, var_num, cl_num, f1index, res);
      _right->plus (prop_map, int_map, var_num, cl_num, f2index, res);
      break;
    }
    default:
      break;
      
  }
  //return result;
}

//std::string 
void 
aalta_formula::minus (int_prt_map& prop_map, af_int_map& int_map, int& var_num, int& cl_num, int cur, std::vector<std::vector<int> >& res)
{
  int findex, f1index, f2index;
  findex = cur;
  std::string result = "";
  switch (oper ())
  {
    case Not:
      //result = _right->plus (prop_map, int_map, var_num, cl_num, findex);
      _right->plus (prop_map, int_map, var_num, cl_num, findex, res);
      break;
    case And:
    {
      switch (_left->oper ())
      {
        case Not:
          if (_left->_right->oper () == And || _left->_right->oper () == Or)
            f1index = ++var_num;
          else if (_left->_right->oper () == Not)
          {
            printf ("Error: two Not together in aalta_formula::TODAMICS-AND\n");
            exit (0);
          }
          else
          {
            f1index = search (*(_left->_right), int_map, var_num);
            prop_map[f1index] = _left->_right;
          }
          //f1index = -f1index;
          break;
        case And:
        case Or:
          f1index = ++var_num;
          f1index = -f1index;
          break;
        default:
          f1index = search (*_left, int_map, var_num);
          prop_map[f1index] = _left;
          f1index = -f1index;
          break;
      }
      
      switch (_right->oper ())
      {
        case Not:
          if (_right->_right->oper () == And || _right->_right->oper () == Or)
            f2index = ++var_num;
          else if (_right->_right->oper () == Not)
          {
            printf ("Error: two Not together in aalta_formula::TODIMACS-AND\n");
            exit (0);
          }
          else
          {
            f2index = search (*(_right->_right), int_map, var_num);
            prop_map[f2index] = _right->_right;
          }

          //f2index = -f2index;
          break;
        case And:
        case Or:
          f2index = ++var_num;
          f2index = -f2index;
          break;
        default:
          f2index = search (*_right, int_map, var_num);
          prop_map[f2index] = _right;
          f2index = -f2index;
          break;
      }
      
      //result = convert_to_string(-findex) + " " + convert_to_string(f1index) + " "
      //         + convert_to_string(f2index) + " 0\n" ;
      int ints[] = {-findex, f1index, f2index};
      std::vector<int> lits (ints, ints + sizeof (ints)/sizeof (int));
      //lits.push_back (-findex);
      //lits.push_back (f1index);
      //lits.push_back (f2index);
      res.push_back (lits);
      
      cl_num += 1;
      //result += _left->minus (prop_map, int_map, var_num, cl_num, f1index);
      //result += _right->minus (prop_map, int_map, var_num, cl_num, f2index);
      _left->minus (prop_map, int_map, var_num, cl_num, f1index, res);
      _right->minus (prop_map, int_map, var_num, cl_num, f2index, res);
      break;
    }  
    case Or:
    {
      switch (_left->oper ())
      {
        case Not:
          if (_left->_right->oper () == And || _left->_right->oper () == Or)
            f1index = ++var_num;
          else if (_left->_right->oper () == Not)
          {
            printf ("Error: two Not together in aalta_formula::TODAMICS-OR\n");
            exit (0);
          }
          else
          {
            f1index = search (*(_left->_right), int_map, var_num);
            prop_map[f1index] = _left->_right;
          }
          //f1index = -f1index;
          break;
        case And:
        case Or:
          f1index = ++var_num;
          f1index = -f1index;
          break;
        default:
          f1index = search (*_left, int_map, var_num);
          prop_map[f1index] = _left;
          f1index = -f1index;
          break;
      }
      
      switch (_right->oper ())
      {
        case Not:
          if (_right->_right->oper () == And || _right->_right->oper () == Or)
            f2index = ++var_num;
          else if (_right->_right->oper () == Not)
          {
            printf ("Error: two Not together in aalta_formula::TODAMICS-OR\n");
            exit (0);
          }
          else
          {
            f2index = search (*(_right->_right), int_map, var_num);
            prop_map[f2index] = _right->_right;
          }

          //f2index = -f2index;
          break;
        case And:
        case Or:
          f2index = ++var_num;
          f2index = -f2index;
          break;
        default:
          f2index = search (*_right, int_map, var_num);
          prop_map[f2index] = _right;
          f2index = -f2index;
          break;
      }
      
      //result = convert_to_string(-findex) + " " + convert_to_string(-f1index) + " "
      //         + convert_to_string(f2index) + " 0\n" ;
      //result += convert_to_string(-findex) + " " + convert_to_string(f1index) + " "
      //         + convert_to_string(-f2index) + " 0\n" ;
      //result += convert_to_string(-findex) + " " + convert_to_string(f1index) + " "
      //         + convert_to_string(f2index) + " 0\n" ;
      int ints[] = {-findex, -f1index, f2index};
      std::vector<int> lits (ints, ints + sizeof (ints)/sizeof (int));
      //lits.push_back (-findex);
      //lits.push_back (-f1index);
      //lits.push_back (f2index);
      res.push_back (lits);
      int ints2[] = {-findex, f1index, -f2index};
      std::vector<int> lits2 (ints2, ints2 + sizeof (ints2)/sizeof (int));
      //lits2.push_back (-findex);
      //lits2.push_back (f1index);
      //lits2.push_back (-f2index);
      res.push_back (lits2);
      int ints3[] = {-findex, f1index, f2index};
      std::vector<int> lits3 (ints3, ints3 + sizeof(ints3)/sizeof(int));
      //lits3.push_back (-findex);
      //lits3.push_back (f1index);
      //lits3.push_back (f2index);
      res.push_back (lits3);
             
      cl_num += 3;
      //result += _left->minus (prop_map, int_map, var_num, cl_num, f1index);
      //result += _right->minus (prop_map, int_map, var_num, cl_num, f2index);
      _left->minus (prop_map, int_map, var_num, cl_num, f1index, res);
      _right->minus (prop_map, int_map, var_num, cl_num, f2index, res);
      break;
    }
    default:
      break;
      
  }
  //return result;
}

aalta_formula::af_prt_set 
aalta_formula::get_prop(std::string dimacs, int_prt_map prop_map)
{
  af_prt_set result;
  std::string line;
  int val;
  
  aalta_formula* f;
  std::vector<std::string> strs;
  ifstream fileout;
  fileout.open(dimacs.c_str());
  if(fileout.is_open())
  {
    std::getline(fileout, line);
    if(line.find("UNSAT") != std::string::npos)
    {
      return result;
    }
    else
    {
      if(line.find("SAT") != std::string::npos)
      {
        std::getline(fileout, line);
        strs = split_str(line);
        int size = strs.size();
        for(int i = 0; i < size; i ++)
        {
          if(strs[i].at(0) == '0')
          {
            continue;
          }
          if(strs[i].at(0) != '-')
          {
            val = atoi(strs[i].c_str());
            f = prop_map[val];
            if(f->oper() > Undefined)
              result.insert(f);
          }
          else
          {
            val = atoi(strs[i].substr(1).c_str());
            f = prop_map[val];
            if(f->oper() > Undefined)
            {
              f = aalta_formula(Not, NULL, f).unique();
              result.insert(f);
            }
          }
        }
      }
      else
      {
        cout << "minisat computing error!" << endl;
        exit(0);
      }
    }
  }
  else
  {
    cout << "Unable to open file " << dimacs << endl;
    exit(0);
  }
  return result;
}

bool 
aalta_formula::find (aalta_formula *f)
{
  switch (oper ())
  {
    case And:
    case Or:
      return _left->find (f) || _right->find (f);
    default:
      if (this == f)
        return true;
      else
        return false;
  }
  return false;
}

aalta_formula::af_prt_set aalta_formula::to_or_set ()
{
	af_prt_set res;
	if (oper () != Or)
		res.insert (this);
	else
	{
		af_prt_set tmp = _left->to_or_set ();
		res.insert (tmp.begin (), tmp.end ());
		tmp = _right->to_or_set ();
		res.insert (tmp.begin (), tmp.end ());
	}
	return res;
}

void 
aalta_formula::to_set (af_prt_set & result)
{
  if(oper() != And)
  {
    result.insert(this);
  }
  else
  {
    _left->to_set(result);
    _right->to_set(result);
  }
}

aalta_formula::af_prt_set 
aalta_formula::to_set()
{
  af_prt_set result;
  to_set (result);
  return result;
  /*
  af_prt_set result, result1, result2;
  if(oper() != And)
  {
    result.insert(this);
  }
  else
  {
    result1 = _left->to_set();
    result2 = _right->to_set();
    result.insert(result1.begin(), result1.end());
    result.insert(result2.begin(), result2.end());
    result1.clear();
    result2.clear();
  }
  return result;
  */
}

hash_set<aalta_formula*> 
aalta_formula::and_to_set()
{
  hash_set<aalta_formula*> result, result1, result2;
  if(oper() != And)
  {
    result.insert(this);
  }
  else
  {
    result1 = _left->and_to_set();
    result2 = _right->and_to_set();
    result.insert(result1.begin(), result1.end());
    result.insert(result2.begin(), result2.end());
    result1.clear();
    result2.clear();
  }
  return result;
}

aalta_formula::af_prt_set 
aalta_formula::get_alphabet()
{
  af_prt_set result, P;
  std::string str = "";
  switch (oper ())
  {
    case True:
    case False:
      break;
    case Not:
    case Next:
      result = _right->get_alphabet ();
      break;
    case And:
    case Or:
    case Until:
    case Release:
      P = _left->get_alphabet ();
      result.insert (P.begin (), P.end ());
      P = _right->get_alphabet ();
      result.insert (P.begin (), P.end ());
      break;
    default:
      str = get_name (oper ());
      if (str.find ("FOR_UNTIL_") == std::string::npos)
        result.insert (this);
  }
  return result;
  /*
  int size = names.size();
  if(size < Undefined + 2)
  { 
    cout << "error! Cannot find atoms!" << endl;
    exit(0); 
  }
  for(int i = Undefined + 1; i < size; i++)
  {
    if (names[i].find ("FOR_UNTIL_") == std::string::npos)
    {
      f = aalta_formula(names[i].c_str()).unique();
      result.insert(f);
    }
  }
  return result;
  */
}

bool 
aalta_formula::find_prop_atom (aalta_formula *f)
{
  if (oper () == And || oper () == Or)
    return _left->find_prop_atom (f) || _right->find_prop_atom (f);
  else 
    return (this == f);
}

void 
aalta_formula::complete (af_prt_set& P)
{
  std::vector<aalta_formula*> vec;
  af_prt_set::iterator it;
  /*
  P.erase (TRUE ());
  std::string str = "";
  for (it = P.begin (); it != P.end (); )
  {
    if ((*it)->_right != NULL)
    {
      str = get_name ((*it)->_right->oper ());
      if (str.find ("FOR_UNTIL_") != std::string::npos)
      {
        P.erase (*it);
        it = P.begin ();
      }
      else
        it ++;
    }
    else
    {
      str = get_name ((*it)->oper ());
      if (str.find ("FOR_UNTIL_") != std::string::npos)
      {
        P.erase (*it);
        it = P.begin ();
      }
      else
        it ++;
    }
  }
  */
  af_prt_set alp = get_alphabet ();
  for (it = P.begin (); it != P.end (); it ++)
  {
    if ((*it)->_right != NULL)
    {
      if (alp.find ((*it)->_right) != alp.end ())
        alp.erase ((*it)->_right);
    }
    else
    {
      if (alp.find (*it) != alp.end ())
        alp.erase (*it);
    }
  }
  P.erase (TRUE ());
  P.insert (alp.begin (), alp.end ());
}

std::string 
aalta_formula::ltlf2ltl()
{
  
  
  std::string result = ltlf2ltlTranslate();
  result += " & (Tail & (Tail U G ! Tail))";
  return result;
}

std::string 
aalta_formula::ltlf2ltlTranslate()
{
  /*This translation is from "Linear Temporal Logic and Linear 
   *Dynamic Logic on Finite Traces", Giuseppe De Giacomo and Moshe Y. Vardi, 
   *IJCAI 2013
   */
  std::string result = "";
  switch(oper())
  {
    case And:
      result = std::string("(") + _left->ltlf2ltlTranslate() + " & " 
               + _right->ltlf2ltlTranslate() + ")";
      break;
    case Or:
      result = std::string("(") + _left->ltlf2ltlTranslate() + " | " 
               + _right->ltlf2ltlTranslate() + ")";
      break;
    case Next:
      result = std::string("(X (Tail & ")  + _right->ltlf2ltlTranslate() + "))";
      break;
    case Until:
      result = std::string("(") + _left->to_string() + " U " + 
               "(Tail & " + _right->ltlf2ltlTranslate() + "))";
      break;
    case Release:
      result = std::string("(") + _left->to_string() + " R " + 
               "(! Tail | " + _right->ltlf2ltlTranslate() + "))";
      break;
    case Undefined:
      result = "";
    default:
      result = to_string();
      break;
  }
  return result;
}

/*
 * The followings are added by Jianwen Li on October 8, 2014
 * For LTL co-safety to dfw
 *
 */
bool 
aalta_formula::is_cosafety ()
{
  switch(_op)
  {
    case Release:
      return false;
    case Next:
      return _right->is_cosafety ();
    case Until:
    case And:
    case Or:
      return _left->is_cosafety () && _right->is_cosafety ();
    case Not:
    default:
      return true;
  }
  return false;
}

std::string 
aalta_formula::cosafety2smv ()
{
  hash_map<aalta_formula*, int> f_ids;
  string result = "MODULE main\n";
  ///////////////for VAR
  result += "VAR\n";
  string var = "v";
  int i = 0;
  afp_set::iterator it;
  afp_set atoms;
  for(it = all_afs.begin(); it != all_afs.end(); it ++)
  {
    switch((*it)->_op)
    {
      case Next:
      case Until:
      case And:
      case Or:
        i ++;
        result += "  " + var + convert_to_string(i) + " : boolean;\n"; 
        f_ids.insert(std::pair<aalta_formula*, int> (*it, i));
        break;
      case Not:
        atoms.insert ((*it)->_right);
        break;
      default:
        atoms.insert (*it);
        break;
    }
  }
  for(it = atoms.begin(); it != atoms.end(); it ++)
  {
    result += "  " + (*it)->to_string() + " : boolean;\n";
  }
  //printf ("\n");
  /////////////////for VAR
  
  /////////////////for INIT
  result += "INIT\n";
  result += "  TRUE;\n"; 
  /////////////////for INIT
  
  /////////////////for TRANS
  result += "TRANS\n";
  string l, r, cur;
  hash_map<aalta_formula*, int>::iterator it_map;
  for(it = all_afs.begin(); it != all_afs.end(); it ++)
  {
    switch((*it)->_op)
    {
      case Next:
        it_map = f_ids.find(*it);
        cur = var + convert_to_string(it_map->second);
        it_map = f_ids.find((*it)->_right);
        if(it_map == f_ids.end())
          r = (*it)->_right->to_string();
        else
          r = var + convert_to_string(it_map->second);
        result += "  next (" + cur + ")" + " = " + r + ";\n";  
        break;
      case Until:
        it_map = f_ids.find(*it);
        cur = var + convert_to_string(it_map->second);
        it_map = f_ids.find((*it)->_right);
        if(it_map == f_ids.end())
          r = (*it)->_right->to_string();
        else
          r = var + convert_to_string(it_map->second);
        it_map = f_ids.find((*it)->_left);
        if(it_map == f_ids.end())
          l = (*it)->_left->to_string();
        else
          l = var + convert_to_string(it_map->second);
        result += "  next(" + cur + ") = " + "next(" + r + ") | (next(" + l + ") & " + cur + ");\n";
        break;
      case And:
        it_map = f_ids.find(*it);
        cur = var + convert_to_string(it_map->second);
        it_map = f_ids.find((*it)->_right);
        if(it_map == f_ids.end())
          r = (*it)->_right->to_string();
        else
          r = var + convert_to_string(it_map->second);
        it_map = f_ids.find((*it)->_left);
        if(it_map == f_ids.end())
          l = (*it)->_left->to_string();
        else
          l = var + convert_to_string(it_map->second);
        result += "  " + cur + "=  " + l + " & " + r + ";\n";
        break;
      case Or:
        it_map = f_ids.find(*it);
        cur = var + convert_to_string(it_map->second);
        it_map = f_ids.find((*it)->_right);
        if(it_map == f_ids.end())
          r = (*it)->_right->to_string();
        else
          r = var + convert_to_string(it_map->second);
        it_map = f_ids.find((*it)->_left);
        if(it_map == f_ids.end())
          l = (*it)->_left->to_string();
        else
          l = var + convert_to_string(it_map->second);
        result += "  " + cur + "=  " + l + " | " + r + ";\n";
        break;
      default:
        break;
    }
  }
  /////////////////for TRANS
  
  
  /////////////////for comments
  result += "\n\n---------------comments\n";
  for(it_map = f_ids.begin(); it_map != f_ids.end(); it_map ++)
  {
    result += var + convert_to_string(it_map->second) + " : " + it_map->first->to_string () + "\n";
  }
  /////////////////for comments
  
  
  return result;
}

}
//end of Jianwen Li












