/* 
 * 定义实用函数
 * File:   utility.h
 * Author: yaoyinbo
 *
 * Created on October 21, 2013, 2:12 PM
 */

#ifndef UTILITY_H
#define	UTILITY_H

#include <sstream>
#include <string>
#include <vector>
#include "util/define.h"

void print_error (const char *msg);

void print_msg (const char *msg);

bool file_write(const char *path, const char *str);

/**
 * 转string
 * @param val
 * @return 
 */
template <typename T>
std::string
convert_to_string (T val)
{
  std::stringstream ss;
  ss << val;
  return ss.str ();
}

/**
 * split a string by whitespace, added by Jianwen LI
 */
std::vector<std::string> split_str(std::string str);


#endif	/* UTILITY_H */

