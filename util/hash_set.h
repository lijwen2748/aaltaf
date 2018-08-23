/* 
 * File:   hash_set.h
 * Author: yaoyinbo
 *
 * Created on October 29, 2013, 2:05 PM
 */

#ifndef HASH_SET_H
#define	HASH_SET_H


#if defined __APPLE__

#include <unordered_set>

#ifndef hash_set
#define hash_set std::unordered_set
#endif

#elif defined __GNUC__

#include <tr1/unordered_set>

#ifndef hash_set
#define hash_set std::tr1::unordered_set
#endif

#else //@ TODO: 补完整
#include <hash_set>
#endif

#ifndef HASH_INIT
#define HASH_INIT 1315423911
#endif

#endif	/* HASH_SET_H */

