#define WINVER 0x0500
#include "windows.h"

#ifdef CHECK_FUNCTION_EXISTS

#ifdef __CLASSIC_C__
int main(){
  int ac;
  char*av[];
#else
int main(int ac, char*av[]){
#endif
  void * test;
  CHECK_FUNCTION_EXISTS(test, test, test, 0, test, test, 0, test, test, test, test);
  if(ac > 1000)
    {
    return *av[0];
    }
  return 0;
}

#else  /* CHECK_FUNCTION_EXISTS */

#  error "CHECK_FUNCTION_EXISTS has to specify the function"

#endif /* CHECK_FUNCTION_EXISTS */
