#pragma once
#include "App.g.h"

namespace winrt::RokeSettings::implementation
{
    struct App : AppT<App, winrt::RokeSettings::IApp>
    {
        using composable = winrt::RokeSettings::App;

        App();
        void OnLaunched(Microsoft::UI::Xaml::LaunchActivatedEventArgs const&);
    private:
        winrt::Microsoft::UI::Xaml::Window m_window{ nullptr };
    };
}

namespace winrt::RokeSettings::factory_implementation
{
    struct App : AppT<App, implementation::App>
    {
    };
}
