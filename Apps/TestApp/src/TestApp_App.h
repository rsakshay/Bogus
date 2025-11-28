#ifndef TEST_APP_H
#define TEST_APP_H
#include "App_Windows.h"

namespace ASR
{
namespace TestApp
{

// ------------------------------------------------------
// ------------------------------------------------------
class TestAppApp : Bogus::App::AppWindows
{
  public:
    void ExecuteApp() override;
    void DestroyAppWindow() override;

  private:
    void Run();
};

} // end namespace TestApp
} // end namespace ASR
#endif
