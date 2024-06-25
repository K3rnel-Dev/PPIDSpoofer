#include <windows.h>
#include <stdio.h>
#include <processthreadsapi.h>

BOOL CreatePPidSpoofedProcess(HANDLE hParentProcess, LPCSTR lpProcessPath, DWORD* dwProcessId, HANDLE* hProcess, HANDLE* hThread) {
    SIZE_T sThreadAttList = 0;
    PPROC_THREAD_ATTRIBUTE_LIST pThreadAttList = NULL;

    STARTUPINFOEXA SiEx = { 0 };
    PROCESS_INFORMATION Pi = { 0 };

    RtlSecureZeroMemory(&SiEx, sizeof(STARTUPINFOEXA));
    RtlSecureZeroMemory(&Pi, sizeof(PROCESS_INFORMATION));

    // Setting the size of the structure
    SiEx.StartupInfo.cb = sizeof(STARTUPINFOEXA);

    // This will fail with ERROR_INSUFFICIENT_BUFFER, as expected
    InitializeProcThreadAttributeList(NULL, 1, 0, &sThreadAttList);

    // Allocating enough memory
    pThreadAttList = (PPROC_THREAD_ATTRIBUTE_LIST)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sThreadAttList);
    if (pThreadAttList == NULL) {
        printf("[!] HeapAlloc Failed With Error: %d\n", GetLastError());
        return FALSE;
    }

    // Calling InitializeProcThreadAttributeList again, but passing the right parameters
    if (!InitializeProcThreadAttributeList(pThreadAttList, 1, 0, &sThreadAttList)) {
        printf("[!] InitializeProcThreadAttributeList Failed With Error: %d\n", GetLastError());
        return FALSE;
    }

    if (!UpdateProcThreadAttribute(pThreadAttList, 0, PROC_THREAD_ATTRIBUTE_PARENT_PROCESS, &hParentProcess, sizeof(HANDLE), NULL, NULL)) {
        printf("[!] UpdateProcThreadAttribute Failed With Error: %d\n", GetLastError());
        return FALSE;
    }

    // Setting the LPPROC_THREAD_ATTRIBUTE_LIST element in SiEx to be equal to what was created using UpdateProcThreadAttribute
    SiEx.lpAttributeList = pThreadAttList;

    if (!CreateProcessA(
        NULL,
        (LPSTR)lpProcessPath,
        NULL,
        NULL,
        FALSE,
        EXTENDED_STARTUPINFO_PRESENT,
        NULL,
        NULL,
        &SiEx.StartupInfo,
        &Pi)) {
        printf("[!] CreateProcessA Failed with Error: %d\n", GetLastError());
        return FALSE;
    }

    *dwProcessId = Pi.dwProcessId;
    *hProcess = Pi.hProcess;
    *hThread = Pi.hThread;

    // Cleaning up
    DeleteProcThreadAttributeList(pThreadAttList);
    HeapFree(GetProcessHeap(), 0, pThreadAttList);

    return TRUE;
}

int main(int argc, char* argv[]) {
    SetConsoleTitle(L"[0x0] - PPID SPOOFER by k3rnel-dev https://github.com/k3rnel-dev");
    printf(""
        " .-----------------------------------------------------------------------.\n"
        " | _____ _____ _____ ____      _____ _____ _____ _____ _____ _____ _____ |\n"
        " ||  _  |  _  |     |    \\ ___|   __|  _  |     |     |   __|   __| __  ||\n"
        " ||   __|   __|-   -|  |  |___|__   |   __|  |  |  |  |   __|   __|    -||\n"
        " ||__|  |__|  |_____|____/    |_____|__|  |_____|_____|__|  |_____|__|__||\n"
        " '-----------------------------------------------------------------------'\n"
        "");
    if (argc != 3) {
        printf("Usage: %s <ParentPID> <ProcessPath>\n", argv[0]);
        return 1;
    }

    DWORD dwParentPid = atoi(argv[1]);
    LPCSTR lpProcessPath = argv[2];

    HANDLE hParentProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwParentPid);
    if (hParentProcess == NULL) {
        printf("[!] OpenProcess Failed With Error: %d\n", GetLastError());
        return 1;
    }

    DWORD dwProcessId;
    HANDLE hProcess;
    HANDLE hThread;

    if (CreatePPidSpoofedProcess(hParentProcess, lpProcessPath, &dwProcessId, &hProcess, &hThread)) {
        printf("[+] Process created successfully! PID: %d\n", dwProcessId);
    }
    else {
        printf("[!] Failed to create process\n");
    }

    // Disposing Resources
    if (hProcess != NULL) CloseHandle(hProcess);
    if (hThread != NULL) CloseHandle(hThread);
    CloseHandle(hParentProcess);

    return 0;
}
