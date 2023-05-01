
// Use this as Main C++ file for an app

#include "Framework.hpp"

//using namespace NCMN;
//using namespace NFWK::NGenericTypes; 
using namespace NFWK;   // TODO: Configurable

#if __has_include ("AppMain.hpp")
#define _APPENTRYPT
#include "AppMain.hpp"         // Your app implementation
#elif __has_include ("../AppMain.hpp")
#define _APPENTRYPT
#include "../AppMain.hpp"      // Your app implementation
#elif __has_include ("../../AppMain.hpp")
#define _APPENTRYPT
#include "../../AppMain.hpp"   // Your app implementation
#endif
#include "EntryPoints.hpp"     // Specify 'AppEntryPoint' to the linker