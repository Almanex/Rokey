#pragma once
#include "MainWindow.g.h"
#include <winrt/Microsoft.UI.Xaml.h>

namespace winrt::RokeSettings::implementation
{
    struct MainWindow : MainWindowT<MainWindow>
    {
        MainWindow();

        void OnSettingChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void OnAutostartChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void OnHotkeyChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const& e);

    private:
        bool m_loading{ true };

        void LoadSettings();
        void SaveSettings();
        void LoadAutostart();
        void SaveAutostart(bool enable);
        void NotifyWorker();
    };
}

namespace winrt::RokeSettings::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}
