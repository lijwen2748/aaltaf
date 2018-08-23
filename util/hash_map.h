/* 
 * File:   hash_map.h
 * Author: yaoyinbo
 *
 * Created on October 29, 2013, 1:46 PM
 */

#ifndef HASH_MAP_H
#define	HASH_MAP_H

#if defined __APPLE__

#include <unordered_map>

#ifndef hash_map
#define hash_map std::unordered_map
#endif

#elif defined __GNUC__

#include <tr1/unordered_map>

#ifndef hash_map
#define hash_map std::tr1::unordered_map
#endif

#else //@ TODO: 补完整
#include <hash_map>
#endif

#ifndef HASH_INIT
#define HASH_INIT 1315423911
#endif

#endif	/* HASH_MAP_H */

