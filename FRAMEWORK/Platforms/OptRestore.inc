#if defined(COMP_MSVC) 
#pragma optimize("", on)
#elif defined(COMP_CLNG)
#pragma clang optimize on
#elif defined(COMP_INTL)
#pragma GCC optimization_level reset 
#elif defined(COMP_AGCC)
#pragma GCC pop_options
#endif