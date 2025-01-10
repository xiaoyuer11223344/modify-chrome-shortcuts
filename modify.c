#include <windows.h>
#include <shobjidl.h>
#include <objbase.h>
#include <objidl.h>
#include <shlguid.h>
#include <stdbool.h>

// Function to check if file exists
bool FileExists(const wchar_t* path) {
    DWORD dwAttrib = GetFileAttributesW(path);
    return (dwAttrib != INVALID_FILE_ATTRIBUTES && 
            !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

const wchar_t* SHORTCUT_PATHS[] = {
    L"C:\\ProgramData\\Microsoft\\Windows\\Start Menu\\Programs\\Google Chrome.lnk",
    L"C:\\Users\\All Users\\Microsoft\\Windows\\Start Menu\\Programs\\Google Chrome.lnk",
    L"C:\\Users\\join\\AppData\\Roaming\\Microsoft\\Internet Explorer\\Quick Launch\\Google Chrome.lnk",
    L"C:\\Users\\Public\\Desktop\\Google Chrome.lnk"
};

#define NUM_SHORTCUTS (sizeof(SHORTCUT_PATHS) / sizeof(SHORTCUT_PATHS[0]))

HRESULT ModifyShortcut(const wchar_t* shortcutPath) {
    HRESULT hres;
    IShellLink* psl = NULL;
    IPersistFile* ppf = NULL;
    wchar_t szTargetPath[MAX_PATH];
    wchar_t szArgs[MAX_PATH];
    
    // Initialize COM
    hres = CoInitialize(NULL);
    if (FAILED(hres)) {
        wprintf(L"Failed to initialize COM library. Error: 0x%08lx\n", hres);
        return hres;
    }
    
    // Create IShellLink instance
    hres = CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                           &IID_IShellLink, (void**)&psl);
    if (FAILED(hres)) {
        wprintf(L"Failed to create IShellLink instance. Error: 0x%08lx\n", hres);
        CoUninitialize();
        return hres;
    }
    
    // Get IPersistFile interface
    hres = psl->lpVtbl->QueryInterface(psl, &IID_IPersistFile, (void**)&ppf);
    if (FAILED(hres)) {
        wprintf(L"Failed to get IPersistFile interface. Error: 0x%08lx\n", hres);
        psl->lpVtbl->Release(psl);
        CoUninitialize();
        return hres;
    }
    
    // Load the shortcut
    hres = ppf->lpVtbl->Load(ppf, shortcutPath, STGM_READ | STGM_WRITE);
    if (FAILED(hres)) {
        wprintf(L"Failed to load shortcut: %ls. Error: 0x%08lx\n", shortcutPath, hres);
        ppf->lpVtbl->Release(ppf);
        psl->lpVtbl->Release(psl);
        CoUninitialize();
        return hres;
    }
    
    // Get current target path and arguments
    hres = psl->lpVtbl->GetPath(psl, szTargetPath, MAX_PATH, NULL, 0);
    if (SUCCEEDED(hres)) {
        hres = psl->lpVtbl->GetArguments(psl, szArgs, MAX_PATH);
        if (SUCCEEDED(hres)) {
            // Check if remote debugging argument already exists
            if (wcsstr(szArgs, L"--remote-debugging-port=9222") == NULL) {
                wchar_t newArgs[MAX_PATH];
                if (wcslen(szArgs) > 0) {
                    swprintf(newArgs, MAX_PATH, L"%ls --remote-debugging-port=9222", szArgs);
                } else {
                    wcscpy(newArgs, L"--remote-debugging-port=9222");
                }
                
                // Set new arguments
                hres = psl->lpVtbl->SetArguments(psl, newArgs);
                if (SUCCEEDED(hres)) {
                    // Save changes
                    hres = ppf->lpVtbl->Save(ppf, NULL, TRUE);
                    if (SUCCEEDED(hres)) {
                        wprintf(L"Successfully modified shortcut: %ls\n", shortcutPath);
                    } else {
                        wprintf(L"Failed to save shortcut: %ls. Error: 0x%08lx\n", shortcutPath, hres);
                    }
                } else {
                    wprintf(L"Failed to set arguments for shortcut: %ls. Error: 0x%08lx\n", shortcutPath, hres);
                }
            } else {
                wprintf(L"Remote debugging port already set for shortcut: %ls\n", shortcutPath);
            }
        }
    }
    
    // Cleanup
    ppf->lpVtbl->Release(ppf);
    psl->lpVtbl->Release(psl);
    CoUninitialize();
    
    return hres;
}

int wmain(int argc, wchar_t* argv[]) {
    HRESULT finalResult = S_OK;
    bool anyShortcutExists = false;
    
    // First check which shortcuts exist
    wprintf(L"Checking shortcut paths:\n");
    for (size_t i = 0; i < NUM_SHORTCUTS; i++) {
        if (FileExists(SHORTCUT_PATHS[i])) {
            wprintf(L"Found shortcut: %ls\n", SHORTCUT_PATHS[i]);
            anyShortcutExists = true;
        } else {
            wprintf(L"Shortcut not found: %ls\n", SHORTCUT_PATHS[i]);
        }
    }
    
    if (!anyShortcutExists) {
        wprintf(L"No Chrome shortcuts found in any of the specified locations.\n");
        return 1;
    }
    
    // Proceed with modifying only existing shortcuts
    wprintf(L"\nModifying existing shortcuts:\n");
    for (size_t i = 0; i < NUM_SHORTCUTS; i++) {
        if (FileExists(SHORTCUT_PATHS[i])) {
            HRESULT hres = ModifyShortcut(SHORTCUT_PATHS[i]);
            if (FAILED(hres)) {
                finalResult = hres;
            }
        }
    }
    
    if (FAILED(finalResult)) {
        wprintf(L"\nSome shortcuts could not be modified. Please run as Administrator.\n");
        return 1;
    }
    
    wprintf(L"\nOperation completed successfully.\n");
    return 0;
}

