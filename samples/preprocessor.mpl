#define number 5

#if number == 5
  #warning right warning
#endif

#if ! defined (number) || number != 5
  #error ded
#else
  #error ded2
#endif
