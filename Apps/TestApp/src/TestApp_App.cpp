#include "TestApp_App.h"

namespace ASR
{
namespace TestApp
{
TestAppApp g_TestApp;

// ------------------------------------------------------
// ------------------------------------------------------
void TestAppApp::ExecuteApp()
{
    //Note(asr): Add check here to make sure we don't execute more than once

    ASR::App::InitAppParams params;
    Inititalize( params );

    while( m_State != App::State::Exiting )
        m_State = Run();
}

// ------------------------------------------------------
// ------------------------------------------------------
ASR::App::State TestAppApp::Run()
{
    ProcessOSMessages();
    return ASR::App::State::Running;
}

} // end namespace TestApp 
} // end namespace ASR