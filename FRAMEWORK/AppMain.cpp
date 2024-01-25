
// Use this as Main C++ file for an app

#include "Framework.hpp"

//using namespace NCMN;
//using namespace NFWK::NGenericTypes;
using namespace NFWK;   // TODO: Configurable

#if __has_include ("AppMain.hpp")
#define _APPENTRYPT
#include "AppMain.hpp"         // Your app implementation (When fs link is to this file)
#elif __has_include ("../AppMain.hpp")
#define _APPENTRYPT
#include "../AppMain.hpp"      // Your app implementation (When fs link is to /COMMON/FRAMEWORK)
#elif __has_include ("../../AppMain.hpp")
#define _APPENTRYPT
#include "../../AppMain.hpp"   // Your app implementation (When fs link is to /COMMON)
#else
#pragma message(">>> No AppMain.hpp is found - expecting ModuleMain as entry point!")
#endif
#include "Platforms/EntryPoints.hpp"     // Specify 'AppEntryPoint' to the linker
