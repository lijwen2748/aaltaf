/* 
 * File:   debug.h
 * Author: Jianwen Li
 * Note: Define debug output
 * Created on June 26, 2017
 */
 
#ifndef DEBUG_H
#define DEBUG_H
 

#ifdef DEBUG
#define dout cout
#else
#define dout 0 && cout
#endif


#endif
