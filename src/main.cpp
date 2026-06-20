/*
 * Roke - Keyboard Layout Converter
 * Скрывается в трей, взаимодействие через иконку
 * Фоновый воркер
 */

#include <windows.h>
#include <string>
#include <unordered_map>
#include <shellapi.h>
#include <objbase.h>
#include <vector>
#include "resource.h"

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")

#define WM_RELOAD_SETTINGS (WM_USER + 100)

// ============== Settings & Persistence ==============
struct Settings {
    bool restoreClipboard = true;
    bool switchLayout = true;
    bool playSound = true;
    int hotkeyType = 0; // 0 = Ctrl+Shift+Q, 1 = Ctrl+Alt+Q, 2 = F12
};

Settings g_settings;

void LoadSettings() {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Roke", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD dwType = REG_DWORD;
        DWORD dwSize = sizeof(DWORD);
        DWORD val = 0;
        
        if (RegQueryValueExW(hKey, L"RestoreClipboard", NULL, &dwType, (LPBYTE)&val, &dwSize) == ERROR_SUCCESS) {
            g_settings.restoreClipboard = (val != 0);
        }
        if (RegQueryValueExW(hKey, L"SwitchLayout", NULL, &dwType, (LPBYTE)&val, &dwSize) == ERROR_SUCCESS) {
            g_settings.switchLayout = (val != 0);
        }
        if (RegQueryValueExW(hKey, L"PlaySound", NULL, &dwType, (LPBYTE)&val, &dwSize) == ERROR_SUCCESS) {
            g_settings.playSound = (val != 0);
        }
        if (RegQueryValueExW(hKey, L"HotkeyType", NULL, &dwType, (LPBYTE)&val, &dwSize) == ERROR_SUCCESS) {
            g_settings.hotkeyType = static_cast<int>(val);
        }
        RegCloseKey(hKey);
    }
}

// ============== Layout Converter ==============
class LayoutConverter {
public:
    static LayoutConverter& instance() {
        static LayoutConverter inst;
        return inst;
    }
    
    std::wstring convert(const std::wstring& text, bool& isToRussian) const {
        if (is_russian(text)) {
            isToRussian = false;
            return ru_to_en(text);
        } else {
            isToRussian = true;
            return en_to_ru(text);
        }
    }
    
private:
    LayoutConverter() = default;
    
    bool is_russian(const std::wstring& text) const {
        int ru = 0, en = 0;
        for (wchar_t c : text) {
            if (is_cyrillic(c)) ru++;
            else if (is_latin(c)) en++;
        }
        return ru > en;
    }
    
    bool is_cyrillic(wchar_t c) const {
        return (c >= 0x0410 && c <= 0x044F) || c == 0x0401 || c == 0x0451;
    }
    
    bool is_latin(wchar_t c) const {
        return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
    }
    
    std::wstring en_to_ru(const std::wstring& text) const {
        static const std::unordered_map<wchar_t, wchar_t> map = {
            {'q', L'й'}, {'w', L'ц'}, {'e', L'у'}, {'r', L'к'}, {'t', L'е'},
            {'y', L'н'}, {'u', L'г'}, {'i', L'ш'}, {'o', L'щ'}, {'p', L'з'},
            {'[', L'х'}, {']', L'ъ'}, {'a', L'ф'}, {'s', L'ы'}, {'d', L'в'},
            {'f', L'а'}, {'g', L'п'}, {'h', L'р'}, {'j', L'о'}, {'k', L'л'},
            {'l', L'д'}, {';', L'ж'}, {'\'', L'э'}, {'z', L'я'}, {'x', L'ч'},
            {'c', L'с'}, {'v', L'м'}, {'b', L'и'}, {'n', L'т'}, {'m', L'ь'},
            {',', L'б'}, {'.', L'ю'}, {'/', L'.'}, {' ', L' '},
            {'Q', L'Й'}, {'W', L'Ц'}, {'E', L'У'}, {'R', L'К'}, {'T', L'Е'},
            {'Y', L'Н'}, {'U', L'Г'}, {'I', L'Ш'}, {'O', L'Щ'}, {'P', L'З'},
            {'{', L'Х'}, {'}', L'Ъ'}, {'A', L'Ф'}, {'S', L'Ы'}, {'D', L'В'},
            {'F', L'А'}, {'G', L'П'}, {'H', L'Р'}, {'J', L'О'}, {'K', L'Л'},
            {'L', L'Д'}, {':', L'Ж'}, {'"', L'Э'}, {'Z', L'Я'}, {'X', L'Ч'},
            {'C', L'С'}, {'V', L'М'}, {'B', L'И'}, {'N', L'Т'}, {'M', L'Ь'},
            {'<', L'Б'}, {'>', L'Ю'}, {'?', L','},
            // Shift + number/misc keys
            {'`', L'ё'}, {'~', L'Ё'}, {'@', L'"'}, {'#', L'№'}, {'$', L';'},
            {'^', L':'}, {'&', L'?'}, {'|', L'/'}
        };
        
        std::wstring result;
        for (wchar_t c : text) {
            auto it = map.find(c);
            result += (it != map.end()) ? it->second : c;
        }
        return result;
    }
    
    std::wstring ru_to_en(const std::wstring& text) const {
        static const std::unordered_map<wchar_t, wchar_t> map = {
            {L'й', 'q'}, {L'ц', 'w'}, {L'у', 'e'}, {L'к', 'r'}, {L'е', 't'},
            {L'н', 'y'}, {L'г', 'u'}, {L'ш', 'i'}, {L'щ', 'o'}, {L'з', 'p'},
            {L'х', '['}, {L'ъ', ']'}, {L'ф', 'a'}, {L'ы', 's'}, {L'в', 'd'},
            {L'а', 'f'}, {L'п', 'g'}, {L'р', 'h'}, {L'о', 'j'}, {L'л', 'k'},
            {L'д', 'l'}, {L'ж', ';'}, {L'э', '\''}, {L'я', 'z'}, {L'ч', 'x'},
            {L'с', 'c'}, {L'м', 'v'}, {L'и', 'b'}, {L'т', 'n'}, {L'ь', 'm'},
            {L'б', ','}, {L'ю', '.'}, {L'.', '/'}, {L' ', ' '},
            {L'Й', 'Q'}, {L'Ц', 'W'}, {L'У', 'E'}, {L'К', 'R'}, {L'Е', 'T'},
            {L'Н', 'Y'}, {L'Г', 'U'}, {L'Ш', 'I'}, {L'Щ', 'O'}, {L'З', 'P'},
            {L'Х', '{'}, {L'Ъ', '}'}, {L'Ф', 'A'}, {L'Ы', 'S'}, {L'В', 'D'},
            {L'А', 'F'}, {L'П', 'G'}, {L'Р', 'H'}, {L'О', 'J'}, {L'Л', 'K'},
            {L'Д', 'L'}, {L'Ж', ':'}, {L'Э', '"'}, {L'Я', 'Z'}, {L'Ч', 'X'},
            {L'С', 'C'}, {L'М', 'V'}, {L'И', 'B'}, {L'Т', 'N'}, {L'Ь', 'M'},
            {L'Б', '<'}, {L'Ю', '>'}, {L',', '?'},
            // Shift + number/misc keys
            {L'ё', '`'}, {L'Ё', '~'}, {L'"', '@'}, {L'№', '#'}, {L';', '$'},
            {L':', '^'}, {L'?', '&'}, {L'/', '|'}
        };
        
        std::wstring result;
        for (wchar_t c : text) {
            auto it = map.find(c);
            result += (it != map.end()) ? it->second : c;
        }
        return result;
    }
};

// ============== Global Variables ==============
HHOOK g_hKeyboardHook = NULL;
HWND g_hTrayWnd = NULL;

// ============== Send Keys ==============
void SendKey(WORD vk, bool down) {
    INPUT input = {};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = vk;
    if (!down) input.ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(1, &input, sizeof(INPUT));
}

void ReleaseAllModifiers() {
    BYTE keyState[256];
    if (GetKeyboardState(keyState)) {
        if (keyState[VK_CONTROL] & 0x80) SendKey(VK_CONTROL, false);
        if (keyState[VK_SHIFT] & 0x80) SendKey(VK_SHIFT, false);
        if (keyState[VK_MENU] & 0x80) SendKey(VK_MENU, false);
        if (keyState[VK_LWIN] & 0x80) SendKey(VK_LWIN, false);
        if (keyState[VK_RWIN] & 0x80) SendKey(VK_RWIN, false);
    }
}

void SendCtrlC() {
    SendKey(VK_CONTROL, true);
    SendKey('C', true);
    SendKey('C', false);
    SendKey(VK_CONTROL, false);
}

void SendCtrlV() {
    SendKey(VK_CONTROL, true);
    SendKey('V', true);
    SendKey('V', false);
    SendKey(VK_CONTROL, false);
}

HKL GetLayoutHKL(bool toRussian) {
    std::wstring layoutId = toRussian ? L"00000419" : L"00000409";
    HKL hkl = LoadKeyboardLayoutW(layoutId.c_str(), KLF_ACTIVATE);
    if (!hkl) {
        int layoutsCount = GetKeyboardLayoutList(0, NULL);
        if (layoutsCount > 0) {
            std::vector<HKL> layouts(layoutsCount);
            GetKeyboardLayoutList(layoutsCount, layouts.data());
            for (HKL l : layouts) {
                WORD langId = LOWORD((DWORD_PTR)l);
                if (toRussian && langId == 0x0419) return l;
                if (!toRussian && langId == 0x0409) return l;
            }
        }
    }
    return hkl;
}

// ============== Convert ==============
void ConvertFromClipboard() {
    ReleaseAllModifiers();
    Sleep(50);
    
    IDataObject* pBackup = nullptr;
    HRESULT hrBackup = E_FAIL;
    if (g_settings.restoreClipboard) {
        hrBackup = OleGetClipboard(&pBackup);
    }
    
    SendCtrlC();
    Sleep(200);
    
    std::wstring text;
    if (OpenClipboard(NULL)) {
        HGLOBAL hData = GetClipboardData(CF_UNICODETEXT);
        if (hData) {
            wchar_t* pData = (wchar_t*)GlobalLock(hData);
            if (pData && *pData) text = pData;
            if (pData) GlobalUnlock(hData);
        }
        CloseClipboard();
    }
    
    if (text.empty()) {
        if (SUCCEEDED(hrBackup) && pBackup) pBackup->Release();
        return;
    }
    
    bool isToRussian = false;
    std::wstring converted = LayoutConverter::instance().convert(text, isToRussian);
    if (converted == text) {
        if (SUCCEEDED(hrBackup) && pBackup) pBackup->Release();
        return;
    }
    
    if (OpenClipboard(NULL)) {
        EmptyClipboard();
        HGLOBAL hNew = GlobalAlloc(GMEM_MOVEABLE, (converted.length() + 1) * sizeof(wchar_t));
        if (hNew) {
            wchar_t* pNew = (wchar_t*)GlobalLock(hNew);
            if (pNew) {
                wcscpy(pNew, converted.c_str());
                GlobalUnlock(pNew);
                SetClipboardData(CF_UNICODETEXT, hNew);
            }
        }
        CloseClipboard();
    }
    
    Sleep(50);
    SendCtrlV();
    Sleep(100);
    
    if (g_settings.switchLayout) {
        HWND activeWnd = GetForegroundWindow();
        if (activeWnd) {
            DWORD threadId = GetWindowThreadProcessId(activeWnd, NULL);
            GUITHREADINFO gti = { sizeof(GUITHREADINFO) };
            HWND targetWnd = activeWnd;
            if (GetGUIThreadInfo(threadId, &gti) && gti.hwndFocus) {
                targetWnd = gti.hwndFocus;
            }
            
            HKL targetHKL = GetLayoutHKL(isToRussian);
            if (targetHKL) {
                PostMessageW(targetWnd, WM_INPUTLANGCHANGEREQUEST, 1, (LPARAM)targetHKL);
                if (targetWnd != activeWnd) {
                    PostMessageW(activeWnd, WM_INPUTLANGCHANGEREQUEST, 1, (LPARAM)targetHKL);
                }
            }
        }
    }
    
    if (g_settings.playSound) {
        MessageBeep(MB_ICONINFORMATION);
    }
    
    if (SUCCEEDED(hrBackup) && pBackup) {
        Sleep(200); 
        OleSetClipboard(pBackup);
        OleFlushClipboard();
        pBackup->Release();
    }
}

// ============== Tray Functions ==============
void ToggleAutostart();

LRESULT CALLBACK TrayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_RELOAD_SETTINGS:
            LoadSettings();
            return 0;
            
        case WM_USER + 1: // Tray icon message
            if (lParam == WM_RBUTTONUP) {
                HMENU hMenu = CreatePopupMenu();
                
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
                
                AppendMenuW(hMenu, MF_STRING, 1, L"Конвертировать выделенное");
                AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
                AppendMenuW(hMenu, MF_STRING, 2, L"Показать настройки");
                AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
                AppendMenuW(hMenu, autostart ? MF_CHECKED : MF_UNCHECKED, 4, L"Запускать при старте Windows");
                AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
                AppendMenuW(hMenu, MF_STRING, 3, L"Выход");
                
                POINT pt;
                GetCursorPos(&pt);
                SetForegroundWindow(hwnd);
                TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
                DestroyMenu(hMenu);
            } else if (lParam == WM_LBUTTONDBLCLK) {
                CreateThread(NULL, 0, [](LPVOID)->DWORD {
                    OleInitialize(NULL);
                    ConvertFromClipboard();
                    OleUninitialize();
                    return 0;
                }, NULL, 0, NULL);
            }
            return 0;
            
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case 1: // Convert
                    CreateThread(NULL, 0, [](LPVOID)->DWORD {
                        OleInitialize(NULL);
                        ConvertFromClipboard();
                        OleUninitialize();
                        return 0;
                    }, NULL, 0, NULL);
                    break;
                case 2: // Show settings
                    ShellExecuteW(NULL, L"open", L"RokeSettings.exe", NULL, NULL, SW_SHOWNORMAL);
                    break;
                case 3: // Exit
                    PostQuitMessage(0);
                    break;
                case 4: // Toggle autostart
                    ToggleAutostart();
                    break;
            }
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void ToggleAutostart() {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_READ | KEY_WRITE, &hKey) != ERROR_SUCCESS) {
        return;
    }
    
    BYTE value[512];
    DWORD size = sizeof(value);
    
    if (RegQueryValueExW(hKey, L"Roke", NULL, NULL, value, &size) == ERROR_SUCCESS) {
        RegDeleteValueW(hKey, L"Roke");
    } else {
        wchar_t exePath[512];
        GetModuleFileNameW(NULL, exePath, 512);
        RegSetValueExW(hKey, L"Roke", 0, REG_SZ, (const BYTE*)exePath, (DWORD)(wcslen(exePath) + 1) * sizeof(wchar_t));
    }
    
    RegCloseKey(hKey);
}

void CreateTrayIcon(HWND hwnd) {
    g_hTrayWnd = CreateWindowExW(0, L"RokeTrayClass", L"Roke Tray",
        WS_OVERLAPPEDWINDOW, 0, 0, 0, 0, NULL, NULL, 
        (HINSTANCE)GetModuleHandle(NULL), NULL);
    
    HICON hIcon = LoadIconW((HINSTANCE)GetModuleHandle(NULL), MAKEINTRESOURCEW(IDI_ICON));
    if (!hIcon) {
        hIcon = LoadIcon(NULL, IDI_APPLICATION);
    }
    
    NOTIFYICONDATAW nid = {};
    nid.cbSize = sizeof(NOTIFYICONDATAW);
    nid.hWnd = g_hTrayWnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_USER + 1;
    nid.hIcon = hIcon;
    wcsncpy(nid.szTip, L"Roke - Конвертер раскладки", 127);
    nid.szTip[127] = L'\0';
    
    Shell_NotifyIconW(NIM_ADD, &nid);
}

void DestroyTrayIcon() {
    NOTIFYICONDATAW nid = {};
    nid.cbSize = sizeof(NOTIFYICONDATAW);
    nid.hWnd = g_hTrayWnd;
    nid.uID = 1;
    Shell_NotifyIconW(NIM_DELETE, &nid);
    
    if (g_hTrayWnd) DestroyWindow(g_hTrayWnd);
}

// ============== Keyboard Hook ==============
LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    static bool waitingForRelease = false;
    
    if (nCode == HC_ACTION) {
        KBDLLHOOKSTRUCT* p = (KBDLLHOOKSTRUCT*)lParam;
        bool keyUp = (p->flags & LLKHF_UP) != 0;
        
        if (!keyUp) {
            bool ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
            bool alt = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
            bool shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
            
            bool triggered = false;
            if (g_settings.hotkeyType == 0) { // Ctrl + Shift + Q
                triggered = ctrl && shift && !alt && p->vkCode == 'Q';
            } else if (g_settings.hotkeyType == 1) { // Ctrl + Alt + Q
                triggered = ctrl && alt && !shift && p->vkCode == 'Q';
            } else if (g_settings.hotkeyType == 2) { // F12
                triggered = !ctrl && !alt && !shift && p->vkCode == VK_F12;
            }
            
            if (triggered) {
                waitingForRelease = true;
                return 1;
            }
        }
        
        if (keyUp && waitingForRelease) {
            waitingForRelease = false;
            CreateThread(NULL, 0, [](LPVOID)->DWORD {
                OleInitialize(NULL);
                ConvertFromClipboard();
                OleUninitialize();
                return 0;
            }, NULL, 0, NULL);
            return 1;
        }
    }
    
    return CallNextHookEx(g_hKeyboardHook, nCode, wParam, lParam);
}

// ============== Entry Point ==============
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    LoadSettings();
    
    OleInitialize(NULL);
    
    WNDCLASSEXW trayClass = {};
    trayClass.cbSize = sizeof(WNDCLASSEXW);
    trayClass.lpfnWndProc = TrayWndProc;
    trayClass.hInstance = hInstance;
    trayClass.lpszClassName = L"RokeTrayClass";
    RegisterClassExW(&trayClass);
    
    CreateTrayIcon(NULL);
    
    g_hKeyboardHook = SetWindowsHookExW(WH_KEYBOARD_LL, KeyboardHookProc, GetModuleHandle(NULL), 0);
    
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    
    if (g_hKeyboardHook) UnhookWindowsHookEx(g_hKeyboardHook);
    DestroyTrayIcon();
    
    OleUninitialize();
    
    return 0;
}
