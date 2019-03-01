/* 
 * File:   olg_item.cpp
 * Author: yaoyinbo
 * 
 * Created on October 26, 2013, 1:42 PM
 */


#include "olg_item.h"
#include "util/define.h"
#include "util/utility.h"
#include <iostream>
#include <assert.h>
#include <zlib.h>
#include "minisat/core/Dimacs.h"
#include "minisat/core/Solver.h"
#include "minisat/core/SolverTypes.h"
#include <unistd.h>
#include <sstream> 

using namespace std;
using namespace Minisat;

namespace aalta
{

olg_item::olg_item (aalta_formula::opkind op, int compos, olg_item *left, olg_item *right, olg_atom *atom)
: _op (op), _atom (atom), _left (left), _right (right), _compos (compos) { }

olg_item::olg_item (aalta_formula *af, bool flag)  //for propositional formula only
{
  olg_item *item1, *item2;
  switch (af -> oper ())
  {
    case aalta_formula::And:
      _left = new olg_item (af->l_af (), flag);
      _right = new olg_item (af->r_af (), flag);
      _op = aalta_formula::And;
      _atom = NULL;
     // _items.push_back (_left);
     // _items.push_back (_right);
      break;
    case aalta_formula::Or:
      _left = new olg_item (af->l_af (), flag);
      _right = new olg_item (af->r_af (), flag);
      _op = aalta_formula::Or;
      _atom = NULL;
      //_items.push_back (_left);
      //_items.push_back (_right);
      break;
    case aalta_formula::Not:
      _op = aalta_formula::Not;
      if(flag)
        _atom = new olg_atom (af->r_af ()->oper (), 0, olg_atom::More);
      else
        _atom = new olg_atom (af->r_af ()->oper (), -1, olg_atom::Inf);
      _atoms.push_back (_atom);
      break;
    default:
      _op = aalta_formula::Literal;
      if(flag)
        _atom = new olg_atom (af->oper (), 0, olg_atom::More);
      else
        _atom = new olg_atom (af->oper (), -1, olg_atom::Inf);
      _atoms.push_back (_atom);
      break;
  }
  _items.push_back (this);
}

/**
 * Unitl算子，将所有子公式的pos置为不确定
 */
void
olg_item::off_pos (int i)
{
  /*
  if (_compos == -1) return;
  if (_atom != NULL) _atom->_pos = -1;
  if (_left != NULL) _left->off_pos ();
  if (_right != NULL) _right->off_pos ();
  _compos = -1;
  */
  if (_atom != NULL) 
  {
  
    if(i == 0)
    {
      _atom->_freq = olg_atom::Once;
      if(_atom->_pos != -1)
      {  
        _atom->_isDisjunct = true;
        _atom->_orig_pos = _atom->_pos;
      } 
    }
    else
    {
      if(_atom->_isDisjunct)  //a U (..|..)
        _atom->_isDisjunct = false;
      _compos = -1;
      _atom->_orig_pos = -1;
    }
    _atom->_pos = -1; 
    
  }
  if (_left != NULL && (_left->_compos != -1 || _left->_op == aalta_formula::And)) _left->off_pos (i);
  if (_right != NULL && (_right->_compos != -1 || _right->_op == aalta_formula::And)) _right->off_pos (i);
}

/**
 * Next算子，将所有子公式pos位置加1
 */
void
olg_item::plus_pos ()
{
  if (_compos == -1 && _op != aalta_formula::And) return;
  if (_atom != NULL) 
  {
    if(_atom->_pos >= 0)
    {
      ++_atom->_pos; 
      _compos = _atom->_pos;
    }
    else
      _compos = -1;
  }
  if (_left != NULL) _left->plus_pos ();
  if (_right != NULL) _right->plus_pos ();
  if(_left != NULL && _right != NULL)
    _compos = (_left->_compos == _right->_compos && _left->_compos != -1) ? _left->_compos : -1;
  else if(_left != NULL)
    _compos = _left -> _compos;
  else if(_right != NULL)
    _compos = _right -> _compos;
  //if (_compos >= 0) ++_compos;
}

/**
 * Global算子
 */
void
olg_item::unonce_freq ()
{
  if (_atom != NULL && _atom->_freq == olg_atom::Once)
    _atom->_freq = _atom->_pos < 0 ? olg_atom::Inf : olg_atom::More;
  if (_left != NULL) _left->unonce_freq ();
  if (_right != NULL) _right->unonce_freq ();
}

/**
 * 判断频率是否都是多次的
 * @return 
 */
bool
olg_item::is_more () const
{
  if (_atom != NULL)
    {
      if (_atom->_freq == olg_atom::More)
        return true;
      else
        return false;
    }
  if (_left != NULL && !_left->is_more ())
    return false;
  if (_right != NULL && _right->is_more ())
    return true;
  return false;
}

/**
 * 获取去除信息后的公式字符串
 * @return 
 */
std::string
olg_item::get_olg_string () const
{
  switch (_op)
    {
    case aalta_formula::Literal:
      return _atom->to_string ();
    case aalta_formula::False:
      return FALSE_STRING;
    case aalta_formula::True:
      return TRUE_STRING;
    case aalta_formula::And:
      return "(" + _left->get_olg_string () + " & " + _right->get_olg_string () + ")";
    case aalta_formula::Or:
      return "(" + _left->get_olg_string () + " | " + _right->get_olg_string () + ")";
    case aalta_formula::Not:
      return "!" + _atom->to_string ();
    default:
      print_error ("[error type]olg convert error");
    }
  return "Error";
}

/**
 * 获取位置信息小于等于pos的olg公式
 * @param pos
 * @return 
 */
std::string
olg_item::get_olg_more_string (int pos) const
{
  std::string l_str, r_str;
  switch (_op)
    {
    case aalta_formula::And:
      l_str = _left->get_olg_more_string (pos);
      if (l_str == FALSE_STRING) return FALSE_STRING;
      r_str = _right->get_olg_more_string (pos);
      if (r_str == TRUE_STRING) std::swap (l_str, r_str);
      if (l_str == TRUE_STRING || r_str == FALSE_STRING)
        return r_str;
      return "(" + l_str + ") & (" + r_str + ")";
    case aalta_formula::Or:
      l_str = _left->get_olg_more_string (pos);
      if (l_str == TRUE_STRING)
        return TRUE_STRING;
      r_str = _right->get_olg_more_string (pos);
      if (r_str == FALSE_STRING) std::swap (l_str, r_str);
      if (l_str == FALSE_STRING || r_str == TRUE_STRING)
        return r_str;
      return "(" + l_str + ") | (" + r_str + ")";
    case aalta_formula::Literal:
      if ((_atom->_freq == olg_atom::More && (pos == INT_MAX || (_atom->_pos >= 0 && _atom->_pos <= pos)))
          || (_atom->_freq == olg_atom::Once && _atom->_pos == pos))
        return _atom->to_string ();
      else
        return TRUE_STRING;
    case aalta_formula::False:
      return FALSE_STRING;
    case aalta_formula::True:
      return TRUE_STRING;
    case aalta_formula::Not:
      if ((_atom->_freq == olg_atom::More && (pos == INT_MAX || (_atom->_pos >= 0 && _atom->_pos <= pos)))
          || (_atom->_freq == olg_atom::Once && _atom->_pos == pos))
        return "! " + _atom->to_string ();
      else
        return TRUE_STRING;
    default:
      print_error ("olg convert error --- error type");
    }
  return "";
}

std::string
olg_item::get_id_string (int id) const
{
  std::string l_str, r_str;
  switch (_op)
    {
    case aalta_formula::And:
      l_str = _left->get_id_string (id);
      if (l_str == FALSE_STRING) return FALSE_STRING;
      r_str = _right->get_id_string (id);
      if (r_str == TRUE_STRING) std::swap (l_str, r_str);
      if (l_str == TRUE_STRING || r_str == FALSE_STRING)
        return r_str;
      return "(" + l_str + ") & (" + r_str + ")";
    case aalta_formula::Or:
      l_str = _left->get_id_string (id);
      if (l_str == TRUE_STRING) return TRUE_STRING;
      r_str = _right->get_id_string (id);
      if (r_str == FALSE_STRING) std::swap (l_str, r_str);
      if (l_str == FALSE_STRING || r_str == TRUE_STRING)
        return r_str;
      return "(" + l_str + ") | (" + r_str + ")";
    case aalta_formula::Literal:
      if ((_atom->_pos < 0 && _atom->_id == id) || (_atom->_pos == 0 && _atom->_freq == olg_atom::More))
        return _atom->to_string ();
      else
        return TRUE_STRING;
    case aalta_formula::False:
      return FALSE_STRING;
    case aalta_formula::True:
      return TRUE_STRING;
    case aalta_formula::Not:
      if ((_atom->_pos < 0 && _atom->_id == -id) || (_atom->_pos == 0 && _atom->_freq == olg_atom::More))
        return "! " + _atom->to_string ();
      else
        return TRUE_STRING;
    default:
      print_error ("olg convert error --- error type");
    }
  return "";
}

/**
 * 带位置和频率信息
 * @return 
 */
std::string
olg_item::to_olg_string () const
{
  switch (_op)
    {
    case aalta_formula::Literal:
      return _atom->to_olg_string ();
    case aalta_formula::False:
      return FALSE_STRING;
    case aalta_formula::True:
      return TRUE_STRING;
    case aalta_formula::And:
      return "(" + _left->to_olg_string () + " & " + _right->to_olg_string () + ")";
    case aalta_formula::Or:
      return "(" + _left->to_olg_string () + " | " + _right->to_olg_string () + ")";
    case aalta_formula::Not:
      return "!" + _atom->to_olg_string ();
    default:
      print_error ("[error type]olg convert error");
    }
  return "Error";
}

std::string
olg_item::to_string () const
{
  switch (_op)
    {
    case aalta_formula::Literal:
      return _atom->to_string ();
    case aalta_formula::False:
      return FALSE_STRING;
    case aalta_formula::True:
      return TRUE_STRING;
    case aalta_formula::And:
      return "(" + _left->to_string () + " & " + _right->to_string () + ")";
    case aalta_formula::Or:
      return "(" + _left->to_string () + " | " + _right->to_string () + ")";
    case aalta_formula::Not:
      return "!" + _atom->to_string ();
    default:
      print_error ("[error type]olg convert error");
    }
  return "Error";
}

//==============================================================================

olg_atom::olg_atom (int id, int pos, freqkind freq, bool disjunct, int orig_pos)
: _id (id), _pos (pos), _freq (freq), _isDisjunct (disjunct), _orig_pos (orig_pos) { }

/**
 * 带位置和频率信息
 * @return 
 */
std::string
olg_atom::to_olg_string () const
{
  std::string ret = "< " + this->to_string ();
  if (_pos < 0) ret += ", ⊥";
  else ret += ", " + convert_to_string (_pos);
  if (_freq == Once) ret += ", −";
  else if (_freq == More) ret += ", ≥";
  else ret += ", inf";
  ret += " >";
  return ret;
}

inline std::string
olg_atom::to_string () const
{
  return aalta_formula::get_name (_id);
}

/*added by Jianwen Li on April 29, 2014
   * creat the Dimacs format for Minisat 
   * the output is stored in the file "cnf.dimacsPID" where PID is the process ID
   * Invoked by toDimacs() of olg_formula
   */
int olg_item::_varNum = 0; //the var number in cnf
int olg_item::_clNum = 0;  // the clause number in cnf
std::vector<int> olg_item::_vars; //the variables in _root
hash_map<int, int> olg_item::_vMap;//the mapping between variable ids and encodings in Dimacs. 
std::vector<olg_item*> olg_item::_items;
std::vector<olg_atom*> olg_item::_atoms;

string
olg_item::toDimacs()
{ 
  pid_t pid = getpid ();
  ostringstream str;  
  str << pid;
  string filename = "cnf.dimacs";
  filename += str.str ();
  FILE *f = NULL;
  f = fopen(filename.c_str (), "w");
  _varNum = _vMap.size();
  _clNum = 0;
  
  if(f == NULL)
  {
    perror(filename.c_str ());
    exit(0);
  }
  switch(_op)
  {
    case aalta_formula::And:
    {
      toDimacsPlus(NULL); 
      _clNum ++;
      fprintf(f, "p cnf %d %d\n", _varNum, _clNum);
      fprintf(f, "%d 0\n", _id);
      
      toDimacsPlus(f);
      break;
    }
    case aalta_formula::Or:
    {
      toDimacsPlus(NULL);
      _clNum ++;
      fprintf(f, "p cnf %d %d\n", _varNum, _clNum);
      fprintf(f, "%d 0\n", _id);
      
      toDimacsPlus(f);
      break;
    }
    case aalta_formula::Not: //why Not here?
    {
      fprintf(f, "p cnf 1 1\n");
      fprintf(f, "%d 0\n", -_vMap[_atom->_id]);
      break;
    }
    case aalta_formula::Literal:
    {
      fprintf(f, "p cnf 1 1\n");
      fprintf(f, "%d 0\n", _vMap[_atom->_id]);
      break;
    }
    default:
      printf("To Dimacs error! Unrecognized operators...\n");
      exit(0);
  }
  
  fclose(f);
  return filename;
}

//invoked by toDimacs();
void 
olg_item::toDimacsPlus(FILE *f) 
{
  int lid, rid;
  if(_op == aalta_formula::And || _op == aalta_formula::Or)
  {
    if(_left->_op != aalta_formula::Not && _left->_op != aalta_formula::Literal)
    {
      lid = _left->_id;
    }
    else if(_left->_op == aalta_formula::Not)
    {
      lid = -_vMap[_left->_atom->_id];
    }
    else
    {
      lid = _vMap[_left->_atom->_id];
    }
    if(_right->_op != aalta_formula::Not && _right->_op != aalta_formula::Literal)
    {
      rid = _right->_id;
    }
    else if(_right->_op == aalta_formula::Not)
    {
      rid = -_vMap[_right->_atom->_id];
    }
    else
    {
      rid = _vMap[_right->_atom->_id];
    }
  }
  switch(_op)
  {
    case aalta_formula::And:
    {
      if(f != NULL)
      {
        fprintf(f, "%d %d 0\n%d %d 0\n", -_id, lid, -_id, rid);
      }
      else
      {
        _varNum ++;
        _clNum += 2;
      }
      _left->toDimacsPlus(f);
      _right->toDimacsPlus(f);
      
      break;
    }
    case aalta_formula::Or:
    {
      if(f != NULL)
      {
        fprintf(f, "%d %d %d 0\n", -_id, lid, rid);
      }
      else
      {
        _varNum ++;
        _clNum ++;
      }
      _left->toDimacsPlus(f);
      _right->toDimacsPlus(f);
      break;
    }
    case aalta_formula::Not: //why Not here?
    case aalta_formula::Literal:
    {
      break;
    }
    default:
      printf("To Dimacs error! Unrecognized operators...\n");
      exit(0);
  }
}

//set the _id for Dimacs construction
void 
olg_item::setId(int& max)
{
  if(_op == aalta_formula::Not || _op == aalta_formula::Literal)
  {
    max --;
    return;
  }
  _id = max;
  _left->setId(++max);  
  _right->setId(++max);
}

//get the variables in _root: set _vars and _vMap
void 
olg_item::getVars(int& i) 
{
  if(_op == aalta_formula::Not || _op == aalta_formula::Literal)
  {
    //cout << to_string() << endl;
    if(_vMap.find(_atom->_id) == _vMap.end())
    {
      _vMap[_atom->_id] = ++i;
      _vars.push_back(_atom->_id);
    }   
  }
  else
  {
    _left->getVars(i);
    _right->getVars(i);
  }
}

void 
olg_item::initial()
{
  _vars.clear();
  _vMap.clear();
  //_items.clear();
  _varNum = 0;
  _clNum = 0;
}

/**
 * SAT solver invoking
 * @param formula
 * @return 
 */
bool
olg_item::SATCall ()
{  
  if(_op == aalta_formula::True)
    return true;
  if(_op == aalta_formula::False)
    return false;
  int i = 0;
  initial();
  getVars(i);
  int max = _vMap.size() + 1;
  setId(max);
  string cnffile = toDimacs();
  gzFile in = gzopen(cnffile.c_str (), "rb");
  if (in == NULL)
  {
    printf("Minisat ERROR: Could not open input file %s!\n", cnffile.c_str ());
    exit(1);
  }
  Minisat::Solver S;
  Minisat::parse_DIMACS(in, S);
  gzclose(in);
  if (!S.simplify())
  {
    return false;
  }
  Minisat::vec<Minisat::Lit> dummy;
  Minisat::lbool ret;
  ret = S.solveLimited(dummy);
  
  if(ret == l_True)
  {
    hash_map<int, int> _map;
    hash_map<int, int>::iterator it;
    for(it = _vMap.begin(); it != _vMap.end(); it++)
    {
      _map[it->second] = it->first;
    }
    for(int i = 0; i < S.nVars(); i++)
    {
      if(_map.find(i+1) != _map.end())
      {
        if(S.model[i] == l_True)
          _evidence[_map[i+1]] = true;
        else
          _evidence[_map[i+1]] = false;
      }
    }
    return true;
  }
  if(ret == l_False)
    return false;
    
    
  printf("Minisat cannot check the formula!\n");
  
  exit(0);
}


hash_set<int> 
olg_item::get_pos(int n)
{
  hash_set<int> res, res2;
  bool val = false;
  
  if(_atom != NULL)
  {
    switch(n)
    {
    case 0: val = _atom->_pos != -1; break;
    case 1: val = _atom->_pos == -1; break;
    case 2: val = _atom->_freq == olg_atom::Inf; break;
    }
    if(val)
    {
      if(n == 0)
        res.insert(_atom->_pos);
      else
        res.insert(_atom->_id);
    }
  }
  else
  {
    res2 = _left->get_pos(n);
    res.insert(res2.begin(), res2.end());
    res2 = _right->get_pos(n);
    res.insert(res2.begin(), res2.end());
  }
  return res;
}


olg_item* 
olg_item::proj(int pos, int i)
{
  olg_item *item, *item1, *item2;
  bool val;
  
  if(_atom != NULL)
  {
    switch(pos)
    {
      case 0: val = _atom->_freq == olg_atom::More; break;
      case 1: val = _atom->_pos == i || (_atom->_pos >= 0 && _atom->_pos <= i && _atom->_freq == olg_atom::More); break;
      case 2: val = (_atom->_id == i && _atom->_pos == -1) || (_atom->_pos == 0 && _atom->_freq == olg_atom::More); break;
      case 3: val = (_atom->_id == i && _atom->_freq == olg_atom::Inf) || _atom->_freq == olg_atom::More; break;
    }
    if(val)
      return this;
    else
    {
      item = new olg_item(aalta_formula::True, 0);
      _items.push_back(item);
      return item;
    }
  }
  else
  {
    item1 = _left->proj(pos, i);
    item2 = _right->proj(pos, i);
    if(_op == aalta_formula::And)
    {
      if(item1->_op == aalta_formula::True)
        return item2;
      else if(item2->_op == aalta_formula::True)
        return item1;
      else
      {
        item = new olg_item(aalta_formula::And, 0, item1, item2);
        _items.push_back(item);
        return item;
      }
    }
    if(_op == aalta_formula::Or)
    {
      if(item1->_op == aalta_formula::True)
        return item1;
      else if(item2->_op == aalta_formula::True)
        return item2;
      else
      {
        item = new olg_item(aalta_formula::Or, 0, item1, item2);
        _items.push_back(item);
        return item;
      }
    }
  }
  printf("Computing projection error: Unrecoginized operator in olg_item\n");
  exit(0);
}

int 
olg_item::get_max_loc(int i, int id)
{
  int res = 0;
  int res1, res2;
  bool val = false;
  if(_op == aalta_formula::True)
    return -1;
  if(_atom != NULL)
  {
    switch(i)
    {
      case 0: val = id == _atom->_id && _atom->_pos == -1; break;
      case 1: val = id == _atom->_id && _atom->_freq == olg_atom::Inf; break;
    }
    if(val)
    {
      res ++; 
    }
  }
  else
  {
    res += _left->get_max_loc(i, id);
    res += _right->get_max_loc(i, id);
  }
  return res;
}

olg_item* 
olg_item::proj_loc(int i, int id, int& count)
{
  if(_op == aalta_formula::True)
    return this;
  olg_item *res, *res1, *res2;
  if(_atom != NULL)
  {
    if(_atom->_pos == 0 && _atom->_freq == olg_atom::More)
      return this;
    if(id == _atom->_id)
    { 
      if(i == count)
      {
        count ++;
        return this;
      }
      else
      {
        count ++; 
        res = new olg_item(aalta_formula::True, 0);
        _items.push_back(res);
        return res; 
      }
    }
    else
    {
      res = new olg_item(aalta_formula::True, 0);
      _items.push_back(res);
      return res;
    }
  }
  else
  {
    res1 = _left->proj_loc(i, id, count);
    res2 = _right->proj_loc(i, id, count);
    if(_op == aalta_formula::And)
    {
      if(res1->_op == aalta_formula::True)
        return res2;
      if(res2->_op == aalta_formula::True)
        return res1; 
      res = new olg_item(aalta_formula::And, 0, res1, res2);
      _items.push_back(res);
      return res;
    }
    if(_op == aalta_formula::Or)
    {
      if(res1->_op == aalta_formula::True)
        return res1;
      if(res2->_op == aalta_formula::True)
        return res2;
      res = new olg_item(aalta_formula::Or, 0, res1, res2);
      _items.push_back(res);
      return res;
    } 
  }
  printf("Computing projection Error!\n");
  exit(0);
  
}

bool 
olg_item::unsat()
{
  olg_item *olgAll, *olgIAll, *olgNonAll, *olgNonAll2, *olgInfAll, *olgInfAll2;
  int i, loc, count;
  
  hash_set<int>::iterator it;
  olgAll = proj(0, 0);
  //1. ofp(\phi)\downarrow S = false => \phi is false
  if(olgAll->_op != aalta_formula::True && !olgAll->SATCall())
  {
    //olg_item::destroy();
    return true;
  }
  
  //2. There exists i s.t. ofp(\phi)\downarrow i = false => \phi is false
  //3. There exists i s.t. ofp(\phi)\downarrow i /\ ofp(\phi)\downarrow S' = false => \phi is false
  //Here S'={l | l.start <= i and l.duration = >=}. If S' = {} then equal to 2.
  hash_set<int> ids = get_pos(0);
  //printf ("%s\n", to_olg_string().c_str ());
  for(it = ids.begin(); it != ids.end(); ++it)
  {
    olgIAll = proj(1, *it);
    if(olgIAll->_op != aalta_formula::True && !olgIAll->SATCall())
    {
      //olg_item::destroy();
      return true;
    }
  }
  
  //4. There exists l s.t. l.start = \nondeter and ofp(\phi)\downarrow {l}US' = false => \phi is false
  //Here S'={l | l.start = 0 and l.duration = >=}
  ids = get_pos(1);  //find such l, but do not distinguish its different copies
  for(it = ids.begin(); it != ids.end(); ++it)
  {
    olgNonAll = proj(2, *it); 
    loc = olgNonAll->get_max_loc(0, *it); //label different copies

    for(i = 0; i < loc; i++)
    {
      count = 0;
      olgNonAll2 = olgNonAll->proj_loc(i, *it, count);
      if(olgNonAll2->_op != aalta_formula::True && !olgNonAll2->SATCall())
      {
        //olg_item::destroy();
        return true;
      }
    } 
  }
  
  //5. There exists l s.t. l.duration = \inf and ofp(\phi)\downarrow {l}US = false => \phi is false.
  ids = get_pos(2);
  for(it = ids.begin(); it != ids.end(); it++)
  {
    olgInfAll = proj(3, *it);//find such l, but do not distinguish its different copies
    loc = olgInfAll->get_max_loc(1, *it); //label different copies

    for(i = 0; i < loc; i++)
    {
      count = 0;
      olgInfAll2 = olgInfAll->proj_loc(i, *it, count);
      if(olgInfAll2->_op != aalta_formula::True && !olgInfAll2->SATCall())
      {
        //olg_item::destroy();
        return true;
      }
    }
  }  
  //olg_item::destroy();
  return false;
}

bool 
olg_item::unsat2()
{
  olg_item *olgAll, *item, *item1, *item2;
  olg_atom *atom;
  olgAll = proj(0, 0);
  if(olgAll->_op != aalta_formula::True)
  {
    hash_set<int> atoms = olgAll->get_atoms();
    hash_set<int> atoms2, atoms3;
    item2 = this;
    hash_set<int>::iterator it, it2;
    for(it = atoms.begin(); it != atoms.end(); )
    {
      item = new olg_item(aalta_formula::Literal, 0);
      atom= new olg_atom(*it, 0, olg_atom::Once);
      item->_atom = atom;
      item1 = new olg_item(aalta_formula::And, 0, olgAll, item);
      _items.push_back(item);
      _items.push_back(item1);
      _atoms.push_back(item->_atom);
      if(!item1->SATCall())
      {
        item2 = item2->replace_inf_item(*it, true);
        item2->change_freq();
        if(item2->_op == aalta_formula::False)
          return true;
        //printf ("%s\n", item2->to_olg_string().c_str());
        if(item2->unsat())
        {
          //olg_item::destroy();
          return true;
        }
        olgAll = item2->proj(0,0);
        //printf ("%s\n", olgAll->to_string().c_str());
        if(olgAll->_op == aalta_formula::True)
          break;
        atoms3.insert(*it);
        atoms2 = olgAll->get_atoms ();
        atoms.insert(atoms2.begin(), atoms2.end ());
        for(it2 = atoms3.begin(); it2 != atoms3.end(); it2++)
          atoms.erase(*it2);
        
        it = atoms.begin();
      }
      else
      {
        item = new olg_item(aalta_formula::Not, 0);
        item->_atom = atom;
        item1 = new olg_item(aalta_formula::And, 0, olgAll, item);
        _items.push_back(item);
        _items.push_back(item1);
        if(!item1->SATCall())
        {
          item2 = item2->replace_inf_item(*it, false);
          item2->change_freq();
          if(item2->_op == aalta_formula::False)
            return true;
          if(item2->unsat())
          {
            //olg_item::destroy();
            return true;
          }
          olgAll = item2->proj(0,0);
          if(olgAll->_op == aalta_formula::True)
            break;
          atoms3.insert(*it);
          atoms2 = olgAll->get_atoms ();
          atoms.insert(atoms2.begin(), atoms2.end ());
          for(it2 = atoms3.begin(); it2 != atoms3.end(); it2++)
            atoms.erase(*it2);

          it = atoms.begin();
        }
        else
        {
          atoms3.insert (*it);
          it ++;
        }
      }
    }
  }
  //olg_item::destroy();
  return false;
}

void 
olg_item::change_freq()
{
  if(_op == aalta_formula::Or)
    return;
  if(_op == aalta_formula::And)
  {
    _left->change_freq();
    _right->change_freq();
  }
  else
  {
    if(_atom != NULL)
    {
      if(_atom->_freq == olg_atom::Inf && _atom->_isDisjunct)
      {
        _atom->_freq = olg_atom::More;
        _atom->_pos = _atom->_orig_pos;
      }
    }
  }
  return;
}

hash_set<int> 
olg_item::get_atoms()
{
  hash_set<int> res, res1;
  if(_op == aalta_formula::True || _op == aalta_formula::False)
    return res;
  if(_atom != NULL)
  {
    res.insert(_atom->_id);
  }
  else
  {
    res1 = _left->get_atoms();
    res.insert(res1.begin(), res1.end());
    res1 = _right->get_atoms();
    res.insert(res1.begin(), res1.end());
  }
  return res;
}

olg_item* 
olg_item::replace_inf_item(int id, bool flag)
{
  if(_op == aalta_formula::True || _op == aalta_formula::False)
    return this;
  if(_atom != NULL)
  {
    if(((flag && _op == aalta_formula::Literal) || ((!flag) && _op == aalta_formula::Not)) 
       && _atom->_id == id && _atom->_freq == olg_atom::Inf)
    {
      olg_item *item = new olg_item(aalta_formula::False, 0);
      _items.push_back(item);
      return item;
    }
    else
      return this;
  }
  else
  {
    olg_item *item, *item1, *item2;
    item1 = _left->replace_inf_item(id, flag);
    item2 = _right->replace_inf_item(id, flag);
    if(_op == aalta_formula::And)
    {
      if(item1->_op == aalta_formula::True)
        return item2;
      if(item2->_op == aalta_formula::True)
        return item1;
      if(item1->_op == aalta_formula::False)
        return item1;
      if(item2->_op == aalta_formula::False)
        return item2;
      item = new olg_item(aalta_formula::And, 0, item1, item2);
      _items.push_back(item);
      return item;
    }
    else if(_op == aalta_formula::Or)
    {
      if(item1->_op == aalta_formula::True)
        return item1;
      if(item2->_op == aalta_formula::True)
        return item2;
      if(item1->_op == aalta_formula::False)
        return item2;
      if(item2->_op == aalta_formula::False)
        return item1;
      item = new olg_item(aalta_formula::Or, 0, item1, item2);
      _items.push_back(item);
      return item;
    }
  }
  return this;
}

void 
olg_item::destroy()
{
  int len = _atoms.size();
  for(int i = 0; i < len; i++)
  {
    if(_atoms[i] != NULL)
    {
      //cout << _items[i]->to_string() << endl;
      delete _atoms[i];
      _atoms[i] = NULL;
    }
  }
  _atoms.clear();
  
  len = _items.size();
  for(int i = 0; i < len; i++)
  {
    if(_items[i] != NULL)
    {
      //cout << _items[i]->to_string() << endl;
      delete _items[i];
      _items[i] = NULL;
    }
  }
  _items.clear();
}

}


