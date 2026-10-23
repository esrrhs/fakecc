/* Report a child's exit code as 8 bits.
 *
 * MSYS bash turns ExitProcess(0xFFFFFFFD) into 127, so a MinGW `return -3`
 * and fakecc's ExitProcess(253) disagree even though both are the POSIX
 * status -3.  This waits on the child itself and returns code & 0xFF, which
 * bash passes through unchanged.  A timeout kills the child and returns 124,
 * matching GNU timeout.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "usage: win_exit8 seconds program [args...]\n");
        return 2;
    }
    int seconds = atoi(argv[1]);
    size_t n = 1;
    for (int i = 2; i < argc; i++)
        n += strlen(argv[i]) + 3;
    char *cmd = malloc(n);
    if (!cmd) return 2;
    char *p = cmd;
    for (int i = 2; i < argc; i++) {
        int quote = strchr(argv[i], ' ') || strchr(argv[i], '\t');
        if (i > 2) *p++ = ' ';
        if (quote) *p++ = '"';
        size_t len = strlen(argv[i]);
        memcpy(p, argv[i], len);
        p += len;
        if (quote) *p++ = '"';
    }
    *p = 0;

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    memset(&si, 0, sizeof si);
    si.cb = sizeof si;
    if (!CreateProcessA(NULL, cmd, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
        fprintf(stderr, "win_exit8: cannot start %s (%lu)\n", argv[2], GetLastError());
        free(cmd);
        return 127;
    }
    free(cmd);
    DWORD wait = seconds > 0 ? (DWORD)seconds * 1000u : INFINITE;
    if (WaitForSingleObject(pi.hProcess, wait) == WAIT_TIMEOUT) {
        TerminateProcess(pi.hProcess, 124);
        WaitForSingleObject(pi.hProcess, 2000);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return 124;
    }
    DWORD code = 1;
    GetExitCodeProcess(pi.hProcess, &code);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return (int)(code & 0xFFu);
}
