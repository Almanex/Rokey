#include <windows.h>
#undef GetCurrentTime
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Microsoft.UI.Xaml.h>
#include <winrt/Microsoft.UI.Xaml.Controls.h>
#include <winrt/Microsoft.UI.Xaml.Controls.Primitives.h>
#include <microsoft.ui.xaml.window.h>
#include "MainWindow.xaml.h"


using namespace winrt;
using namespace Microsoft::UI::Xaml;

#define WM_RELOAD_SETTINGS (WM_USER + 100)

namespace winrt::RokeSettings::implementation
{
    MainWindow::MainWindow()
    {
        InitializeComponent();

        auto windowNative{ this->try_as<::IWindowNative>() };
        HWND hwnd{ nullptr };
        if (windowNative)
        {
            windowNative->get_WindowHandle(&hwnd);
        }

        if (hwnd)
        {
            // Set Window Title
            SetWindowTextW(hwnd, L"Rokey - Настройки");

            // Center the window on the primary screen with DPI scaling
            UINT dpi = GetDpiForWindow(hwnd);
            double scale = dpi / 96.0;
            int width = static_cast<int>(640 * scale);
            int height = static_cast<int>(700 * scale);

            int screenWidth = GetSystemMetrics(SM_CXSCREEN);
            int screenHeight = GetSystemMetrics(SM_CYSCREEN);
            int x = (screenWidth - width) / 2;
            int y = (screenHeight - height) / 2;
            SetWindowPos(hwnd, NULL, x, y, width, height, SWP_NOZORDER | SWP_SHOWWINDOW);
        }

        LoadSettings();
        LoadAutostart();
        m_loading = false;
    }

    void MainWindow::LoadSettings()
    {
        HKEY hKey;
        bool restoreClipboard = true;
        bool switchLayout = true;
        bool playSound = true;
        int hotkeyType = 0;

        if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Roke", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            DWORD dwType = REG_DWORD;
            DWORD dwSize = sizeof(DWORD);
            DWORD val = 0;
            
            if (RegQueryValueExW(hKey, L"RestoreClipboard", NULL, &dwType, (LPBYTE)&val, &dwSize) == ERROR_SUCCESS) {
                restoreClipboard = (val != 0);
            }
            if (RegQueryValueExW(hKey, L"SwitchLayout", NULL, &dwType, (LPBYTE)&val, &dwSize) == ERROR_SUCCESS) {
                switchLayout = (val != 0);
            }
            if (RegQueryValueExW(hKey, L"PlaySound", NULL, &dwType, (LPBYTE)&val, &dwSize) == ERROR_SUCCESS) {
                playSound = (val != 0);
            }
            if (RegQueryValueExW(hKey, L"HotkeyType", NULL, &dwType, (LPBYTE)&val, &dwSize) == ERROR_SUCCESS) {
                hotkeyType = static_cast<int>(val);
            }
            RegCloseKey(hKey);
        }

        RestoreClipboardToggle().IsOn(restoreClipboard);
        SwitchLayoutToggle().IsOn(switchLayout);
        PlaySoundToggle().IsOn(playSound);
        HotkeyComboBox().SelectedIndex(hotkeyType);
    }

    void MainWindow::SaveSettings()
    {
        if (m_loading) return;

        HKEY hKey;
        if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Roke", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
            DWORD val = RestoreClipboardToggle().IsOn() ? 1 : 0;
            RegSetValueExW(hKey, L"RestoreClipboard", 0, REG_DWORD, (const BYTE*)&val, sizeof(DWORD));
            
            val = SwitchLayoutToggle().IsOn() ? 1 : 0;
            RegSetValueExW(hKey, L"SwitchLayout", 0, REG_DWORD, (const BYTE*)&val, sizeof(DWORD));
            
            val = PlaySoundToggle().IsOn() ? 1 : 0;
            RegSetValueExW(hKey, L"PlaySound", 0, REG_DWORD, (const BYTE*)&val, sizeof(DWORD));
            
            val = static_cast<DWORD>(HotkeyComboBox().SelectedIndex());
            RegSetValueExW(hKey, L"HotkeyType", 0, REG_DWORD, (const BYTE*)&val, sizeof(DWORD));
            
            RegCloseKey(hKey);
        }

        NotifyWorker();
    }

    void MainWindow::LoadAutostart()
    {
        bool autostart = false;
        HKEY hKey;
        if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            BYTE value[512];
            DWORD size = sizeof(value);
            if (RegQueryValueExW(hKey, L"Roke", NULL, NULL, value, &size) == ERROR_SUCCESS) {
                autostart = true;
            }
            RegCloseKey(hKey);
        }
        AutostartToggle().IsOn(autostart);
    }

    void MainWindow::SaveAutostart(bool enable)
    {
        if (m_loading) return;

        HKEY hKey;
        if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_READ | KEY_WRITE, &hKey) == ERROR_SUCCESS) {
            if (enable) {
                wchar_t exePath[512];
                GetModuleFileNameW(NULL, exePath, 512);
                std::wstring pathStr(exePath);
                size_t pos = pathStr.find_last_of(L"\\/");
                if (pos != std::wstring::npos) {
                    pathStr = pathStr.substr(0, pos + 1) + L"Roke.exe";
                } else {
                    pathStr = L"Roke.exe";
                }
                RegSetValueExW(hKey, L"Roke", 0, REG_SZ, (const BYTE*)pathStr.c_str(), (DWORD)(pathStr.length() + 1) * sizeof(wchar_t));
            } else {
                RegDeleteValueW(hKey, L"Roke");
            }
            RegCloseKey(hKey);
        }
    }

    void MainWindow::NotifyWorker()
    {
        HWND hwndWorker = FindWindowW(L"RokeTrayClass", L"Roke Tray");
        if (hwndWorker) {
            PostMessageW(hwndWorker, WM_RELOAD_SETTINGS, 0, 0);
        }
    }

    void MainWindow::OnSettingChanged(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
    {
        SaveSettings();
    }

    void MainWindow::OnAutostartChanged(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
    {
        SaveAutostart(AutostartToggle().IsOn());
    }

    void MainWindow::OnHotkeyChanged(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const&)
    {
        SaveSettings();
    }
}
