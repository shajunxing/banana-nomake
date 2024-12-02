/*
Copyright 2024 ShaJunXing <shajunxing@hotmail.com>

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include <assert.h>
#include <float.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum compiler_type { msvc,
                     gcc };

#if defined(_MSC_VER)
const enum compiler_type compiler = msvc;
#else
const enum compiler_type compiler = gcc;
#endif

// https://stackoverflow.com/questions/2124339/c-preprocessor-va-args-number-of-arguments 兼容msvc
// _1 ... _63是参数名占位符，最终返回N
// 0个参数会有问题，参见https://gist.github.com/aprell/3722962，也只能支持gcc/clang
#define _expand(x) x
#define _numargs(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, N, ...) N
#define numargs(...) _expand(_numargs(_, ##__VA_ARGS__, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0))

// 计算多个浮点数的最大值
// 不用float，因为va_arg需要double，参见 https://stackoverflow.com/questions/11270588/variadic-function-va-arg-doesnt-work-with-float 下同
double _max(int num, ...) {
    double ret = -DBL_MAX;
    va_list args;
    int i;
    va_start(args, num);
    for (i = 0; i < num; i++) {
        double v = va_arg(args, double);
        if (v > ret) {
            ret = v;
        }
    }
    va_end(args);
    return ret;
}
#define max(...) _max(numargs(__VA_ARGS__), ##__VA_ARGS__)

// 连接字符串，返回值需要手动free
char *_join(const char *sep, int num, ...) {
    va_list args;
    int i, len;
    char *ret;
    va_start(args, num);
    for (len = 0, i = 0; i < num; i++) {
        len += strlen(va_arg(args, char *));
    }
    va_end(args);
    len += (strlen(sep) * (num - 1));
    ret = (char *)calloc(len + 1, 1);
    va_start(args, num);
    for (i = 0; i < num; i++) {
        if (i > 0) {
            strcat(ret, sep);
        }
        strcat(ret, va_arg(args, char *));
    }
    va_end(args);
    return ret;
}
#define join(sep, ...) _join(sep, numargs(__VA_ARGS__), ##__VA_ARGS__)
#define concat(...) join("", ##__VA_ARGS__)

// 注意dest应该是已经动态分配的内存
void append(char **dest, const char *src) {
    *dest = (char *)realloc(*dest, strlen(*dest) + strlen(src) + 1);
    strcat(*dest, src);
}

// 判断字符串是否相等
int equals(const char *str1, const char *str2) {
    return strcmp(str1, str2) == 0;
}

// 判断字符串是否以给定的开头
int startswith(const char *str, const char *prefix) {
    int len = strlen(str);
    int prefixlen = strlen(prefix);
    if (prefixlen > len) {
        return 0;
    }
    return strncmp(str, prefix, prefixlen) == 0;
}

// 判断字符串是否以给定的结尾
int endswith(const char *str, const char *suffix) {
    int len = strlen(str);
    int suffixlen = strlen(suffix);
    if (suffixlen > len) {
        return 0;
    }
    return strncmp(str + len - suffixlen, suffix, suffixlen) == 0;
}

enum os_type { windows,
               posix };

#if defined(_WIN32)

    #define WIN32_LEAN_AND_MEAN
    #include <direct.h> // 包含getcwd, chdir, mkdir, rmdir函数
    #include <windows.h>

// 操作系统
enum os_type os = windows;
// 目录分隔符
const char *pathsep = "\\";
// 常用文件扩展名
const char *objext = ".obj";
const char *dllext = ".dll";
const char *exeext = ".exe";

// 返回文件修改时间，UTC秒
double __mtime(const char *filename) {
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
}

// 遍历目录，注意dir必须以分隔符结尾，callback(dir, base, ext)，如果是子目录，那么dir就是子目录，base、ext都为NULL，如果是文件，那么dir就是父目录，base、ext都不为NULL，ext带点号，即使没有扩展名，ext也不为NULL，而是""，这样做的好处是可以直接concat(dir, base, ext)得到完整文件名
void listdir(const char *dir, void (*callback)(const char *, const char *, const char *)) {
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
                    int baselen = ext - fd.cFileName;
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
}

#else

    #include <sys/stat.h>
    #include <unistd.h>

enum os_type os = posix;
const char *pathsep = "/";
const char *objext = ".o";
const char *dllext = ".so";
const char *exeext = "";

double __mtime(const char *filename) {
    struct stat sb;
    if (lstat(filename, &sb) == -1) {
        return 0;
    }
    return sb.st_mtime;
}

#endif

// 返回一个或多个文件的最近修改时间
double _mtime(int num, ...) {
    double ret = -DBL_MAX;
    va_list args;
    int i;
    va_start(args, num);
    for (i = 0; i < num; i++) {
        double t = __mtime(va_arg(args, char *));
        if (t > ret) {
            ret = t;
        }
    }
    va_end(args);
    return ret;
}
#define mtime(...) _mtime(numargs(__VA_ARGS__), ##__VA_ARGS__)