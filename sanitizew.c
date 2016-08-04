// sanitize file and folder names by stripping unwanted characters
// won't touch any files/folders starting with .
// v2.0 by Antoni Sawicki <as@tenoware.com>
//
// Configuration:
// alow alpha, numeric, underscore and dot
// will also convert spaces to underscore
#define STRIPCFG  L"anu_."


#include <windows.h>
#include <wchar.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <shlwapi.h>


unsigned int files=0, folders=0, changed=0;


void error(int exit, WCHAR *msg, ...) {
    va_list valist;
    WCHAR vaBuff[1024]={L'\0'};
    WCHAR errBuff[1024]={L'\0'};
    DWORD err;

    err=GetLastError();

    va_start(valist, msg);
    _vsnwprintf_s(vaBuff, sizeof(vaBuff), sizeof(vaBuff), msg, valist);
    va_end(valist);

    wprintf(L"%s: %s\n", (exit) ? L"ERROR":L"WARNING", vaBuff);

    if (err) {
        FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), errBuff, sizeof(errBuff), NULL);
        wprintf(L"[0x%08X] %s", err, errBuff);
    }
    else {
        putchar(L'\n');
    }

    FlushFileBuffers(GetStdHandle(STD_OUTPUT_HANDLE));

    if(exit)
        ExitProcess(1);
}

int wstrip(WCHAR *str, int len, WCHAR *allow) {
    int n,a;
    int alpha=0, number=0, spctou=0;
    WCHAR *dst;

    if(!str || !wcslen(str) || !allow || !wcslen(allow))
        return -1;

    if(*allow == L'a') {
        alpha=1;
        allow++;
    }
    if(*allow == L'n') {
        number=1;
        allow++;
    }
    if(*allow == L'u') {
        spctou=1;
        allow++;
    }
    if(*allow == L'!') {
        allow++;
    }

    dst=str;

    for(n=0; n<len && *str!=L'\0'; n++, str++) {
        if(alpha && ((*str>=L'A' && *str<=L'Z') || (*str>=L'a' && *str<=L'z')))
            *(dst++)=*str;
        else if(number && (*str >= L'0' && *str <= L'9'))
            *(dst++)=*str;
        else if(spctou && *str==L' ')
            *(dst++)=L'_';
        else if(wcslen(allow))
            for(a=0; a<wcslen(allow); a++) 
                if(*str==allow[a])
                    *(dst++)=*str;
    }

    *dst=L'\0';

    return 0;
}

int recurse(WCHAR *dir) {
    WIN32_FIND_DATAW find_data;
    HANDLE hFind;
    size_t l=0;
    WCHAR new_full_name[MAX_PATH];
    WCHAR old_full_name[MAX_PATH];
    WCHAR old_file_name[MAX_PATH];
    WCHAR new_file_name[MAX_PATH];
    WCHAR newnew[MAX_PATH];
    WCHAR dirn[MAX_PATH];
    WCHAR *part;

    memset(dirn, L'\0', sizeof(dirn));
    _snwprintf(dirn, sizeof(dirn), L"%s\\*", dir);

    hFind=FindFirstFileW(dirn, &find_data);
    if(hFind==INVALID_HANDLE_VALUE)
        error(1, L"Unable to list directory: %s", dir);

    do { 
        memset(old_file_name, L'\0', sizeof(old_file_name));
        memset(old_full_name, L'\0', sizeof(old_full_name));
        memset(new_full_name, L'\0', sizeof(new_full_name));
        memset(new_file_name, L'\0', sizeof(new_file_name));
        memset(newnew, '\0', sizeof(newnew));

        wcscpy_s(old_file_name, sizeof(old_file_name), find_data.cFileName); 
        if(old_file_name[0]!=L'.') {
            wcscpy_s(new_file_name, sizeof(new_file_name), old_file_name); 
            wstrip(new_file_name, sizeof(new_file_name), STRIPCFG); 

            if(wcscmp(new_file_name, old_file_name)!=0) {
                changed++;
                _snwprintf(old_full_name, sizeof(old_full_name), L"%s\\%s", dir, old_file_name);
                _snwprintf(new_full_name, sizeof(new_full_name), L"%s\\%s", dir, new_file_name);

                if(MoveFileW(old_full_name, new_full_name)==0) {
                    error(0, L"Could not rename [%s] to [%s] will try adding '_'", old_full_name, new_full_name);
                    
                    part=wcsrchr(new_file_name, L'.');
                    if(part)
                        _snwprintf(new_full_name, sizeof(new_full_name), L"%s\\%s_%s", dir, wcsncat(newnew, new_file_name, wcslen(new_file_name)-wcslen(part)), part);
                    else
                        _snwprintf(new_full_name, sizeof(new_full_name), L"%s\\%s_", dir, new_file_name);

                    if(MoveFileW(old_full_name, new_full_name)==0)
                        error(1, L"Unable to Rename [%s] to [%s]", old_full_name, new_full_name);

                    wprintf(L"Successfully renamed to %s\n", new_full_name);
                }
             } else {
                 _snwprintf(new_full_name, sizeof(new_full_name), L"%s\\%s", dir, old_file_name);
             }

            if((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY) {
                folders++;
                recurse(new_full_name);
            }
            else {
                files++;
            }
            
        }
    }
    while(FindNextFileW(hFind, &find_data) != 0);

    FindClose(hFind);

    return 0;
}

int wmain(int argc, WCHAR **argv) {

    if(argc!=2)
        error(1, L"Usage: %s <topdir>\n\n", argv[0]);

    recurse(argv[1]);

    wprintf(L"Done.\n"
            L"Files  : %u\n"
            L"Folders: %u\n"
            L"Changed: %u\n",
                files, folders, changed);

    return 0;
}