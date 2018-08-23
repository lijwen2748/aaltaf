/*
 * added by Jianwen LI on December 20th, 2014
 * translate ltlf formulas to smv spec
*/

#include "ltlf2smv.h"
#include <stdlib.h>
#include <stdio.h>
#include <set>
#include <assert.h>

using namespace std;
using namespace aalta;
#define MAXN 1000000


std::string get_var (ltl_formula *root)
{
  if (root->_var != NULL) 
    return root->_var;
  if (root->_type == eTRUE)
    return "TRUE";
  if (root->_type == eFALSE)
    return "FALSE";
  std::string str = to_string (root);
  std::string var = "";
  std::map<std::string, std::string>::iterator it = VARS.find (str);
  if (it != VARS.end ())
  {
    return it->second;
  }
  else
  {
    var += "var" + string_of (id_num ++);
    VARS.insert (pair<std::string, std::string> (str, var));
    return var;
  }
    
}

std::string get_ltlspec (std::set<std::string> P)
{
  return "\nLTLSPEC\nG ! Tail\n";
}

/*

//bnf/fussy encoding (CGH)
std::string ltlf2tran (ltl_formula *root, std::set<std::string>& P)
{
  std::string res = "";
  std::string var, var2, var3;
  switch (root->_type)
  {
        case eNOT:
          var = get_var (root);
          var2 = get_var (root->_right);
          res += "(" + var + " = (! " + var2 + ")) & \n";
          P.insert (var);
          P.insert (var2);
          res += ltlf2tran (root->_right, P);
          break;
        case eNEXT:
          var = get_var (root);
          var2 = get_var (root->_right);
          res += "(" + var + " = ((! Tail) & next(" + var2 + "))) & \n";
          P.insert (var);
          P.insert (var2);
          res += ltlf2tran (root->_right, P);
          break; 
        case eFUTURE:
          var2 = get_var (root);
          var = get_var (root->_right);
          
          res += "(" + var2 + " = (" + var + " | (! Tail) & next(" + var2 + "))) & \n";
          P.insert (var);
          P.insert (var2);
          res += ltlf2tran (root->_right, P);
          break;
        case eUNTIL:
          var = get_var (root);
          var2 = get_var (root->_left);
          var3 = get_var (root->_right);
          res += "(" + var + " = (" + var3 + " | ((! Tail) & " + var2 + " & next(" + var + ")))) & \n";
          P.insert (var);
          P.insert (var2);
          P.insert (var3);
          res += ltlf2tran (root->_left, P);
          res += ltlf2tran (root->_right, P);
          break;
        case eOR:
          var = get_var (root);
          var2 = get_var (root->_left);
          var3 = get_var (root->_right);
          res += "(" + var + " = (" + var2 + " | " + var3 + ")) & \n";
          P.insert (var);
          P.insert (var2);
          P.insert (var3);
          res += ltlf2tran (root->_left, P);
          res += ltlf2tran (root->_right, P);
          break;
        case eGLOBALLY:
          var2 = get_var (root);
          var = get_var (root->_right);
          
          res += "(" + var2 + " = (" + var + " & ((! Tail) -> next(" + var2 + ")))) & \n";
          P.insert (var);
          P.insert (var2);
          res += ltlf2tran (root->_right, P);
          break;
        case eRELEASE:
          var = get_var (root);
          var2 = get_var (root->_left);
          var3 = get_var (root->_right);
          res += "(" + var + " = (" + var3 + " & (" + var2 + " | ((! Tail) -> next(" + var + "))))) & \n";
          P.insert (var);
          P.insert (var2);
          P.insert (var3);
          res += ltlf2tran (root->_left, P);
          res += ltlf2tran (root->_right, P);
          break;
          
        case eAND:
          var = get_var (root);
          var2 = get_var (root->_left);
          var3 = get_var (root->_right);
          res += "(" + var + " = (" + var2 + " & " + var3 + ")) & \n";
          P.insert (var);
          P.insert (var2);
          P.insert (var3);
          res += ltlf2tran (root->_left, P);
          res += ltlf2tran (root->_right, P);
          break;
        
        case eIMPLIES:
          var = get_var (root);
          var2 = get_var (root->_left);
          var3 = get_var (root->_right);
          res += "(" + var + " = (" + var2 + " -> " + var3 + ")) & \n";
          P.insert (var);
          P.insert (var2);
          P.insert (var3);
          res += ltlf2tran (root->_left, P);
          res += ltlf2tran (root->_right, P);
          break;
        case eEQUIV:
          var = get_var (root);
          var2 = get_var (root->_left);
          var3 = get_var (root->_right);
          res += "(" + var + " = (" + var2 + " <-> " + var3 + ")) & \n";
          P.insert (var);
          P.insert (var2);
          P.insert (var3);
          res += ltlf2tran (root->_left, P);
          res += ltlf2tran (root->_right, P);
          break;
         
        default:
          break;
  }
  return res;
}



std::string ltlf2smv (ltl_formula *root)
{
  std::string res = "MODULE main\nVAR\n";
  std::set<std::string> P = get_alphabet (root);
  std::string str = ltlf2tran (root, P);
  P.insert ("Tail");
  P.erase ("TRUE");
  P.erase ("FALSE");
  for (std::set<std::string>::iterator it = P.begin (); it != P.end (); it ++)
    res += (*it) + " : boolean;\n";
  res += "\nINIT\n";
  res += "var0 = TRUE;\n";
  
  str += "TRUE;\n";
  res += "\nTRANS\n" + str;
  std::string ltlspec = get_ltlspec (P);
  res += ltlspec;
  return res;
}

//bnf/fussy encoding (CGH)
void ltlf2trans_2 (ltl_formula *root, std::string& trans, std::string& defines, std::set<std::string>& P)
{
  std::string var, var2, var3, var4;
  ltl_formula *nx;
  std::set<std::string>::iterator it;
  switch (root->_type)
  {
        case eNOT:
          assert (root->_right->_type != eNOT);
          ltlf2trans_2 (root->_right, trans, defines, P);
          break;
        case eNEXT:
          var = get_var (root);
          P.insert (var);
          it = already_exists.find (var);
          if (it == already_exists.end ())
          {
            trans += "(" + var + " = ((! Tail) & next (" + get_expr (root->_right, P) + "))) & \n";
            already_exists.insert (var);
          }
          
          ltlf2trans_2 (root->_right, trans, defines, P);
          break;
        case eFUTURE:
          nx = create_operation (eNEXT, NULL, root);
          var = get_var (nx);
          var3 = get_expr (root->_right, P);
          var2 = get_var (root);
          it = already_exists.find (var2);
          if (it == already_exists.end ())
          {
            defines += var2 + " := ((" + var3 + ") | (" + var + "));\n";
            
            already_exists.insert (var2);
          }
          it = already_exists.find (var);
          if (it == already_exists.end ())
          {
            trans += "(" + var + " = ((! Tail) & next (" + var2 + "))) & \n";
            already_exists.insert (var);
          }
          P.insert (var);
          delete nx;
          ltlf2trans_2 (root->_right, trans, defines, P);
          
          break;
        case eUNTIL:
          var = get_var (root);
          nx = create_operation (eNEXT, NULL, root);
          
          var2 = get_expr (root->_left, P);
          var3 = get_expr (root->_right, P);
          var4 = get_var (nx);
          it = already_exists.find (var);
          if (it == already_exists.end ())
          {
            var2 += " & (" + var4 + ")";
            defines += var + " := ((" + var3 + ") | (" + var2 + "));\n";
            
            already_exists.insert (var);
          }
          it = already_exists.find (var4);
          if (it == already_exists.end ())
          {
            trans += "(" + var4 + " = ((! Tail) & next (" + var + "))) & \n";
            already_exists.insert (var4);
          }
         
          P.insert (var4);
          delete nx;
          ltlf2trans_2 (root->_left, trans, defines, P);
          ltlf2trans_2 (root->_right, trans, defines, P);
          
          break;
        case eOR:
          ltlf2trans_2 (root->_left, trans, defines, P);
          ltlf2trans_2 (root->_right, trans, defines, P);
        
          break;
        default:
          break;
  }
}



std::string ltlf2smv_2 (ltl_formula *root)
{
  std::string res = "MODULE main\nVAR\n";
  std::set<std::string> P = get_alphabet (root);
  std::string trans = "", defines = "";
  ltlf2trans_2 (root, trans, defines, P);
  P.insert ("Tail");
  P.erase ("TRUE");
  P.erase ("FALSE");
  for (std::set<std::string>::iterator it = P.begin (); it != P.end (); it ++)
    res += (*it) + " : boolean;\n";
  
  if (defines.compare ("") != 0)
    res += "\nDEFINE\n" + defines;    
  res += "\nINIT\n";
  res += get_expr (root, P) + "\n";
  if (trans.compare ("") != 0)
    res += "\nTRANS\n" + trans + "TRUE\n";

  std::string ltlspec = get_ltlspec (P);
  res += ltlspec;
  return res;
}

//nnf/fussy(/sloppy) encoding
void ltlf2trans_3 (ltl_formula *root, std::string& trans, std::string& defines, std::set<std::string>& P)
{
  std::string var, var2, var3, var4;
  ltl_formula *nx;
  std::set<std::string>::iterator it;
  switch (root->_type)
  {
        case eNOT:
          break;
        case eNEXT:
          var = get_var (root);
          P.insert (var);
          it = already_exists.find (var);
          if (it == already_exists.end ())
          {
            trans += "(" + var + " -> ((! Tail) & next (" + get_expr (root->_right, P) + "))) & \n";
            already_exists.insert (var);
          }
          
          ltlf2trans_3 (root->_right, trans, defines, P);
          break;
        case eWNEXT:
          var = get_var (root);
          P.insert (var);
          it = already_exists.find (var);
          if (it == already_exists.end ())
          {
            trans += "(" + var + " -> ((! Tail) -> next (" + get_expr (root->_right, P) + "))) & \n";
            already_exists.insert (var);
          }
          
          ltlf2trans_3 (root->_right, trans, defines, P);
          break;
        case eFUTURE:
          nx = create_operation (eNEXT, NULL, root);
          var = get_var (nx);
          var3 = get_expr (root->_right, P);
          var2 = get_var (root);
          it = already_exists.find (var2);
          if (it == already_exists.end ())
          {
            defines += var2 + " := ((" + var3 + ") | (" + var + "));\n";
            
            already_exists.insert (var2);
          }
          it = already_exists.find (var);
          if (it == already_exists.end ())
          {
            trans += "(" + var + " -> ((! Tail) & next (" + var2 + "))) & \n";
            already_exists.insert (var);
          }
          P.insert (var);
          delete nx;
          ltlf2trans_3 (root->_right, trans, defines, P);
          
          break;
        case eGLOBALLY:
          nx = create_operation (eNEXT, NULL, root);
          var = get_var (nx);
          var3 = get_expr (root->_right, P);
          var2 = get_var (root);
          it = already_exists.find (var2);
          if (it == already_exists.end ())
          {
            defines += var2 + " := ((" + var3 + ") & ( (! Tail) -> (" + var + ")));\n";
            
            already_exists.insert (var2);
          }
          it = already_exists.find (var);
          if (it == already_exists.end ())
          {
            trans += "(" + var + " -> ((! Tail) & next (" + var2 + "))) & \n";
            already_exists.insert (var);
          }
          P.insert (var);
          delete nx;
          ltlf2trans_3 (root->_right, trans, defines, P);
          
          break;
        case eUNTIL:
          var = get_var (root);
          nx = create_operation (eNEXT, NULL, root);
          
          var2 = get_expr (root->_left, P);
          var3 = get_expr (root->_right, P);
          var4 = get_var (nx);
          it = already_exists.find (var);
          if (it == already_exists.end ())
          {
            var2 += " & (" + var4 + ")";
            defines += var + " := ((" + var3 + ") | (" + var2 + "));\n";
            
            already_exists.insert (var);
          }
          it = already_exists.find (var4);
          if (it == already_exists.end ())
          {
            trans += "(" + var4 + " -> ((! Tail) & next (" + var + "))) & \n";
            already_exists.insert (var4);
          }
         
          P.insert (var4);
          delete nx;
          ltlf2trans_3 (root->_left, trans, defines, P);
          ltlf2trans_3 (root->_right, trans, defines, P);
          
          break;
        case eRELEASE:
          var = get_var (root);
          nx = create_operation (eNEXT, NULL, root);
          
          var2 = get_expr (root->_left, P);
          var3 = get_expr (root->_right, P);
          var4 = get_var (nx);
          it = already_exists.find (var);
          if (it == already_exists.end ())
          {
            var2 += " | (Tail |" + var4 + ")";
            defines += var + " := ((" + var3 + ") & (" + var2 + "));\n";
            
            already_exists.insert (var);
          }
          it = already_exists.find (var4);
          if (it == already_exists.end ())
          {
            trans += "(" + var4 + " -> ((! Tail) & next (" + var + "))) & \n";
            already_exists.insert (var4);
          }
         
          P.insert (var4);
          delete nx;
          ltlf2trans_3 (root->_left, trans, defines, P);
          ltlf2trans_3 (root->_right, trans, defines, P);
          
          break;
        case eOR:
          ltlf2trans_3 (root->_left, trans, defines, P);
          ltlf2trans_3 (root->_right, trans, defines, P);
        
          break;
        case eAND:
          ltlf2trans_3 (root->_left, trans, defines, P);
          ltlf2trans_3 (root->_right, trans, defines, P);
        
          break;
        default:
          break;
  }
}

std::string ltlf2smv_3 (ltl_formula *root)
{
  std::string res = "MODULE main\nVAR\n";
  std::set<std::string> P = get_alphabet (root);
  std::string trans = "", defines = "";
  ltlf2trans_3 (root, trans, defines, P);
  P.insert ("Tail");
  P.erase ("TRUE");
  P.erase ("FALSE");
  for (std::set<std::string>::iterator it = P.begin (); it != P.end (); it ++)
    res += (*it) + " : boolean;\n";
  
  if (defines.compare ("") != 0)
    res += "\nDEFINE\n" + defines;    
  res += "\nINIT\n";
  res += get_expr (root, P) + "\n";
  if (trans.compare ("") != 0)
    res += "\nTRANS\n" + trans + "TRUE\n";

  std::string ltlspec = get_ltlspec (P);
  res += ltlspec;
  return res;
}

std::string ltlf2smv_4 (ltl_formula *root)
{
  std::string res = "MODULE main\nVAR\n";
  std::set<std::string> P = get_alphabet (root);
  std::string trans = "", defines = "";
  //ltlf2trans_4 (root, trans, defines, P);
  P.insert ("Tail");
  P.erase ("TRUE");
  P.erase ("FALSE");
  for (std::set<std::string>::iterator it = P.begin (); it != P.end (); it ++)
    res += (*it) + " : boolean;\n";
  
  if (defines.compare ("") != 0)
    res += "\nDEFINE\n" + defines;    
  res += "\nINIT\n";
  res += get_expr (root, P) + "\n";
  if (trans.compare ("") != 0)
    res += "\nTRANS\n" + trans + "TRUE\n";

  std::string ltlspec = get_ltlspec (P);
  res += ltlspec;
  return res;
}
*/

std::string get_expr (ltl_formula *root, std::set<std::string>& P)
{
  std::string res = "";
  std::string var, var2, var3, var4;
  
  if (root->_type == eTRUE)
  {
    res = "TRUE";
    return res;
  }
  if (root->_type == eFALSE)
  {
    res = "FALSE";
    return res;
  }
  if (root->_var != NULL)
  {
    res = root->_var;
    return res;
  }
  switch (root->_type)
  {
        case eNOT:
          res += "(!" + get_expr (root->_right, P) + ")";
          break;
        case eNEXT:
          var = get_var (root);
          P.insert (var);
          res += var;
         
          break;
        case eWNEXT:
          var = get_var (root);
          P.insert (var);
          res += var;
         
          break;
        case eFUTURE:
        case eUNTIL:
        case eGLOBALLY:
        case eRELEASE:
          var = get_var (root);
          res += var;          
          break;
        case eOR:
          res += "(" + get_expr (root->_left, P) + " | " + get_expr (root->_right, P) + ")";
          break;
        case eAND:
          res += "(" + get_expr (root->_left, P) + " & " + get_expr (root->_right, P) + ")";
          break;
        default:
          break;
  }
  return res;
}



std::string ltlf2smv (ltl_formula *root, NF nf, ENCODE encode)
{
  std::string res = "MODULE main\nVAR\n";
  std::set<std::string> P = get_alphabet (root);
  std::string trans = "", defines = "";
  ltlf2trans (root, nf, encode, trans, defines, P);
  P.insert ("Tail");
  P.erase ("TRUE");
  P.erase ("FALSE");
  for (std::set<std::string>::iterator it = P.begin (); it != P.end (); it ++)
    res += (*it) + " : boolean;\n";
  
  if (defines.compare ("") != 0)
    res += "\nDEFINE\n" + defines;    
  res += "\nINIT\n";
  res += get_expr (root, P) + "\n";
  if (trans.compare ("") != 0)
    res += "\nTRANS\n" + trans + "TRUE\n";

  std::string ltlspec = get_ltlspec (P);
  res += ltlspec;
  return res;
}

void add_trans (std::string& left, std::string& right, ENCODE encode, std::string& trans)
{
	if (encode == fussy)
    	trans += "(" + left + " = ((! Tail) & next (" + right + "))) & \n";
    else if (encode == sloppy)
        trans += "(" + left + " -> ((! Tail) & next (" + right + "))) & \n";
    else
    {
    	printf ("Unrecoginized encoding: not fussy or sloppy\n");
        exit (0);
    }
}

void ltlf2trans (ltl_formula* root, NF nf, ENCODE encode, std::string& trans, std::string& defines, std::set<std::string>& P)
{
	std::string var, var2, var3, var4;
    ltl_formula *nx;
    std::set<std::string>::iterator it;
    switch (root->_type)
    {
        case eNOT:
          if (nf == BNF)
          {
          	assert (root->_right->_type != eNOT);
            ltlf2trans (root->_right, nf, encode, trans, defines, P);
          }
          break;
        case eNEXT:
          var = get_var (root);
          P.insert (var);
          it = already_exists.find (var);
          if (it == already_exists.end ())
          {
          	std::string temp = get_expr (root->_right, P);
          	add_trans (var, temp, encode, trans);
            
            already_exists.insert (var);
          }
          
          ltlf2trans (root->_right, nf, encode, trans, defines, P);
          break;
        case eWNEXT:
          var = get_var (root);
          P.insert (var);
          it = already_exists.find (var);
          if (it == already_exists.end ())
          {
          	std::string temp = get_expr (root->_right, P);
          	add_trans (var, temp, encode, trans);
            already_exists.insert (var);
          }
          
          ltlf2trans (root->_right, nf, encode, trans, defines, P);
          break;
        case eFUTURE:
          nx = create_operation (eNEXT, NULL, root);
          var = get_var (nx);
          var3 = get_expr (root->_right, P);
          var2 = get_var (root);
          it = already_exists.find (var2);
          if (it == already_exists.end ())
          {
            defines += var2 + " := ((" + var3 + ") | (" + var + "));\n";
            
            already_exists.insert (var2);
          }
          it = already_exists.find (var);
          if (it == already_exists.end ())
          {
          	add_trans (var, var2, encode, trans);
            //trans += "(" + var + " -> ((! Tail) & next (" + var2 + "))) & \n";
            already_exists.insert (var);
          }
          P.insert (var);
          delete nx;
          ltlf2trans (root->_right, nf, encode, trans, defines, P);
          
          break;
        case eGLOBALLY:
          assert (nf != BNF);
          nx = create_operation (eNEXT, NULL, root);
          var = get_var (nx);
          var3 = get_expr (root->_right, P);
          var2 = get_var (root);
          it = already_exists.find (var2);
          if (it == already_exists.end ())
          {
            defines += var2 + " := ((" + var3 + ") & ( (! Tail) -> (" + var + ")));\n";
            
            already_exists.insert (var2);
          }
          it = already_exists.find (var);
          if (it == already_exists.end ())
          {
          	add_trans (var, var2, encode, trans);
            //trans += "(" + var + " -> ((! Tail) & next (" + var2 + "))) & \n";
            already_exists.insert (var);
          }
          P.insert (var);
          delete nx;
          ltlf2trans (root->_right, nf, encode, trans, defines, P);
          
          break;
        case eUNTIL:
          var = get_var (root);
          nx = create_operation (eNEXT, NULL, root);
          
          var2 = get_expr (root->_left, P);
          var3 = get_expr (root->_right, P);
          var4 = get_var (nx);
          it = already_exists.find (var);
          if (it == already_exists.end ())
          {
            var2 += " & (" + var4 + ")";
            defines += var + " := ((" + var3 + ") | (" + var2 + "));\n";
            
            already_exists.insert (var);
          }
          it = already_exists.find (var4);
          if (it == already_exists.end ())
          {
            add_trans (var4, var, encode, trans);
            //trans += "(" + var4 + " -> ((! Tail) & next (" + var + "))) & \n";
            already_exists.insert (var4);
          }
         
          P.insert (var4);
          delete nx;
          ltlf2trans (root->_left, nf, encode, trans, defines, P);
          ltlf2trans (root->_right, nf, encode, trans, defines, P);
          
          break;
        case eRELEASE:
          assert (nf != BNF);
          var = get_var (root);
          nx = create_operation (eNEXT, NULL, root);
          
          var2 = get_expr (root->_left, P);
          var3 = get_expr (root->_right, P);
          var4 = get_var (nx);
          it = already_exists.find (var);
          if (it == already_exists.end ())
          {
            var2 += " | (Tail |" + var4 + ")";
            defines += var + " := ((" + var3 + ") & (" + var2 + "));\n";
            
            already_exists.insert (var);
          }
          it = already_exists.find (var4);
          if (it == already_exists.end ())
          {
          	add_trans (var4, var, encode, trans);
            //trans += "(" + var4 + " -> ((! Tail) & next (" + var + "))) & \n";
            already_exists.insert (var4);
          }
         
          P.insert (var4);
          delete nx;
          ltlf2trans (root->_left, nf, encode, trans, defines, P);
          ltlf2trans (root->_right, nf, encode, trans, defines, P);
          
          break;
        case eOR:
          ltlf2trans (root->_left, nf, encode, trans, defines, P);
          ltlf2trans (root->_right, nf, encode, trans, defines, P);
        
          break;
        case eAND:
          assert (nf != BNF);
          ltlf2trans (root->_left, nf, encode, trans, defines, P);
          ltlf2trans (root->_right, nf, encode, trans, defines, P);
        
          break;
        default:
          break;
  }
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
   
    ltl_formula *newroot = bnf (root);
    //printf ("%s\n", to_string (newroot).c_str ());
    std::string res = ltlf2smv (newroot, BNF, fussy);
    
    printf ("%s\n", res.c_str ());
    destroy_formula (root);
    destroy_formula (newroot);
}






