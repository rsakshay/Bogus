#ifndef CORE_ASSERT_H
#define CORE_ASSERT_H

#define BGASSERT( bExpression, szMsg ) Bogus::Core::AssertWithMessage( bExpression, #szMsg )
namespace Bogus::Core
{

void AssertWithMessage( bool bExpression, char const* szMsg );

} // namespace Bogus::Core
#endif
