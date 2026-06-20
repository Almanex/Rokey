#include <unknwn.h>
#include <windows.h>
#undef GetCurrentTime
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Microsoft.UI.Xaml.h>
#include <winrt/Microsoft.UI.Xaml.Controls.h>
#include <winrt/Microsoft.UI.Xaml.Markup.h>
#include "App.xaml.h"
#include "MainWindow.xaml.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;

namespace winrt::RokeSettings::implementation
{
    App::App()
    {
        InitializeComponent();
    }

    void App::OnLaunched(LaunchActivatedEventArgs const&)
    {
        // Single instance check
        HANDLE hMutex = CreateMutexW(NULL, TRUE, L"RokeSettingsMutex");
        if (hMutex && GetLastError() == ERROR_ALREADY_EXISTS)
        {
            HWND hwnd = FindWindowW(NULL, L"Rokey - Настройки");
            if (hwnd)
            {
                ShowWindow(hwnd, SW_RESTORE);
                SetForegroundWindow(hwnd);
            }
            if (hMutex) CloseHandle(hMutex);
            ExitProcess(0);
        }

        m_window = make<MainWindow>();
        m_window.Activate();
    }
}

int __stdcall wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    winrt::init_apartment(winrt::apartment_type::single_threaded);
    ::winrt::Microsoft::UI::Xaml::Application::Start([](auto&&)
        {
            ::winrt::make<winrt::RokeSettings::implementation::App>();
        });
    return 0;
}
