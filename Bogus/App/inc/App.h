#ifndef APP_H
#define APP_H

namespace ASR
{
namespace App
{
// ------------------------------------------------------
struct CreateWindowParams
{
};
// ------------------------------------------------------
struct InitAppParams
{
    CreateWindowParams windowParams;
};
enum class State
{
    Starting,
    Running,
    Exiting
};
// ------------------------------------------------------
// ------------------------------------------------------
class Application
{
public:
    Application();
    void Inititalize(InitAppParams const& kParams);
    virtual void CreateAppWindow( CreateWindowParams const& kParams ) = 0;
    virtual void ExecuteApp() = 0;
    virtual void ProcessOSMessages() = 0;
    State m_State = State::Starting;
};

extern Application* g_pApp;
}
}
#endif