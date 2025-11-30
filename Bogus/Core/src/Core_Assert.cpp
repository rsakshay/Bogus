#include "Core_Assert.h"
#include <iostream>

namespace Bogus::Core
{

void AssertWithMessage( bool bExpression, char const* szMsg )
{
    if( !bExpression )
    {
        std::cerr << printf( "[ERROR]: %s\n", szMsg );
        __debugbreak();
    }
}

} // namespace Bogus::Core
