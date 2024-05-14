#ifndef APP_WINDOWS_H
#define APP_WINDOWS_H
#include "App.h"
#include "Globals.h"

#include <windows.h>

namespace ASR
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
    virtual void ProcessOSMessages() override;

    HWND m_hWnd;
    HINSTANCE m_hInstance;
    LPSTR m_lpCmdLine;
    int32 m_nCmdShow;
};

extern AppWindows* g_pAppWindows;
} // end namespace Core
} // end namespace ASR
#endif