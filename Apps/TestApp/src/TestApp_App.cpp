#include "TestApp_App.h"
#include "App_Windows.h"

namespace ASR
{
namespace TestApp
{
TestAppApp g_TestApp;

// ------------------------------------------------------
// ------------------------------------------------------
void TestAppApp::ExecuteApp()
{
    // Note(asr): Add check here to make sure we don't execute more than once

    ASR::App::InitAppParams params;
    Inititalize( params );

    while( m_State != App::State::Exiting )
        Run();

    Terminate();
}

// ------------------------------------------------------
// ------------------------------------------------------
void TestAppApp::Run()
{
    ProcessOSMessages();
}

// ------------------------------------------------------
// ------------------------------------------------------
void TestAppApp::DestroyAppWindow()
{
    App::AppWindows::DestroyAppWindow();
    m_State = App::State::Exiting;
}

} // end namespace TestApp
} // end namespace ASR
