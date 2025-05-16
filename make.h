/*
Copyright 2024-2025 ShaJunXing <shajunxing@hotmail.com>

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef MAKE_H
#define MAKE_H

#include <assert.h>
#include <float.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <direct.h> // including functions getcwd, chdir, mkdir, rmdir
    #include <windows.h>
#else
    #include <sys/stat.h>
    #include <unistd.h>
#endif

// https://en.cppreference.com/w/c/preprocessor/replace
#if __STDC_VERSION__ >= 199901L // c99
    #include <stdbool.h>
#else
// https://stackoverflow.com/questions/1608318/is-bool-a-native-c-type/1608350
typedef enum {
    false = (1 == 0),
    true = (!false)
} bool;
#endif

enum compiler_type { msvc,
                     gcc };

enum os_type { windows,
               posix };

#ifdef _MSC_VER
    #define libext ".lib"
    #define objext ".obj"
const enum compiler_type compiler = msvc;
#else
    #define libext ".a"
    #define objext ".o"
const enum compiler_type compiler = gcc;
#endif

#ifdef _WIN32
    #define dllext ".dll" // use define instead of const for easily string literal concatenation
    #define exeext ".exe"
    #define pathsep "\\"
const enum os_type os = windows;
#else
    #define dllext ".so"
    #define exeext ""
    #define pathsep "/"
const enum os_type os = posix;
#endif

#ifndef numargs
    // https://stackoverflow.com/questions/2124339/c-preprocessor-va-args-number-of-arguments
    // in msvc, works fine even with old version
    // in gcc, only works with default or -std=gnu2x, -std=c?? will treat zero parameter as 1, maybe ## is only recognized by gnu extension
    #define _numargs_call(__arg_0, __arg_1) __arg_0 __arg_1
    #define _numargs_select(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, __arg_0, ...) __arg_0
    #define numargs(...) _numargs_call(_numargs_select, (_, ##__VA_ARGS__, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0))
#endif

// do not use float, because va_arg needs double, see https://stackoverflow.com/questions/11270588/variadic-function-va-arg-doesnt-work-with-float same below
double _max(size_t nargs, ...) {
    double ret = -DBL_MAX;
    va_list args;
    size_t i;
    va_start(args, nargs);
    for (i = 0; i < nargs; i++) {
        double v = va_arg(args, double);
        if (v > ret) {
            ret = v;
        }
    }
    va_end(args);
    return ret;
}
#ifdef max // see https://stackoverflow.com/questions/4234004/is-maxa-b-defined-in-stdlib-h-or-not
    #undef max
#endif
#define max(...) _max(numargs(__VA_ARGS__), ##__VA_ARGS__)

#ifndef countof
    // https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/countof-macro?view=msvc-170
    #define countof(__arg_0) (sizeof(__arg_0) / sizeof(__arg_0[0]))
#endif

char *_join(const char *sep, size_t nargs, ...) {
    va_list args;
    size_t i, len;
    char *ret;
    va_start(args, nargs);
    for (len = 0, i = 0; i < nargs; i++) {
        len += strlen(va_arg(args, char *));
    }
    va_end(args);
    len += (strlen(sep) * (nargs - 1));
    ret = (char *)calloc(len + 1, 1);
    va_start(args, nargs);
    for (i = 0; i < nargs; i++) {
        if (i > 0) {
            strcat(ret, sep);
        }
        strcat(ret, va_arg(args, char *));
    }
    va_end(args);
    return ret;
}
#define join(__arg_0, ...) _join(__arg_0, numargs(__VA_ARGS__), ##__VA_ARGS__)
#define concat(...) join("", ##__VA_ARGS__)

void _append(char **dest, size_t nargs, ...) {
    va_list args;
    size_t i, len;
    len = strlen(*dest);
    va_start(args, nargs);
    for (i = 0; i < nargs; i++) {
        len += strlen(va_arg(args, char *));
    }
    va_end(args);
    *dest = (char *)realloc(*dest, len + 1);
    va_start(args, nargs);
    for (i = 0; i < nargs; i++) {
        strcat(*dest, va_arg(args, char *));
    }
    va_end(args);
}
#define append(__arg_0, ...) _append(__arg_0, numargs(__VA_ARGS__), ##__VA_ARGS__)

bool equals(const char *str1, const char *str2) {
    return strcmp(str1, str2) == 0;
}

bool startswith(const char *str, const char *prefix) {
    size_t len = strlen(str);
    size_t prefixlen = strlen(prefix);
    if (prefixlen > len) {
        return false;
    }
    return strncmp(str, prefix, prefixlen) == 0;
}

bool endswith(const char *str, const char *suffix) {
    size_t len = strlen(str);
    size_t suffixlen = strlen(suffix);
    if (suffixlen > len) {
        return false;
    }
    return strncmp(str + len - suffixlen, suffix, suffixlen) == 0;
}

#define run(__arg_0)                                                      \
    do {                                                                  \
        int ret;                                                          \
        ret = system(__arg_0);                                            \
        if (ret != 0) {                                                   \
            printf("%s:%d: exit code is %d.\n", __FILE__, __LINE__, ret); \
            exit(EXIT_FAILURE);                                           \
        }                                                                 \
    } while (0)

double __mtime(const char *filename) {
#ifdef _WIN32
    HANDLE fh;
    FILETIME mt;
    double ret = 0;
    fh = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (fh != INVALID_HANDLE_VALUE) {
        if (GetFileTime(fh, NULL, NULL, &mt) != 0) {
            ret = (mt.dwHighDateTime * 4294967296.0 + mt.dwLowDateTime) / 10000000.0 - 11644473600.0;
        }
        CloseHandle(fh);
    }
    return ret;
#else
    struct stat sb;
    if (lstat(filename, &sb) == -1) {
        return 0;
    }
    return sb.st_mtime;
#endif
}
double _mtime(int nargs, ...) {
    double ret = -DBL_MAX;
    va_list args;
    int i;
    va_start(args, nargs);
    for (i = 0; i < nargs; i++) {
        double t = __mtime(va_arg(args, char *));
        if (t > ret) {
            ret = t;
        }
    }
    va_end(args);
    return ret;
}
#define mtime(...) _mtime(numargs(__VA_ARGS__), ##__VA_ARGS__)

void listdir(const char *dir, void (*callback)(const char *dir, const char *base, const char *ext)) {
#ifdef _WIN32
    HANDLE sh;
    WIN32_FIND_DATA fd;
    char *standardized_dir;
    char *pattern;
    if (endswith(dir, pathsep)) {
        standardized_dir = concat(dir);
    } else {
        standardized_dir = concat(dir, pathsep);
    }
    pattern = concat(standardized_dir, "*");
    sh = FindFirstFile(pattern, &fd);
    if (sh != INVALID_HANDLE_VALUE) {
        do {
            if (equals(fd.cFileName, ".") || equals(fd.cFileName, "..")) {
                continue;
            }
            if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                char *subdir = concat(standardized_dir, fd.cFileName, pathsep);
                callback(subdir, NULL, NULL);
                free(subdir);
            } else {
                char *ext = strrchr(fd.cFileName, '.');
                if (ext) {
                    size_t baselen = ext - fd.cFileName;
                    char *base = (char *)calloc(baselen + 1, 1);
                    strncpy(base, fd.cFileName, baselen);
                    callback(standardized_dir, base, ext);
                    free(base);
                } else {
                    callback(standardized_dir, fd.cFileName, "");
                }
            }
        } while (FindNextFile(sh, &fd) != 0);
        FindClose(sh);
    }
    free(pattern);
    free(standardized_dir);
#else
        // TODO
#endif
}

// stdlib.c already has _sleep()
void __sleep(double secs) {
#ifdef _MSC_VER
    Sleep((DWORD)(secs * 1000));
#else
    usleep((int)(secs * 1000000));
#endif
}

const char *_win_error_string() {
#ifdef _WIN32
    // https://learn.microsoft.com/en-us/windows/win32/debug/retrieving-the-last-error-code
    // https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-formatmessage
    static char es[256];
    memset(es, 0, sizeof(es));
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   NULL, GetLastError(), 1033, es, sizeof(es), NULL);
    return es;
#else
    return strerror(errno);
#endif
}

unsigned _parallel_num_workers = 0;
struct _parallel_worker_info {
    const char *file;
    size_t line;
#ifdef _WIN32
    HANDLE proc;
#else
    pid_t proc;
#endif
    char *cmd;
};
struct _parallel_worker_info *_parallel_workers = NULL;
void _parallel_init() {
    if (_parallel_num_workers == 0) {
        SYSTEM_INFO info;
        GetSystemInfo(&info);
        _parallel_num_workers = (unsigned)info.dwNumberOfProcessors;
        if (_parallel_num_workers == 0) {
            printf("%s:%d: Failed to get _parallel_num_workers.\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        _parallel_workers = (struct _parallel_worker_info *)calloc(_parallel_num_workers, sizeof(struct _parallel_worker_info));
    }
}
void _parallel_kill_all() {
    size_t slot;
    for (slot = 0; slot < _parallel_num_workers; slot++) {
        if (_parallel_workers[slot].proc != 0) {
            printf("Kill: %ld %s\n", slot, _parallel_workers[slot].cmd);
            if (TerminateProcess(_parallel_workers[slot].proc, EXIT_FAILURE)) {
                _parallel_workers[slot].file = NULL;
                _parallel_workers[slot].line = 0;
                _parallel_workers[slot].proc = 0;
                free(_parallel_workers[slot].cmd);
                _parallel_workers[slot].cmd = NULL;
            } else {
                printf("%s:%d: TerminateProcess() failed: %s\n", _win_error_string());
                exit(EXIT_FAILURE);
            }
        }
    }
}
void _parallel_cleanup() {
    _parallel_num_workers = 0;
    free(_parallel_workers);
    _parallel_workers = NULL;
}
// TODO: use posix_spawn, waitpid, numCPU = sysconf( _SC_NPROCESSORS_ONLN ) https://stackoverflow.com/questions/2693948/how-do-i-retrieve-the-number-of-processors-on-c-linux
bool _parallel_check(size_t slot) {
    DWORD exit_code;
    // printf("_parallel_check(%llu)\n", slot);
    if (_parallel_workers[slot].proc == 0) { // slot is available
        return true;
    } else if (WaitForSingleObject(_parallel_workers[slot].proc, 0) == WAIT_OBJECT_0) { // process finished
        if (GetExitCodeProcess(_parallel_workers[slot].proc, &exit_code)) {
            if (exit_code != 0) {
                printf("%s:%d: exit code is %d.\n", _parallel_workers[slot].file, _parallel_workers[slot].line, exit_code);
                _parallel_workers[slot].file = NULL;
                _parallel_workers[slot].line = 0;
                _parallel_workers[slot].proc = 0;
                free(_parallel_workers[slot].cmd);
                _parallel_workers[slot].cmd = NULL;
                _parallel_kill_all();
                exit(EXIT_FAILURE);
            } else {
                _parallel_workers[slot].file = NULL;
                _parallel_workers[slot].line = 0;
                _parallel_workers[slot].proc = 0;
                free(_parallel_workers[slot].cmd);
                _parallel_workers[slot].cmd = NULL;
                return true;
            }
        } else {
            printf("%s:%d: GetExitCodeProcess() failed: %s\n", _win_error_string());
            _parallel_kill_all();
            exit(EXIT_FAILURE);
        }
    } else {
        return false;
    }
}
void _parallel_run(const char *file, size_t line, const char *cmd) {
    size_t slot;
    STARTUPINFOA si = {sizeof(STARTUPINFOA)};
    PROCESS_INFORMATION pi;
    _parallel_init();
    for (;;) { // no available slot
        // wait one of the _parallel_workers to finish
        bool any_done = false;
        for (slot = 0; slot < _parallel_num_workers; slot++) {
            if (_parallel_check(slot)) {
                any_done = true;
                break;
            }
        }
        if (any_done) {
            break;
        } else {
            Sleep(200);
        }
    }
    if (CreateProcessA(NULL, (LPSTR)cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        _parallel_workers[slot].file = file;
        _parallel_workers[slot].line = line;
        _parallel_workers[slot].proc = pi.hProcess;
        _parallel_workers[slot].cmd = (char *)calloc(strlen(cmd) + 1, 1);
        strcpy(_parallel_workers[slot].cmd, cmd);
    } else {
        printf("%s:%d: CreateProcessA() failed: %s\n", _win_error_string());
        exit(EXIT_FAILURE);
    }
}
#define async(__arg_0) _parallel_run(__FILE__, __LINE__, __arg_0)
void await() {
    if (_parallel_num_workers == 0) {
        return;
    }
    for (;;) {
        bool all_done = true;
        size_t slot;
        for (slot = 0; slot < _parallel_num_workers; slot++) {
            // must loop whole workers to handle possible error exit
            all_done &= _parallel_check(slot);
        }
        if (all_done) {
            break;
        } else {
            Sleep(200);
        }
    }
}

#endif