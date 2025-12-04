#include "Core_Assert.h"
#include <iostream>
#ifdef _WIN32
#include "windows.h"
#endif

namespace Bogus::Core
{

void AssertWithMessage( bool bExpression, char const* szMsg )
{
    if( !bExpression )
    {
        std::cerr << "\n\n\x1b[31m[ERROR]: " << szMsg << "\n\x1b[0m";
#ifdef _WIN32
        if( IsDebuggerPresent() )
        {
            __debugbreak();
        }
#endif
    }
}

} // namespace Bogus::Core
