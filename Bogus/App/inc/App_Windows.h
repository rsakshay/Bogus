#ifndef APP_WINDOWS_H
#define APP_WINDOWS_H
#include "App.h"
#include "Globals.h"

#include <windows.h>

namespace Bogus
{
namespace App
{
// ------------------------------------------------------
// ------------------------------------------------------
class AppWindows : public Application
{
  public:
    AppWindows();
    virtual ~AppWindows() {};

    virtual void CreateAppWindow( CreateWindowParams const& kParams ) override;
    virtual void DestroyAppWindow() override;
    virtual void ProcessOSMessages() override;

    void HandleResize( uint32 uiWidth, uint32 uiHeight );

    HWND m_hWnd;
    uint32 m_uiClientWidth = 1920;
    uint32 m_uiClientHeight = 1080;
    HINSTANCE m_hInstance;
    LPSTR m_lpCmdLine;
    int32 m_nCmdShow;
};

extern AppWindows* g_pAppWindows;
} // namespace App
} // namespace Bogus
#endif
