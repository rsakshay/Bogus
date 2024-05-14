#ifndef TEST_APP_H
#define TEST_APP_H
#include "App_Windows.h"

namespace ASR
{
namespace TestApp
{

// ------------------------------------------------------
// ------------------------------------------------------
class TestAppApp : ASR::App::AppWindows
{
public:
    void ExecuteApp() override;

private:
    ASR::App::State Run();
};

} // end namespace TestApp
} // end namespace ASR
#endif