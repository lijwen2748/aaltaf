/* A Bison parser, made by GNU Bison 3.0.4.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

#ifndef YY_YY_LTLPARSER_H_INCLUDED
# define YY_YY_LTLPARSER_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif
/* "%code requires" blocks.  */
#line 19 "ltlparser.y" /* yacc.c:1909  */

#ifndef YY_TYPEDEF_YY_SCANNER_T
#define YY_TYPEDEF_YY_SCANNER_T
typedef void* yyscan_t;
#endif

#line 51 "../ltlparser.h" /* yacc.c:1909  */

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    TOKEN_EQUIV = 258,
    TOKEN_IMPLIES = 259,
    TOKEN_OR = 260,
    TOKEN_AND = 261,
    TOKEN_RELEASE = 262,
    TOKEN_UNTIL = 263,
    TOKEN_WEAK_UNTIL = 264,
    TOKEN_FUTURE = 265,
    TOKEN_GLOBALLY = 266,
    TOKEN_NEXT = 267,
    TOKEN_WEAK_NEXT = 268,
    TOKEN_NOT = 269,
    TOKEN_TRUE = 270,
    TOKEN_FALSE = 271,
    TOKEN_LPAREN = 272,
    TOKEN_RPAREN = 273,
    TOKEN_VARIABLE = 274
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 34 "ltlparser.y" /* yacc.c:1909  */

	char* var_name;
	ltl_formula *formula;

#line 88 "../ltlparser.h" /* yacc.c:1909  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif



int yyparse (ltl_formula **formula, yyscan_t scanner);

#endif /* !YY_YY_LTLPARSER_H_INCLUDED  */
