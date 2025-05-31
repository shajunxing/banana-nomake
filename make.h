/*
Copyright 2024-2025 ShaJunXing <shajunxing@hotmail.com>

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef MAKE_H
#define MAKE_H

#ifdef _MSC_VER
    #pragma warning(disable : 4996) // such as "'strcpy' unsafe" or "'rmdir' deprecated"
#endif
#include <assert.h>
#include <errno.h>
#include <float.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <direct.h> // including functions getcwd, chdir, mkdir, rmdir
    #include <windows.h>
#else
    #include <dirent.h>
    #include <signal.h>
    #include <sys/stat.h>
    #include <sys/wait.h>
    #include <unistd.h>
#endif

enum compiler_type { msvc,
                     gcc };

enum os_type { windows,
               posix };

#ifdef _MSC_VER
    #define libext ".lib"
    #define objext ".obj"
const enum compiler_type compiler = msvc; // DON'T use #define, because cannot used in switch
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
    #if defined(__GNUC__) && defined(__STRICT_ANSI__)
        #error numargs() only works with gnu extension enabled
    #endif
    #define _numargs_call(__arg_0, __arg_1) __arg_0 __arg_1
    #define _numargs_select(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, __arg_0, ...) __arg_0
    #define numargs(...) _numargs_call(_numargs_select, (_, ##__VA_ARGS__, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0))
#endif

#define _log_x(__arg_0, __arg_1, __arg_2, ...) printf("%s:%d: " __arg_0 "\n", __arg_1, __arg_2, ##__VA_ARGS__)
#define _log(__arg_0, ...) _log_x(__arg_0, __FILE__, __LINE__, ##__VA_ARGS__)
#define _error_exit(__arg_0, ...)     \
    do {                              \
        _log(__arg_0, ##__VA_ARGS__); \
        exit(EXIT_FAILURE);           \
    } while (0)
#ifdef _WIN32
const char *_windows_error_string() {
    // https://learn.microsoft.com/en-us/windows/win32/debug/retrieving-the-last-error-code
    // https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-formatmessage
    static char es[256];
    memset(es, 0, sizeof(es));
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   NULL, GetLastError(), 1033, es, sizeof(es), NULL);
    return es;
}
    #define _windows_error_exit() _error_exit("error %ld: %s", GetLastError(), _windows_error_string())
#endif
#define _posix_error_exit() _error_exit("error %d: %s", errno, strerror(errno)) // also exists in windows

// do not use float, because va_arg needs double, see https://stackoverflow.com/questions/11270588/variadic-function-va-arg-doesnt-work-with-float same below
double _max(size_t nargs, ...) {
    double ret = -DBL_MAX;
    size_t i;
    va_list args;
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
    size_t i, len;
    char *ret;
    va_list args;
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
    size_t i, len = strlen(*dest);
    va_list args;
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

bool _equals(const char *str, size_t nargs, ...) {
    bool ret = false;
    size_t i;
    va_list args;
    va_start(args, nargs);
    for (i = 0; i < nargs; i++) {
        const char *cmp = va_arg(args, const char *);
        if (strcmp(str, cmp) == 0) {
            ret = true;
            break;
        }
    }
    va_end(args);
    return ret;
}
#define equals(__arg_0, ...) _equals(__arg_0, numargs(__VA_ARGS__), ##__VA_ARGS__)

bool _startswith(const char *str, size_t nargs, ...) {
    size_t len = strlen(str);
    bool ret = false;
    size_t i;
    va_list args;
    va_start(args, nargs);
    for (i = 0; i < nargs; i++) {
        const char *prefix = va_arg(args, const char *);
        size_t prefixlen = strlen(prefix);
        if (prefixlen <= len && strncmp(str, prefix, prefixlen) == 0) {
            ret = true;
            break;
        }
    }
    va_end(args);
    return ret;
}
#define startswith(__arg_0, ...) _startswith(__arg_0, numargs(__VA_ARGS__), ##__VA_ARGS__)

bool _endswith(const char *str, size_t nargs, ...) {
    size_t len = strlen(str);
    bool ret = false;
    size_t i;
    va_list args;
    va_start(args, nargs);
    for (i = 0; i < nargs; i++) {
        const char *suffix = va_arg(args, const char *);
        size_t suffixlen = strlen(suffix);
        if (suffixlen <= len && strncmp(str + len - suffixlen, suffix, suffixlen) == 0) {
            ret = true;
            break;
        }
    }
    va_end(args);
    return ret;
}
#define endswith(__arg_0, ...) _endswith(__arg_0, numargs(__VA_ARGS__), ##__VA_ARGS__)

#if defined(_MSC_VER) && _MSC_VER < 1900
    // https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/vsnprintf-vsnprintf-vsnprintf-l-vsnwprintf-vsnwprintf-l?view=msvc-170
    #error below vs2015, _vsnprintf() behavior is different with vsnprintf()
#endif
char *format(const char *fmt, ...) {
    char *buf = NULL;
    size_t buflen;
    int result;
    // https://en.cppreference.com/w/c/io/vfprintf
    va_list args, args_copy;
    va_start(args, fmt);
    va_copy(args_copy, args);
    result = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    if (result < 0) {
        va_end(args_copy);
        return NULL;
    }
    buflen = (size_t)result + 1;
    buf = (char *)calloc(buflen, 1);
    result = vsnprintf(buf, buflen, fmt, args_copy);
    va_end(args_copy);
    if (result < 0) {
        free(buf);
        return NULL;
    }
    return buf;
}

double __mtime(const char *filename) {
    double ret = -DBL_MAX;
#ifdef _WIN32
    HANDLE fh;
    FILETIME mt;
    fh = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (fh != INVALID_HANDLE_VALUE) {
        if (GetFileTime(fh, NULL, NULL, &mt) != 0) {
            ret = (mt.dwHighDateTime * 4294967296.0 + mt.dwLowDateTime) / 10000000.0 - 11644473600.0;
        }
        CloseHandle(fh);
    }
#else
    struct stat sb;
    if (lstat(filename, &sb) == 0) {
        ret = (double)sb.st_mtime;
    }
#endif
    return ret;
}
double _mtime(int nargs, ...) {
    double ret = -DBL_MAX;
    int i;
    va_list args;
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
    char *standardized_dir = endswith(dir, pathsep) ? concat(dir) : concat(dir, pathsep);
    bool isdir;
    char *filename;
#ifdef _WIN32
    char *pattern = concat(standardized_dir, "*");
    HANDLE sh;
    WIN32_FIND_DATAA fd;
    sh = FindFirstFileA(pattern, &fd);
    if (sh != INVALID_HANDLE_VALUE) {
        do {
            isdir = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
            filename = fd.cFileName;
#else
    // https://www.geeksforgeeks.org/c-program-list-files-sub-directories-directory/
    struct dirent *de;
    DIR *dr = opendir(standardized_dir);
    if (dr != NULL) {
        while ((de = readdir(dr)) != NULL) {
            isdir = de->d_type == DT_DIR;
            filename = de->d_name;
#endif
            // BEGIN BLOCK
            if (isdir) {
                if (!equals(filename, ".") && !equals(filename, "..")) {
                    char *subdir = concat(standardized_dir, filename, pathsep);
                    callback(subdir, NULL, NULL);
                    free(subdir);
                }
            } else {
                char *ext = strrchr(filename, '.');
                if (ext) {
                    size_t baselen = ext - filename;
                    char *base = (char *)calloc(baselen + 1, 1);
                    strncpy(base, filename, baselen);
                    callback(standardized_dir, base, ext);
                    free(base);
                } else {
                    callback(standardized_dir, filename, "");
                }
            }
            // END BLOCK
#ifdef _WIN32
        } while (FindNextFileA(sh, &fd) != 0);
        FindClose(sh);
    }
    free(pattern);
#else
        }
        closedir(dr);
    }
#endif
    free(standardized_dir);
}

// stdlib.c already has _sleep()
void __sleep(double secs) {
#ifdef _WIN32
    Sleep((DWORD)(secs * 1000));
#else
    usleep((int)(secs * 1000000));
#endif
}

#define run(__arg_0)                                           \
    do {                                                       \
        _log("%s", __arg_0);                                   \
        int ret = system(__arg_0);                             \
        if (ret != 0) {                                        \
            _error_exit("%s: exit code is %d.", __arg_0, ret); \
        }                                                      \
    } while (0)

unsigned _parallel_num_workers = 0;
struct _parallel_worker_info {
    const char *file;
    int line;
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
#ifdef _WIN32
        SYSTEM_INFO info;
        GetSystemInfo(&info);
        _parallel_num_workers = (unsigned)info.dwNumberOfProcessors;
#else
        // https://stackoverflow.com/questions/2693948/how-do-i-retrieve-the-number-of-processors-on-c-linux
        _parallel_num_workers = (unsigned)sysconf(_SC_NPROCESSORS_ONLN);
#endif
        if (_parallel_num_workers == 0) {
            _error_exit("Failed to get _parallel_num_workers.");
        }
        _parallel_workers = (struct _parallel_worker_info *)calloc(_parallel_num_workers, sizeof(struct _parallel_worker_info));
    }
}
void _parallel_zero_out(size_t slot) {
    _parallel_workers[slot].file = NULL;
    _parallel_workers[slot].line = 0;
    _parallel_workers[slot].proc = 0;
    free(_parallel_workers[slot].cmd);
    _parallel_workers[slot].cmd = NULL;
}
void _parallel_kill_all() {
    size_t slot;
    for (slot = 0; slot < _parallel_num_workers; slot++) {
        if (_parallel_workers[slot].proc != 0) {
            // printf("kill: %s\n", _parallel_workers[slot].cmd);
#ifdef _WIN32
            if (TerminateProcess(_parallel_workers[slot].proc, EXIT_FAILURE)) {
                _parallel_zero_out(slot);
            } else {
                _windows_error_exit();
            }
#else
            if (kill(_parallel_workers[slot].proc, SIGKILL) == 0) {
                _parallel_zero_out(slot);
            } else {
                _posix_error_exit();
            }
#endif
        }
    }
}
// void _parallel_cleanup() {
//     _parallel_num_workers = 0;
//     free(_parallel_workers);
//     _parallel_workers = NULL;
// }
bool _parallel_check(size_t slot) {
    long exit_code;
    if (_parallel_workers[slot].proc == 0) { // slot is available
        return true;
    }
    {
#ifdef _WIN32
        DWORD status;
        if (WaitForSingleObject(_parallel_workers[slot].proc, 0) != WAIT_OBJECT_0) { // process is running
            return false;
        }
        if (GetExitCodeProcess(_parallel_workers[slot].proc, &status) == 0) {
            _log("%s", _windows_error_string());
            _parallel_kill_all();
            exit(EXIT_FAILURE);
        }
        exit_code = (long)status;
#else
        int status;
        if (waitpid(_parallel_workers[slot].proc, &status, WNOHANG) == 0) { // process is running
            return false;
        }
        exit_code = WEXITSTATUS(status);
#endif
    }
    if (exit_code != 0) {
        _log_x("%s: exit code is %ld.", _parallel_workers[slot].file, _parallel_workers[slot].line, _parallel_workers[slot].cmd, exit_code);
        _parallel_zero_out(slot); // must zero out to prevent next kill error
        _parallel_kill_all();
        exit(EXIT_FAILURE);
    } else {
        _parallel_zero_out(slot);
        return true;
    }
}
void _parallel_run(const char *file, int line, const char *cmd) {
    size_t slot;
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
            __sleep(0.2);
        }
    }
    _log_x("%s", file, line, cmd);
    {
#ifdef _WIN32
        STARTUPINFOA si = {sizeof(STARTUPINFOA)};
        PROCESS_INFORMATION pi;
        if (CreateProcessA(NULL, (LPSTR)cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
            _parallel_workers[slot].file = file;
            _parallel_workers[slot].line = line;
            _parallel_workers[slot].proc = pi.hProcess;
            _parallel_workers[slot].cmd = (char *)calloc(strlen(cmd) + 1, 1);
            strcpy(_parallel_workers[slot].cmd, cmd);
        } else {
            _windows_error_exit();
        }
#else
        pid_t pid = fork(); // posix_spawn's parameters are hard to use
        switch (pid) {
        case -1:
            _posix_error_exit();
            break;
        case 0: {
                // from chatgpt
    #define MAX_ARGS 64
            char *argv[MAX_ARGS];
            char *cmd_copy = strdup(cmd);
            char *token;
            int i = 0;
            if (!cmd_copy) {
                _posix_error_exit();
            }
            token = strtok(cmd_copy, " ");
            while (token != NULL && i < MAX_ARGS - 1) {
                argv[i++] = token;
                token = strtok(NULL, " ");
            }
            argv[i] = NULL;
            if (i == 0) {
                free(cmd_copy);
                _error_exit("Empty command\n");
            }
            execvp(argv[0], argv);
            // Only reached if execvp fails
            perror("execvp failed");
            free(cmd_copy);
            _posix_error_exit();
        } break;
        default:
            _parallel_workers[slot].file = file;
            _parallel_workers[slot].line = line;
            _parallel_workers[slot].proc = pid;
            _parallel_workers[slot].cmd = (char *)calloc(strlen(cmd) + 1, 1);
            strcpy(_parallel_workers[slot].cmd, cmd);
            break;
        }
#endif
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
            __sleep(0.2);
        }
    }
}

#endif