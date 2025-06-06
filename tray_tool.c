#include <windows.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <stdio.h>

#define WM_TRAYICON (WM_USER + 1)
#define TRAY_ICON_ID 1001
#define TRAY_MENU_ID_BASE 2000
#define MAX_MENU_ITEMS 1000
#define TOOL_FOLDER "C:\\Tools"  // <-- Change to your folder path

NOTIFYICONDATA nid;
HMENU hTrayMenu;
char *batFiles[MAX_MENU_ITEMS];
int fileCount = 0;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

void AddTrayIcon(HWND hwnd) {
    memset(&nid, 0, sizeof(nid));
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwnd;
    nid.uID = TRAY_ICON_ID;
    nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    strcpy(nid.szTip, "Batch Tool Menu");
    Shell_NotifyIcon(NIM_ADD, &nid);
}

void RemoveTrayIcon() {
    Shell_NotifyIcon(NIM_DELETE, &nid);
}

void ExecuteBatch(const char *filepath) {
    char command[1024];
    snprintf(command, sizeof(command), "cmd.exe /c \"%s\"", filepath);
    ShellExecute(NULL, "open", "cmd.exe", command + 9, NULL, SW_HIDE);
}

void FreeBatFiles() {
    for (int i = 0; i < fileCount; i++) {
        free(batFiles[i]);
    }
    fileCount = 0;
}

void AddBatchFilesToMenu(HMENU parentMenu, const char *folderPath) {
    WIN32_FIND_DATA findFileData;
    char searchPath[MAX_PATH];
    snprintf(searchPath, MAX_PATH, "%s\\*", folderPath);

    HANDLE hFind = FindFirstFile(searchPath, &findFileData);
    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        if (strcmp(findFileData.cFileName, ".") == 0 || strcmp(findFileData.cFileName, "..") == 0)
            continue;

        char fullPath[MAX_PATH];
        snprintf(fullPath, MAX_PATH, "%s\\%s", folderPath, findFileData.cFileName);

        if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            HMENU subMenu = CreatePopupMenu();
            AppendMenu(parentMenu, MF_POPUP, (UINT_PTR)subMenu, findFileData.cFileName);
            AddBatchFilesToMenu(subMenu, fullPath);  // recurse
        } else {
            char *ext = PathFindExtension(findFileData.cFileName);
            if (_stricmp(ext, ".bat") == 0 || _stricmp(ext, ".cmd") == 0) {
                if (fileCount < MAX_MENU_ITEMS) {
                    batFiles[fileCount] = _strdup(fullPath);
                    AppendMenu(parentMenu, MF_STRING, TRAY_MENU_ID_BASE + fileCount, findFileData.cFileName);
                    fileCount++;
                }
            }
        }
    } while (FindNextFile(hFind, &findFileData));

    FindClose(hFind);
}


void ShowContextMenu(HWND hwnd) {
    POINT pt;
    GetCursorPos(&pt);
    FreeBatFiles();
    hTrayMenu = CreatePopupMenu();
    AddBatchFilesToMenu(hTrayMenu, TOOL_FOLDER);
    AppendMenu(hTrayMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hTrayMenu, MF_STRING, 9999, "Exit");

    SetForegroundWindow(hwnd);
    TrackPopupMenu(hTrayMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
    DestroyMenu(hTrayMenu);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    const char CLASS_NAME[] = "TrayAppWindow";
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(0, CLASS_NAME, "Tray App", 0,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL, NULL, hInstance, NULL);

    AddTrayIcon(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    RemoveTrayIcon();
    FreeBatFiles();
    return 0;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_TRAYICON) {
        if (lParam == WM_RBUTTONUP) {
            ShowContextMenu(hwnd);
        }
    } else if (msg == WM_COMMAND) {
        int id = LOWORD(wParam);
        if (id >= TRAY_MENU_ID_BASE && id < TRAY_MENU_ID_BASE + fileCount) {
            ExecuteBatch(batFiles[id - TRAY_MENU_ID_BASE]);
        } else if (id == 9999) {
            PostQuitMessage(0);
        }
    } else if (msg == WM_DESTROY) {
        PostQuitMessage(0);
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}
