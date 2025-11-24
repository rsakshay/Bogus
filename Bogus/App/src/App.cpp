#include "App.h"

namespace ASR
{
namespace App
{
Application* g_pApp;

// ------------------------------------------------------
// ------------------------------------------------------
Application::Application()
{
    g_pApp = (Application*)this;
}

// ------------------------------------------------------
// ------------------------------------------------------
void Application::Inititalize( InitAppParams const& kParams )
{
    m_State = State::Running;
    CreateAppWindow( kParams.windowParams );
}

// ------------------------------------------------------
// ------------------------------------------------------
void Application::Terminate()
{
    DestroyAppWindow();
}
} // namespace App
} // namespace ASR
