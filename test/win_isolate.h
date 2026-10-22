#ifndef FAKECC_WIN_ISOLATE_H
#define FAKECC_WIN_ISOLATE_H
/* The POSIX unit tests fork so a crash in one case cannot kill the suite.
 * MinGW has no fork.  The child block runs in this process.  exit() inside
 * the linked compiler is redirected with -Wl,--wrap=exit to the wrapper
 * below, which longjmps back to fork() with the status waitpid reports.
 * stderr is restored so a child that redirects it does not leak that into
 * the parent. */
#include <direct.h>
#include <fcntl.h>
#include <io.h>
#include <process.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void fakecc_clear_error(void);

#ifndef STDERR_FILENO
#define STDERR_FILENO 2
#endif

static const char *win_devnull(const char *path) __attribute__((unused));
static const char *win_devnull(const char *path) {
    if (path && strcmp(path, "/dev/null") == 0) return "NUL";
    return path;
}
#define open(path, flags) _open(win_devnull(path), (flags))
#define close _close
#define read _read
#define dup2 _dup2

static int win_mkdir(const char *path, int mode) __attribute__((unused));
static int win_mkdir(const char *path, int mode) {
    (void)mode;
    return _mkdir(path);
}
#define mkdir(path, mode) win_mkdir((path), (mode))

static char *win_mkdtemp(char *tmpl) __attribute__((unused));
static char *win_mkdtemp(char *tmpl) {
    if (_mktemp(tmpl) == NULL) return NULL;
    if (_mkdir(tmpl) != 0) return NULL;
    return tmpl;
}
#define mkdtemp(tmpl) win_mkdtemp(tmpl)

/* Host path a MinGW process can create.  /tmp is an MSYS mount, not a path
 * the native runtime can open. */
static char *win_tmp(const char *name) __attribute__((unused));
static char *win_tmp(const char *name) {
    static char buf[8][260];
    static int slot;
    char *p = buf[slot++ & 7];
    const char *t = getenv("TEMP");
    if (!t || !t[0]) t = ".";
    snprintf(p, 260, "%s\\%s", t, name);
    return p;
}

static jmp_buf win_fork_jb[8];
static int win_fork_sp;
static int win_fork_status;
static int win_fork_saved_err[8];

static void win_child_leave(int code) __attribute__((noreturn));
static void win_child_leave(int code) {
    int sp = --win_fork_sp;
    if (sp >= 0 && win_fork_saved_err[sp] >= 0) {
        _dup2(win_fork_saved_err[sp], 2);
        _close(win_fork_saved_err[sp]);
    }
    win_fork_status = code;
    fakecc_clear_error();
    longjmp(win_fork_jb[sp], 1);
}

#define fork()                                                                 \
    (win_fork_saved_err[win_fork_sp] = _dup(2),                                \
     setjmp(win_fork_jb[win_fork_sp++]) == 0 ? 0 : 1)
#define _exit(code) win_child_leave((int)(code))
#define waitpid(pid, stp, fl)                                                  \
    do {                                                                       \
        (void)(pid);                                                           \
        (void)(fl);                                                            \
        *(stp) = (win_fork_status << 8);                                       \
    } while (0)
#define WIFEXITED(st) (1)
#define WEXITSTATUS(st) (((st) >> 8) & 0xff)
#define WIFSIGNALED(st) (0)
#define WTERMSIG(st) (0)

/* GNU ld --wrap.  Calls compiled into fakecc_core hit these, not the macros
 * above, because those calls live in other translation units. */
void __real_exit(int code);
void __real_abort(void);

void __wrap_exit(int code) __attribute__((used));
void __wrap_exit(int code) {
    if (win_fork_sp > 0) win_child_leave(code);
    __real_exit(code);
}

void __wrap_abort(void) __attribute__((used));
void __wrap_abort(void) {
    if (win_fork_sp > 0) win_child_leave(134);
    __real_abort();
}

/* Run a linked image and report its status through the fork shim. */
static int win_execl(const char *path, const char *arg0, ...) __attribute__((unused));
static int win_execl(const char *path, const char *arg0, ...) {
    (void)arg0;
    intptr_t rc = _spawnl(_P_WAIT, path, path, NULL);
    if (rc < 0) win_child_leave(127);
    win_child_leave((int)rc);
}
#define execl win_execl
#endif
