
#include "utility.h"

#include <stdio.h>
#include <iostream>

void
print_error (const char *msg)
{
  fprintf (stderr, "\033[31mERROR\033[0m: %s\n", msg);
}

void
print_msg (const char *msg)
{
  fprintf (stdout, "\033[34mMESSAGE\033[0m: %s\n", msg);
}

bool
file_write (const char *path, const char *str)
{
  FILE *fp = fopen (path, "w");
  if(fp == NULL) return false;
  fprintf (fp, "%s", str);
  fclose (fp);
  return true;
}

/**
 * split a string by whitespace
 */
std::vector<std::string>
split_str(std::string str)
{
  std::vector<std::string> result;
  std::istringstream iss(str);
  std::string s;
  while ( getline( iss, s, ' ' ) ) 
  {
    result.push_back(s);
  }
  
  return result; 
}
