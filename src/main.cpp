/*
 * Roke - Keyboard Layout Converter
 * Скрывается в трей, взаимодействие через иконку
 */

#include <windows.h>
#include <string>
#include <unordered_map>
#include <shellapi.h>
#include <objbase.h>
#include <dwmapi.h>
#include <uxtheme.h>
#include <vector>
#include "resource.h"

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "uxtheme.lib")

// SetPreferredAppMode Ordinal function definition for dark context menus
typedef int (WINAPI* pfnSetPreferredAppMode)(int appMode);

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

void SaveSettings() {
    HKEY hKey;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Roke", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        DWORD val = g_settings.restoreClipboard ? 1 : 0;
        RegSetValueExW(hKey, L"RestoreClipboard", 0, REG_DWORD, (const BYTE*)&val, sizeof(DWORD));
        
        val = g_settings.switchLayout ? 1 : 0;
        RegSetValueExW(hKey, L"SwitchLayout", 0, REG_DWORD, (const BYTE*)&val, sizeof(DWORD));
        
        val = g_settings.playSound ? 1 : 0;
        RegSetValueExW(hKey, L"PlaySound", 0, REG_DWORD, (const BYTE*)&val, sizeof(DWORD));
        
        val = static_cast<DWORD>(g_settings.hotkeyType);
        RegSetValueExW(hKey, L"HotkeyType", 0, REG_DWORD, (const BYTE*)&val, sizeof(DWORD));
        
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
HWND g_hMainWnd = NULL;
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
    // Release physical modifier interference
    ReleaseAllModifiers();
    Sleep(50);
    
    // Backup clipboard via OLE
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
    
    // Switch keyboard layout if requested
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
    
    // Play sound if requested
    if (g_settings.playSound) {
        MessageBeep(MB_ICONINFORMATION);
    }
    
    // Restore original clipboard contents
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
        case WM_USER + 1: // Tray icon message
            if (lParam == WM_RBUTTONUP) {
                HMENU hMenu = CreatePopupMenu();
                
                // Check if autostart is enabled
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
                // Background thread with correct OLE lifetime
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
                    ShowWindow(g_hMainWnd, SW_SHOW);
                    SetForegroundWindow(g_hMainWnd);
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
        WS_OVERLAPPEDWINDOW, 0, 0, 0, 0, HWND_MESSAGE, NULL, 
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

// ============== GUI Double-Buffered Painting ==============
void DrawSettingsUI(HWND hwnd, HDC hdc) {
    RECT rc;
    GetClientRect(hwnd, &rc);
    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;

    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBM = CreateCompatibleBitmap(hdc, width, height);
    HBITMAP oldBM = (HBITMAP)SelectObject(memDC, memBM);

    // Background: Genuine Win11 Slate Dark #1C1C1E
    HBRUSH bgBrush = CreateSolidBrush(RGB(28, 28, 30)); 
    FillRect(memDC, &rc, bgBrush);
    DeleteObject(bgBrush);

    // Fonts
    HFONT hFontLogo = CreateFontW(28, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    HFONT hFontSub = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    HFONT hFontText = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    HFONT hFontHeader = CreateFontW(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

    // Title & Subtitle drawing with transparent backgrounds
    SetBkMode(memDC, TRANSPARENT);
    
    // Logo
    SelectObject(memDC, hFontLogo);
    SetTextColor(memDC, RGB(96, 205, 255)); // Fluent Blue #60CDFF
    TextOutW(memDC, 24, 18, L"ROKE", 4);

    // Subtitle
    SelectObject(memDC, hFontSub);
    SetTextColor(memDC, RGB(160, 160, 160)); 
    TextOutW(memDC, 110, 28, L"умный конвертер раскладки", 25);

    // Divider Line (Fluent Dark #2C2C2E)
    HPEN dividerPen = CreatePen(PS_SOLID, 1, RGB(44, 44, 46)); 
    HPEN oldPen = (HPEN)SelectObject(memDC, dividerPen);
    MoveToEx(memDC, 24, 60, NULL);
    LineTo(memDC, width - 24, 60);

    POINT ptCursor;
    GetCursorPos(&ptCursor);
    ScreenToClient(hwnd, &ptCursor);

    // Elegant Win11 Toggle Switch Row Drawer
    auto DrawSwitchRow = [&](const std::wstring& label, int y, bool value, bool hover) {
        SelectObject(memDC, hFontText);
        SetBkMode(memDC, TRANSPARENT);
        SetTextColor(memDC, hover ? RGB(255, 255, 255) : RGB(224, 224, 224));
        TextOutW(memDC, 24, y + 2, label.c_str(), static_cast<int>(label.length()));

        HBRUSH pillBrush;
        HPEN borderPen;
        HBRUSH knobBrush;
        
        if (value) {
            // ON State: Fluent Accent Blue (#0078D4 normal, #60CDFF hover), no border
            COLORREF blueBg = hover ? RGB(96, 205, 255) : RGB(0, 120, 212);
            pillBrush = CreateSolidBrush(blueBg);
            borderPen = CreatePen(PS_SOLID, 1, blueBg);
            knobBrush = CreateSolidBrush(RGB(255, 255, 255)); // White knob
        } else {
            // OFF State: transparent/dark background, gray outline (#9F9F9F normal, #FFFFFF hover)
            COLORREF borderCol = hover ? RGB(255, 255, 255) : RGB(159, 159, 159);
            pillBrush = CreateSolidBrush(hover ? RGB(45, 45, 48) : RGB(28, 28, 30));
            borderPen = CreatePen(PS_SOLID, 1, borderCol);
            knobBrush = CreateSolidBrush(borderCol); // Knob matches border color
        }
        
        HPEN oldBorder = (HPEN)SelectObject(memDC, borderPen);
        HBRUSH oldPill = (HBRUSH)SelectObject(memDC, pillBrush);

        // Draw pill: 40px wide, 20px high, fully rounded
        RoundRect(memDC, 390, y + 2, 430, y + 22, 20, 20);

        SelectObject(memDC, oldBorder);
        DeleteObject(borderPen);
        SelectObject(memDC, oldPill);
        DeleteObject(pillBrush);

        // Draw circular knob: 12px diameter
        HBRUSH oldKnob = (HBRUSH)SelectObject(memDC, knobBrush);
        int knobX = value ? 414 : 394;
        Ellipse(memDC, knobX, y + 6, knobX + 12, y + 18);
        
        SelectObject(memDC, oldKnob);
        DeleteObject(knobBrush);
    };

    DrawSwitchRow(L"Восстанавливать буфер обмена", 80, g_settings.restoreClipboard, (ptCursor.x >= 24 && ptCursor.x <= 428 && ptCursor.y >= 75 && ptCursor.y <= 105));
    DrawSwitchRow(L"Переключать раскладку системы", 125, g_settings.switchLayout, (ptCursor.x >= 24 && ptCursor.x <= 428 && ptCursor.y >= 120 && ptCursor.y <= 150));
    DrawSwitchRow(L"Звук при успешной конвертации", 170, g_settings.playSound, (ptCursor.x >= 24 && ptCursor.x <= 428 && ptCursor.y >= 165 && ptCursor.y <= 195));

    // Bottom Divider
    MoveToEx(memDC, 24, 210, NULL);
    LineTo(memDC, width - 24, 210);

    // Hotkey header
    SelectObject(memDC, hFontHeader);
    SetBkMode(memDC, TRANSPARENT);
    SetTextColor(memDC, RGB(255, 255, 255));
    TextOutW(memDC, 24, 225, L"Сочетание клавиш активации:", 27);

    // Modern Button Tabs
    auto DrawHotkeyButton = [&](const std::wstring& text, int x, int y, int w, int h, bool selected, bool hover) {
        HBRUSH btnBrush;
        HPEN btnPen;
        
        if (selected) {
            btnBrush = CreateSolidBrush(RGB(38, 38, 40)); 
            btnPen = CreatePen(PS_SOLID, 2, RGB(96, 205, 255)); 
        } else {
            btnBrush = CreateSolidBrush(hover ? RGB(44, 44, 46) : RGB(34, 34, 36)); 
            btnPen = CreatePen(PS_SOLID, 1, hover ? RGB(80, 80, 82) : RGB(50, 50, 52)); 
        }
        
        HPEN oldP = (HPEN)SelectObject(memDC, btnPen);
        HBRUSH oldB = (HBRUSH)SelectObject(memDC, btnBrush);

        RoundRect(memDC, x, y, x + w, y + h, 8, 8); 

        SelectObject(memDC, oldP);
        SelectObject(memDC, oldB);
        DeleteObject(btnPen);
        DeleteObject(btnBrush);

        if (selected) {
            HBRUSH accentBarBrush = CreateSolidBrush(RGB(96, 205, 255));
            RECT barRect = { x + w / 4, y + h - 3, x + 3 * w / 4, y + h - 1 };
            FillRect(memDC, &barRect, accentBarBrush);
            DeleteObject(accentBarBrush);
        }

        SelectObject(memDC, hFontSub);
        SetBkMode(memDC, TRANSPARENT);
        SetTextColor(memDC, selected ? RGB(96, 205, 255) : (hover ? RGB(255, 255, 255) : RGB(180, 180, 180))); 
        
        SIZE textSize;
        GetTextExtentPoint32W(memDC, text.c_str(), static_cast<int>(text.length()), &textSize);
        TextOutW(memDC, x + (w - textSize.cx) / 2, y + (h - textSize.cy) / 2, text.c_str(), static_cast<int>(text.length()));
    };

    DrawHotkeyButton(L"Ctrl + Shift + Q", 24, 255, 134, 38, g_settings.hotkeyType == 0, (ptCursor.x >= 24 && ptCursor.x <= 158 && ptCursor.y >= 255 && ptCursor.y <= 293));
    DrawHotkeyButton(L"Ctrl + Alt + Q", 170, 255, 134, 38, g_settings.hotkeyType == 1, (ptCursor.x >= 170 && ptCursor.x <= 304 && ptCursor.y >= 255 && ptCursor.y <= 293));
    DrawHotkeyButton(L"F12", 316, 255, 134, 38, g_settings.hotkeyType == 2, (ptCursor.x >= 316 && ptCursor.x <= 450 && ptCursor.y >= 255 && ptCursor.y <= 293));

    SelectObject(memDC, oldPen);
    DeleteObject(dividerPen);

    DeleteObject(hFontLogo);
    DeleteObject(hFontSub);
    DeleteObject(hFontText);
    DeleteObject(hFontHeader);

    BitBlt(hdc, 0, 0, width, height, memDC, 0, 0, SRCCOPY);

    SelectObject(memDC, oldBM);
    DeleteObject(memBM);
    DeleteDC(memDC);
}

// ============== Main Window Proc ==============
LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            DrawSettingsUI(hwnd, hdc);
            EndPaint(hwnd, &ps);
            return 0;
        }
        
        case WM_ERASEBKGND:
            return 1; // Prevent flicker

        case WM_SETCURSOR: {
            POINT pt;
            GetCursorPos(&pt);
            ScreenToClient(hwnd, &pt);
            
            bool hover = (pt.x >= 24 && pt.x <= 450 && pt.y >= 75 && pt.y <= 105) || 
                         (pt.x >= 24 && pt.x <= 450 && pt.y >= 120 && pt.y <= 150) || 
                         (pt.x >= 24 && pt.x <= 450 && pt.y >= 165 && pt.y <= 195) || 
                         (pt.x >= 24 && pt.x <= 158 && pt.y >= 255 && pt.y <= 293) || 
                         (pt.x >= 170 && pt.x <= 304 && pt.y >= 255 && pt.y <= 293) || 
                         (pt.x >= 316 && pt.x <= 450 && pt.y >= 255 && pt.y <= 293);  
            
            if (hover) {
                SetCursor(LoadCursor(NULL, IDC_HAND));
                return TRUE;
            }
            break;
        }

        case WM_MOUSEMOVE:
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;

        case WM_LBUTTONDOWN: {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);
            bool changed = false;
            
            if (x >= 24 && x <= 450 && y >= 75 && y <= 105) {
                g_settings.restoreClipboard = !g_settings.restoreClipboard;
                changed = true;
            } else if (x >= 24 && x <= 450 && y >= 120 && y <= 150) {
                g_settings.switchLayout = !g_settings.switchLayout;
                changed = true;
            } else if (x >= 24 && x <= 450 && y >= 165 && y <= 195) {
                g_settings.playSound = !g_settings.playSound;
                changed = true;
            } else if (x >= 24 && x <= 158 && y >= 255 && y <= 293) {
                g_settings.hotkeyType = 0;
                changed = true;
            } else if (x >= 170 && x <= 304 && y >= 255 && y <= 293) {
                g_settings.hotkeyType = 1;
                changed = true;
            } else if (x >= 316 && x <= 450 && y >= 255 && y <= 293) {
                g_settings.hotkeyType = 2;
                changed = true;
            }
            
            if (changed) {
                SaveSettings();
                InvalidateRect(hwnd, NULL, FALSE);
            }
            return 0;
        }

        case WM_CLOSE:
            ShowWindow(hwnd, SW_HIDE);
            return 0;
            
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// ============== Entry Point ==============
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Load persisted settings
    LoadSettings();
    
    // Enable Native Windows Dark Mode Context Menus safely via GetModuleHandle
    HMODULE hUxtheme = GetModuleHandleW(L"uxtheme.dll");
    if (!hUxtheme) {
        hUxtheme = LoadLibraryW(L"uxtheme.dll");
    }
    if (hUxtheme) {
        pfnSetPreferredAppMode SetPreferredAppMode = (pfnSetPreferredAppMode)GetProcAddress(hUxtheme, MAKEINTRESOURCEA(135));
        if (SetPreferredAppMode) {
            SetPreferredAppMode(2); // 2 = Force Dark
        }
    }
    
    // Initialize OLE for advanced clipboard backup/restore
    OleInitialize(NULL);
    
    // Register tray class
    WNDCLASSEXW trayClass = {};
    trayClass.cbSize = sizeof(WNDCLASSEXW);
    trayClass.lpfnWndProc = TrayWndProc;
    trayClass.hInstance = hInstance;
    trayClass.lpszClassName = L"RokeTrayClass";
    RegisterClassExW(&trayClass);
    
    // Register main window class
    WNDCLASSEXW mainClass = {};
    mainClass.cbSize = sizeof(WNDCLASSEXW);
    mainClass.lpfnWndProc = MainWndProc;
    mainClass.hInstance = hInstance;
    mainClass.lpszClassName = L"RokeMainClass";
    mainClass.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_ICON));
    if (!mainClass.hIcon) {
        mainClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    }
    mainClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassExW(&mainClass);
    
    // Calculate exact size for 480x320 client area
    RECT rc = { 0, 0, 480, 320 };
    AdjustWindowRectEx(&rc, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, FALSE, 0);
    
    // Create main window (hidden by default) centered
    HWND hwnd = CreateWindowExW(0, L"RokeMainClass", L"Roke - Настройки",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        (GetSystemMetrics(SM_CXSCREEN) - (rc.right - rc.left)) / 2, 
        (GetSystemMetrics(SM_CYSCREEN) - (rc.bottom - rc.top)) / 2, 
        rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hInstance, NULL);
    g_hMainWnd = hwnd;
    
    if (hwnd == NULL) {
        OleUninitialize();
        return 1;
    }
    
    // Force Immersive Dark Mode for Titlebar and window controls (minimize/maximize/close)
    BOOL dark = TRUE;
    DwmSetWindowAttribute(hwnd, 20, &dark, sizeof(dark)); // DWMWA_USE_IMMERSIVE_DARK_MODE
    
    CreateTrayIcon(hwnd);
    
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
