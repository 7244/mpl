#define salsa

#ifdef salsa
  #ifdef not_defined_name
    #some invalid stuff
    #error wrong error message0
  #else
    #error right error message
  #endif
#else
  #error wrong error message1
#endif
