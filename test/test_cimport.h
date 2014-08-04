/* This file is here to test the passing of -I and -D flags
   to cpp
*/
#ifdef GOOD
#include <test/test_cimport.1>
#else
#error "GOOD is not defined"
#endif
