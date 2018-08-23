/* 
 * File:   olg_formula.h
 * Author: yaoyinbo
 * 
 * φ = p: ofp(φ) = ⟨p,1,−⟩;
 * φ = φ1 ∧ φ2: ofp(φ) = ofp(φ1) ∧ ofp(φ2);
 * φ = φ1 ∨ φ2: ofp(φ) = ofp(φ1) ∨ ofp(φ2);
 * φ = X φ1: ofp(φ) = Pos(ofp(φ1), X );
 * φ = Gφ1: ofp(φ) = Pos(ofp(φ1),G);
 * φ = φ1Uφ2: ofp(φ) = Pos(ofp(φ2), U);
 * φ = φ1Rφ2 (φ1 != ff): ofp(φ) = Pos(ofp(φ2), R);
 * 
 * Created on October 25, 2013, 10:38 PM
 */

#ifndef OLG_FORMULA_H
#define	OLG_FORMULA_H

#include "olg_item.h"

#include <stdlib.h>


namespace aalta
{

class olg_formula
{
private:
  ////////////
  //成员变量//
  //////////////////////////////////////////////////
  olg_item *_root; // olg_formula根节点

  
  int *_pos_arr;
  int _pos_size;
  int *_id_arr;
  int _id_size;

  /* 以下成员用于处理 φi ∧ G (Ai | F Bi) 类型公式 */
  std::list<olg_item *> _ALLG; // Ai | F Bi 节点
  
  std::list<olg_item *> _GF; // G(Ai | F Bi) formulas 
  std::list<olg_item *> _NG; // Not G formulas formulas
  std::list<olg_item *> _G; // G formulas
  std::list<olg_item *> _GR; // G release formulas
  std::list<olg_item *> _GU; //G until formulas
  std::list<olg_item *> _GX; //G X formulas
  std::list<olg_item *> _GP; //G P formulas
  std::list<olg_item *> _NR; //not release contained formulas
  std::list<olg_item *> _R; //release contained formulas
  
  std::list<aalta_formula *> _FGX; // G X aalta formulas
  
  

  //////////////////////////////////////////////////

public:
  olg_formula (aalta_formula *af);
  virtual ~olg_formula ();

  bool sat ();
  bool unsat ();

  std::string to_string() const;
  std::string to_olg_string ()const;
  
  //added by Jianwen Li
  std::vector<aalta_formula*> _GX_loop;  
  olg_item* GX_imply ();  
  olg_item* G_be_implied ();                
  void GX_loop ();                  
  aalta_formula* build_from_set (hash_set<aalta_formula*>);      
  bool find_in_GX_loop (aalta_formula*);  
  std::pair<aalta_formula*, aalta_formula*> split_GX (aalta_formula*);
  int get_pos (aalta_formula*);
  //added by Jianwen Li

private:
  void classify (aalta_formula *af);
  olg_item *build (const aalta_formula *af);
  void col_info(olg_item *root);
  void destroy (olg_item *root);

public: 
  hash_map<int, bool> _evidence; //for evidence
  

};

}

#endif	/* OLG_FORMULA_H */

