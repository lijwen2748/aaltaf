/* 
 * parser部分主函数
 * File:   main.c
 * Author: yaoyinbo
 *
 * Created on October 20, 2013, 9:11 PM
 */

#include "../ltl_formula.h"
#include "../trans.h"

#include <stdio.h>
#include <stdlib.h>

#define MAXN 10000

char in[MAXN];

int
main (int argc, char** argv)
{
  while (gets (in))
    {
      ltl_formula *formula = getAST(in);
      print_formula(formula);
      puts("");
      destroy_formula(formula);
    }

  return (EXIT_SUCCESS);
}

