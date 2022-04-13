//
// Created by mixi on feb/3/2017.
//

#ifdef __APPLE__
  #ifndef FMT_LLU
    #define FMT_LLU     "%llu"
  #endif
  #ifndef LLU
    #define LLU         "llu"
  #endif
#else
  #ifndef FMT_LLU
    #define FMT_LLU     "%lu"
  #endif
  #ifndef LLU
    #define LLU         "lu"
  #endif
#endif