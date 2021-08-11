/* 
 * File:   olg_formula.cpp
 * Author: yaoyinbo
 * 
 * Created on October 25, 2013, 10:38 PM
 */

#include "olg_formula.h"
#include "util/define.h"
#include "util/utility.h"
#include <stdio.h>
#include <map>
#include <algorithm>
#include <zlib.h>
#include <iostream>

using namespace std;

namespace aalta
{

olg_formula::olg_formula (aalta_formula *af)
{
  _id_arr = _pos_arr = NULL;
  _id_size = _pos_size = 0;

  this->classify (af);
  std::list<olg_item *>::iterator lit;
  _root = NULL;
  for (lit = _G.begin (); lit != _G.end (); lit++)
    {
      if (_root == NULL)
        _root = *lit;
      else
        {
          int cp = (*lit)->_compos == _root->_compos ? (*lit)->_compos : 0;
          _root = new olg_item (aalta_formula::And, cp, *lit, _root);
        }
    }
  for (lit = _NG.begin (); lit != _NG.end (); lit++)
    {
      if (_root == NULL)
        _root = *lit;
      else
        {
          int cp = (*lit)->_compos == _root->_compos ? (*lit)->_compos : 0;
          _root = new olg_item (aalta_formula::And, cp, *lit, _root);
        }
    }
    
}

olg_formula::~olg_formula ()
{
  this->destroy (_root);
  _root = NULL;
  
/*
  for (std::list<olg_item *>::iterator it = _GF.begin (); it != _GF.end (); it++)
    this->destroy (*it);
    */
    
}

/**
 * 判olg的可满足性
 * @return 
 */
bool
olg_formula::sat ()
{
  //cout << _root->to_string() << endl;
  if (_root->SATCall ()) 
  {
    _evidence = _root->_evidence;
    return true;
  }
  return false;
  
  /*
  
  //Should package the following codes as a block. To be done.
  if (_GF.empty ()) return false;
  if (! _R.empty ()) return false;
  if (! _GR.empty ()) return false;
  if (! _GU.empty ()) return false;
  if (! _GX.empty ()) return false;
  
  std::list<olg_item *> gp, nr;
  std::list<olg_item *>::const_iterator lit;
  olg_item *olg_base, *olg;
  int cp;
  bool ret = true;
  for (lit = _NR.begin (); lit != _NR.end (); lit++)
    {
      if (nr.empty ())
        nr.push_back (*lit);
      else
        {
          cp = (*lit)->_compos == nr.back ()->_compos ? (*lit)->_compos : 0;
          nr.push_back (new olg_item (aalta_formula::And, cp, *lit, nr.back ()));
        }
    }
  for (lit = _GP.begin (); lit != _GP.end (); lit++)
    {
      if (gp.empty ())
        gp.push_back (*lit);
      else
        {
          cp = (*lit)->_compos == gp.back ()->_compos ? (*lit)->_compos : 0;
          gp.push_back (new olg_item (aalta_formula::And, cp, *lit, gp.back ()));
        }
    }
  if(_NR.empty ())
  {
    if(_GP.empty ())
      olg_base = NULL;
    else
      olg_base = gp.back ();
  }
  else
  {
    if(_GP.empty ())
    {
      olg_base = NULL;
      if(!(nr.back ()->SATCall ())) 
      {
        ret = false;
      }
    }
    else
    {
      cp = (nr.back ()->_compos == gp.back ()->_compos) ? gp.back ()->_compos : 0;
      olg = new olg_item (aalta_formula::And, cp, nr.back (), gp.back ());
      if(! olg->SATCall ()) 
        ret = false;
      else
        olg_base = gp.back (); 
      delete olg;
    }
  }
  
  if (ret)
  {
  for (lit = _GF.begin (); lit != _GF.end () && ret; lit++)
    {
      if (olg_base == NULL)
      {
        ret &= (*lit)->SATCall(); //sat ((*lit)->get_olg_string ());
        if(!ret)
        {
          //ret = false;
          break;
        }
      }
      else
      {
        olg = new olg_item (aalta_formula::And, 0, *lit, olg_base);
        ret &= olg->SATCall(); //sat ((*lit)->get_olg_string () + ngf.back ()->get_olg_string ());
        delete olg;
        if(!ret)
        {
          //ret = false;
          break;
        }
      }
    }
  }
  if (!nr.empty ()) 
     nr.pop_front ();  //cannot delete the first element because it is not newed here!
  if (!gp.empty ()) 
     gp.pop_front ();  //cannot delete the first element because it is not newed here!
  for (lit = nr.begin (); lit != nr.end (); lit++)
  {
    delete (*lit);
    //*lit = NULL;
  }
  for (lit = gp.begin (); lit != gp.end (); lit++)
  {
    delete (*lit);
    //*lit = NULL;
  }
  return ret;
  */
  
}

/**
 * check the unsatisfiability of olg formulas
 * @return 
 * Modified by Jianwen Li on May 5th, 2014
 */

bool
olg_formula::unsat ()
{
  
  if(_root->_op == aalta_formula::False)
    return true;
  if(_root->_op == aalta_formula::True)
    return false;
    
  if(_root->unsat()) {olg_item::destroy(); return true;}
  return false;
  /*
  if(_root->unsat2()) {olg_item::destroy(); return true;}
  
  GX_loop ();
  olg_item *gx_imply = GX_imply ();
  if (gx_imply != NULL)
  {
    olg_item *root_renew = new olg_item (aalta_formula::And, 0, gx_imply, _root);
    //printf ("%s\n", root_renew->to_olg_string().c_str());
    //if(root_renew->unsat ()) { delete root_renew; olg_item::destroy(); return true;}
    if(root_renew->unsat2 ()) { delete root_renew; olg_item::destroy(); return true;}
    delete root_renew;
  }
  
  
  olg_item::destroy();
  return false; 
  */
  
}


/**
 * 对af子公式进行分类，
 * 以处理φi ∧ G (Ai | F Bi)类型的公式
 * @param af
 */
void
olg_formula::classify (aalta_formula *af)
{
  aalta_formula *and_next, *and_now, *or_next, *or_now;
  for (and_next = af; and_next != NULL; and_next = and_next->af_next (aalta_formula::And))
    { // 按and分离af公式
      and_now = and_next->af_now (aalta_formula::And);
      olg_item *item = build (and_now);
      if (!and_now->is_globally ())
        { // 非Globally，φi
          _NG.push_back (item);
          if(and_now -> release_free ())
            _NR.push_back (item);
          else
            _R.push_back (item);
          continue;
        }
      // G formulas
      _G.push_back (item);

      int F = 0;
      for (or_next = and_now->r_af (); or_next != NULL; or_next = or_next->af_next (aalta_formula::Or))
        { // 按or分离公式
          or_now = or_next->af_now (aalta_formula::Or);
          if (!or_now->release_free ())
            { // 含有release (including G)算子
              _GR.push_back (item);
              F = 1;
              break;
            }
         }
      if(F > 0) continue;
      for (or_next = and_now->r_af (); or_next != NULL; or_next = or_next->af_next (aalta_formula::Or))
        { // 按or分离公式
          or_now = or_next->af_now (aalta_formula::Or);
          if (or_now-> is_until ())
            { // 含有until(not F)算子
              _GU.push_back (item);
              F = 1;
              break;
            }
         }
      if(F > 0) continue;
      for (or_next = and_now->r_af (); or_next != NULL; or_next = or_next->af_next (aalta_formula::Or))
        { // 按or分离公式
          or_now = or_next->af_now (aalta_formula::Or);
          if (or_now->is_future ())
            { // Futrue
              //@ TODO: 可否不要再一次build？？，若不build，可不用在析构中destroy
              //if (!F) _GF.push_back (build (or_now->r_af ()));
              
              _GF.push_back(build (or_now->r_af()));  
              F = 1;
              break; 
            }
        }
      if(F > 0) continue;
      int pos, old_pos;
      old_pos = -2;
      for (or_next = and_now->r_af (); or_next != NULL; or_next = or_next->af_next (aalta_formula::Or))
        { // 按or分离公式
          or_now = or_next->af_now (aalta_formula::Or);
          pos = get_pos (or_now);
          if (old_pos == -2)
            old_pos = pos;
          else
          {
            if(pos != old_pos)
            {
              _GX.push_back (item);
              _FGX.push_back (and_now);
              F = 1;
              break;
            }
          }
          /*
          if (or_now-> is_next ())
            { // 含有next算子
              _GX.push_back (item);
              _FGX.push_back (and_now);
              F = 1;
              break;
            }
          */
        }
      
      if(F > 0) continue;
        _GP.push_back (item);
      /*
      if (F > 0) 
      {
        //_GF.push_back (item);
        _G.push_back (item);  //_G used in constructive function
      }
      else if(F <= 0)
        _NG.push_back (item);
      */
    }
}

/**
 * 通过aalta_formula构造olg结构
 * @param af
 * @return 
 */
olg_item *
olg_formula::build (const aalta_formula *af)
{
  _pos_size++;
  olg_item *root = NULL, *left, *right;
  int cp;
  switch (af->oper ())
    {
    case aalta_formula::And: // &
      left = build (af->l_af ());
      if (left->_op == aalta_formula::False)
        root = left;
      else
        {
          right = build (af->r_af ());
          if (right->_op == aalta_formula::True) std::swap (left, right);
          if (left->_op == aalta_formula::True || right->_op == aalta_formula::False)
            {
              root = right;
              destroy (left);
            }
          else
            {
              //cp = left->_compos == right->_compos ? left->_compos : 0;
              /*
              if(left->_compos == -2 || right->_compos == -2)
                cp = -2;
              else 
              */
              cp = (left->_compos == right->_compos && left->_compos != -1) ? left->_compos : -1;
              root = new olg_item (aalta_formula::And, cp, left, right);
              //if(cp == -1) root->off_pos (0);
            }
        }
      break;
    case aalta_formula::Or: // |
      left = build (af->l_af ());
      if (left->_op == aalta_formula::True)
        root = left;
      else
        {
          right = build (af->r_af ());
          if (right->_op == aalta_formula::False) std::swap (left, right);
          if (left->_op == aalta_formula::False || right->_op == aalta_formula::True)
            {
              root = right;
              destroy (left);
            }
          else
            {
              //cp = left->_compos == right->_compos ? left->_compos : 0;
              /*
              if(left->_compos == -2 || right->_compos == -2)
                cp = -1;
              else 
              */
              cp = (left->_compos == right->_compos && left->_compos != -1) ? left->_compos : -1;
              root = new olg_item (aalta_formula::Or, cp, left, right);
              if(cp == -1) root->off_pos (0);
            }
        }
      break;
    case aalta_formula::Next: // X
      root = build (af->r_af ());
      root->plus_pos ();
      break;
      
    case aalta_formula::Release: // R
      root = build (af->r_af ());
      if (af->l_af ()->oper () == aalta_formula::False) // G
        root->unonce_freq ();
      break;
    case aalta_formula::Until: // U
      root = build (af->r_af ());
      root->off_pos (1);
      break;
    case aalta_formula::True: // True
    case aalta_formula::False: // False
      root = new olg_item ((aalta_formula::opkind)af->oper (), 0);
      break;
    case aalta_formula::Not: // !
      root = new olg_item (aalta_formula::Not, 0);
      root->_atom = new olg_atom (af->r_af ()->oper (), 0, olg_atom::Once);
      break;
    default: // 原子
      root = new olg_item (aalta_formula::Literal, 0);
      root->_atom = new olg_atom (af->oper (), 0, olg_atom::Once);
      break;
    }
  return root;
}

/**
 * 收集位置和变量id信息
 * @param root
 */
void
olg_formula::col_info (olg_item *root)
{
  if (root == NULL)return;
  if (root->_atom != NULL)
    {
      if (root->_atom->_pos > 0)
        _pos_arr[_pos_size++] = root->_atom->_pos;
      else if (root->_op == aalta_formula::Not)
        _id_arr[_id_size++] = -root->_atom->_id;
      else
        _id_arr[_id_size++] = root->_atom->_id;
    }
  col_info (root->_left);
  col_info (root->_right);
}

/**
 * 销毁以root为根的olg结构
 * @param root
 */

void
olg_formula::destroy (olg_item *root)
{
  if (root == NULL) return;
  if (root->_atom != NULL)
  {
    delete root->_atom;
    root->_atom = NULL;
  }
  destroy (root->_left);
  if(root->_left) root->_left = NULL;
  destroy (root->_right);
  if(root->_right) root->_right = NULL;
  delete root;
}



/**
 * 带信息
 * @return 
 */
std::string
olg_formula::to_olg_string () const
{
  if (_root == NULL) return "";
  return _root->to_olg_string ();
}

std::string
olg_formula::to_string () const
{
  if (_root == NULL) return "";
  return _root->to_string ();
}

olg_item* 
olg_formula::GX_imply ()
{
  if(_GX_loop.empty()) return NULL;
  
  std::vector<aalta_formula*>::iterator it = _GX_loop.begin();
  olg_item *result = new olg_item (*it, false);  //0 for GF
  olg_item *item;
  //olg_item::_items.push_back(result);
  it ++;
  for(; it != _GX_loop.end(); it ++)
  {
    item = new olg_item (*it, false);
    result = new olg_item (aalta_formula::And, 0, result, item);
    //olg_item::_items.push_back (item);
    olg_item::_items.push_back (result);
  }
  
  item = G_be_implied ();
  if(item != NULL)
  {
    result = new olg_item (aalta_formula::And, 0, result, item);
    olg_item::_items.push_back (result);
  }
  return result;
}

olg_item* 
olg_formula::G_be_implied ()
{
  if(_GX_loop.empty()) return NULL;
  
  std::vector<aalta_formula*>::iterator it = _GX_loop.begin();
  hash_set<aalta_formula*> atoms1, atoms2;
  hash_set<aalta_formula*>::iterator it_f;
  atoms1 = (*it)->and_to_set ();
  it ++;
  for(; it != _GX_loop.end(); it ++)
  {
    atoms2 = (*it)->and_to_set ();
    for(it_f = atoms1.begin(); it_f != atoms1.end(); it_f++)
    {
      if(atoms2.find (*it_f) == atoms2.end())
      {
        atoms1.erase (it_f);
        it_f = atoms1.begin ();
      }
    }
    if(atoms1.empty ())
      return NULL;
  }
  
  aalta_formula *af = build_from_set (atoms1);
  olg_item *result = new olg_item (af, true);
  return result;
}

aalta_formula* 
olg_formula::build_from_set (hash_set<aalta_formula*> atoms)
{
  hash_set<aalta_formula*>::iterator it = atoms.begin();
  aalta_formula *result, *af;
  result = *it;
  it ++;
  for(; it != atoms.end (); it ++)
  {
    result = aalta_formula (aalta_formula::And, result, *it).unique ();
  }
  
  return result;
}

void 
olg_formula::GX_loop ()
{
  std::list<aalta_formula*>::iterator it;
  olg_item *item1, *item2, *item3, *item;
  std::pair<aalta_formula*, aalta_formula*> gx;
  item1 = _root->proj(1, 0);
  if (item1->_op == aalta_formula::True)
    return;
  int pos = 1;
  while (1)
  {
    if(pos > _FGX.size ()) break;
    for(it = _FGX.begin(); it != _FGX.end (); it ++)
    {
      gx = split_GX ((*it)->r_af ());
      if(gx.first == NULL) continue;
      if(gx.second == NULL) continue;
      item2 = new olg_item (gx.first, true);
      item = new olg_item (aalta_formula::And, 0, item1, item2);
      olg_item::_items.push_back (item);
      if(! item->SATCall ())
      {
        if(_GX_loop.size () < pos)
        {
          _GX_loop.push_back (gx.second);
          item3 = new olg_item (gx.second, true);
        }
        else
        {
          _GX_loop[pos-1] = aalta_formula (aalta_formula::And, _GX_loop[pos-1], gx.second).unique ();
          item3 = new olg_item (aalta_formula::And, 0, item3, new olg_item (gx.second, true));
          olg_item::_items.push_back (item3);
        }
        if (find_in_GX_loop (_GX_loop.back ())) break;  //_GX_loop cannot find last element
      }
    }
    if(_GX_loop.size () < pos || it != _FGX.end ())
      break;
    else
    {
      pos ++;
      item1 = item3; 
    }
  }
}

std::pair<aalta_formula*, aalta_formula*> 
olg_formula::split_GX (aalta_formula *af)
{
  aalta_formula *or_next = af;
  aalta_formula *or_now, *cur, *nex;
  std::vector<aalta_formula*> gx;
  cur = NULL;
  for (; or_next != NULL; or_next = or_next->af_next (aalta_formula::Or))
  { // 按or分离公式
    or_now = or_next->af_now (aalta_formula::Or);
    if (or_now-> is_next ())
    { // 含有next算子
      if(or_now->oper () != aalta_formula::Next)
        return std::make_pair((aalta_formula*)NULL, (aalta_formula*)NULL);
      gx.push_back (or_now->r_af ());
    }
    else
    {
      if(cur == NULL)
        cur = or_now;
      else
      {
        cur = aalta_formula(aalta_formula::Or, cur, or_now).unique ();
      }
    }
  }
  
  if(cur == NULL)
    return std::make_pair((aalta_formula*)NULL, (aalta_formula*)NULL);
  bool atom, next;
  while(1)
  {
    atom = false;
    next = false;
    for(int i = 0; i < gx.size (); i ++)
    {
      if(gx[i]->is_next ()) 
      {
        if(gx[i]->oper () == aalta_formula::Next && !atom)
        { 
          gx[i] = gx[i]->r_af ();
          next = true;
        }
        else
          return std::make_pair((aalta_formula*)NULL, (aalta_formula*)NULL);
      }
      else
      {
        atom = true;
      } 
    }
    if(atom)  break;
    if(next)  next = false;
  }
  
  nex = gx[0];
  for(int i = 1; i < gx.size(); i ++)
  {
    nex = aalta_formula(aalta_formula::Or, nex, gx[i]).unique ();
  }
  
  return std::make_pair (cur, nex);
}

bool 
olg_formula::find_in_GX_loop (aalta_formula *af) //cannot find the last element
{
  int size = _GX_loop.size ();
  for(int i =0; i < size-1; i ++)
  {
    if(af == _GX_loop[i])
      return true;
  }
  return false;
}


int 
olg_formula::get_pos (aalta_formula *af)
{
  int result, pos;
  switch (af->oper ())
  {
    case aalta_formula::Next:
      pos = get_pos (af->r_af ());
      if (pos >= 0)
        result = pos + 1;
      else
        result = -1;
      break;
    case aalta_formula::Until:
    case aalta_formula::Release:
      result = -1;
      break;
    case aalta_formula::And:
    case aalta_formula::Or:
      if(af->l_af ()->is_next () || af->r_af ()->is_next ()) //can be more gerneralized
        result = -1;
      else
        result = 0;
      break;
    case aalta_formula::Not:
    default:
      result = 0;
  }
  
  return result;
}

}





