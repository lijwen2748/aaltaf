/*
 * added by Jianwen LI on December 20th, 2014
 * translate ltlf formulas to smv spec
*/

#include "ltl2smvspec.h"
#include <stdlib.h>
#include <stdio.h>
#include <set>
using namespace std;
usning namespace aalta;
#define MAXN 1000000


ltl_formula* ltlf2ltl (ltl_formula *f)
{
  if (f == NULL)
    return NULL;
  ltl_formula *res, *l, *r, *t;
  if (f->_var != NULL)
    res = create_var (f->_var);
  else if (f->_type == eTRUE)
    res = create_operation (eTRUE, NULL, NULL);
  else if (f->_type == eFALSE)
    res = create_operation (eFALSE, NULL, NULL);
  else
  {
  switch (f->_type)
  {
    case eNOT:
      r = ltlf2ltl (f->_right);
      res = create_operation (eNOT, NULL, r);
      break;
    case eNEXT:
      r = ltlf2ltl (f->_right);
      t = create_var ("Tail");
      t = create_operation (eNOT, NULL, t);
      r = create_operation (eAND, t, r);
      res = create_operation (eNEXT, NULL, r);
      break;
    
    case eFUTURE:
      r = ltlf2ltl (f->_right);
      t = create_var ("Tail");
      t = create_operation (eNOT, NULL, t);
      r = create_operation (eAND, t, r);
      res = create_operation (eFUTURE, NULL, r);
      break;
    case eUNTIL:
      l = ltlf2ltl (f->_left);
      r = ltlf2ltl (f->_right);
      t = create_var ("Tail");
      t = create_operation (eNOT, NULL, t);
      r = create_operation (eAND, t, r);
      res = create_operation (eUNTIL, l, r);
      break;
    case eGLOBALLY:
    
      r = ltlf2ltl (f->_right);
      t = create_var ("Tail");
      //t = create_operation (eNOT, NULL, t);
      r = create_operation (eOR, t, r);
      res = create_operation (eGLOBALLY, NULL, r);
      break;     
    case eRELEASE:
    
      l = ltlf2ltl (f->_left);
      r = ltlf2ltl (f->_right);
      t = create_var ("Tail");
      //t = create_operation (eNOT, NULL, t);
      r = create_operation (eOR, t, r);
      res = create_operation (eRELEASE, l, r);
      break;
    case eAND:
      l = ltlf2ltl (f->_left);
      r = ltlf2ltl (f->_right);
      res = create_operation (eAND, l, r);
      break;
    case eOR:
      l = ltlf2ltl (f->_left);
      r = ltlf2ltl (f->_right);
      res = create_operation (eOR, l, r);
      break;
    case eIMPLIES:
      l = ltlf2ltl (f->_left);
      r = ltlf2ltl (f->_right);
      res = create_operation (eIMPLIES, l, r);
      break;
    case eEQUIV:
      l = ltlf2ltl (f->_left);
      r = ltlf2ltl (f->_right);
      res = create_operation (eEQUIV, l, r);
      break;
    default:
          fprintf (stderr, "Error formula!");
          exit (0);
  }
  }
  return res;
}


std::string ltl2smvspec (ltl_formula *root)
{
  std::set<std::string> S = get_alphabet (root);
  if (S.find ("Tail") == S.end ())
    S.insert ("Tail");
  std::string res = "MODULE main\n";
  res += "VAR\n";
  for (std::set<std::string>::iterator it = S.begin (); it != S.end (); it ++)
    res += (*it) + " : boolean;\n";
  res += "\nLTLSPEC\n";
  //res += "! (" + to_string (root) + ")\n";
  res += "! (" + to_string (root) + " & ((! Tail) & ((! Tail) U G Tail))" + ")\n";
  return res;
}


char in[MAXN];

int main (int argc, char ** argv)
{
  if (argc == 1)
    {
      puts ("please input the formula:");
      if (fgets (in, MAXN, stdin) == NULL)
      {
        printf ("Error: read input!\n");
        exit (0);
      }
    }
  else
    {
      strcpy (in, argv[1]);
    }
    
    ltl_formula *root = getAST (in);
    ltl_formula *f = ltlf2ltl (root);
    std::string res = ltl2smvspec (f);
    printf ("%s", res.c_str ());
    destroy_formula (f);
    destroy_formula (root);
}







