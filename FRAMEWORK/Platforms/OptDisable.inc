#if defined(COMP_MSVC) 
#pragma optimize("", off)
#elif defined(COMP_CLNG)
#pragma clang optimize off
#elif defined(COMP_INTL)
#pragma GCC optimization_level 1 
#elif defined(COMP_AGCC)
#pragma GCC push_options
#pragma GCC optimize ("-O0")
#endif