#include "App_Windows.h"

#include "Renderer.h"

#include <tchar.h>
#include <winuser.h>

static TCHAR szWindowClass[] = _T( "BogusApp" );
static TCHAR szTitle[] = _T( "BOGUS" );

// ENTRY POINT
int APIENTRY WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
    Bogus::App::g_pAppWindows->m_hInstance = hInstance;
    Bogus::App::g_pAppWindows->m_lpCmdLine = lpCmdLine;
    Bogus::App::g_pAppWindows->m_nCmdShow = nCmdShow;

    Bogus::App::g_pAppWindows->ExecuteApp();

    return 0;
}

// ------------------------------------------------------
namespace Bogus
{
// ------------------------------------------------------
namespace App
{
AppWindows* g_pAppWindows;

static LRESULT CALLBACK StaticWndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );

// ------------------------------------------------------
// ------------------------------------------------------
AppWindows::AppWindows()
{
    g_pAppWindows = (AppWindows*)this;
}

// ------------------------------------------------------
// ------------------------------------------------------
void AppWindows::CreateAppWindow( CreateWindowParams const& kParams )
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof( WNDCLASSEX );
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = StaticWndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = m_hInstance;
    wcex.hIcon = LoadIcon( wcex.hInstance, IDI_APPLICATION );
    wcex.hCursor = LoadCursor( NULL, IDC_ARROW );
    wcex.hbrBackground = (HBRUSH)( COLOR_WINDOW + 1 );
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon( wcex.hInstance, IDI_APPLICATION );

    if( !RegisterClassEx( &wcex ) )
    {
        MessageBox( NULL, _T( "Failed to register WNDCLASS" ), _T( "Bogus App window" ), NULL );
        return;
    }

    m_hWnd = CreateWindowEx(
        WS_EX_OVERLAPPEDWINDOW, szWindowClass, szTitle,
        ( WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX ),
        CW_USEDEFAULT, CW_USEDEFAULT, m_uiClientWidth, m_uiClientHeight, NULL, NULL, m_hInstance,
        NULL );

    if( !m_hWnd )
    {
        MessageBox( NULL, _T( "Failed to create window" ), _T( "Bogus App window" ), NULL );
        return;
    }

    Bogus::Renderer::Initialize();
    ShowWindow( m_hWnd, m_nCmdShow );
    UpdateWindow( m_hWnd );
}

// ------------------------------------------------------
// ------------------------------------------------------
void AppWindows::DestroyAppWindow()
{
    Bogus::Renderer::Terminate();
    DestroyWindow( m_hWnd );
}

// ------------------------------------------------------
// ------------------------------------------------------
void AppWindows::ProcessOSMessages()
{
    MSG msg;
    while( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
    {
        TranslateMessage( &msg );
        DispatchMessage( &msg );
    }
}

// ------------------------------------------------------
// ------------------------------------------------------
void AppWindows::HandleResize( uint32 uiWidth, uint32 uiHeight )
{
    if( m_uiClientWidth == uiWidth && m_uiClientHeight == uiHeight )
    {
        return;
    }

    m_uiClientWidth = uiWidth;
    m_uiClientHeight = uiHeight;
    Renderer::Resize( uiWidth, uiHeight );
}

// ------------------------------------------------------
static LRESULT CALLBACK StaticWndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    switch( message )
    {
    case WM_PAINT:
    {
        Bogus::Renderer::Render();
    }
    break;

    case WM_DESTROY:
    {
        PostQuitMessage( 0 );
    }
    break;

    case WM_CLOSE:
    {
        g_pAppWindows->DestroyAppWindow();
    }
    break;

    case WM_SIZE:
    {
        RECT clientRect = {};
        GetClientRect( hWnd, &clientRect );

        uint32 const uiWidth = clientRect.right - clientRect.left;
        uint32 const uiHeight = clientRect.bottom - clientRect.top;
        g_pAppWindows->HandleResize( uiWidth, uiHeight );
    }
    break;

    case WM_CREATE:
    {
        SetActiveWindow( hWnd );
    } // follow through
    default:
    {
        return DefWindowProc( hWnd, message, wParam, lParam );
    }
    break;
    }

    return 0;
}

} // end namespace App
} // namespace Bogus
