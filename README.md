# Banana NoMake, only one .h required, direct write script with C, replace those annoying gmake, nmake, cmake...

This article is openly licensed via [CC BY-NC-ND 4.0](https://creativecommons.org/licenses/by-nc-nd/4.0/).

[English Version](README.md) | [Chinese Version](README_zhCN.md)

Project Address: <https://github.com/shajunxing/banana-make>

I don't like those build systems, I think they break their own belief "mechanism better than policy" and "keep it simple stupid". Why should one learn those ugly and rigid rules? Wouldn't a Turing-Complete programming language be better? Since C compiler is essential, encapsulate necessary functions into a header file, most important points I summarized as follows: 1. **Recursive traversal of file and directories**; 2. **Comparison of file timestamps**; 3. **Serial and parallel execution of commands**, then I can happily write scripts in C, right? Customers would be happy too, as they won't need to install any additional build systems, they can simply type `gcc make.c && ./a.out` or `cl make.c && make.exe`, isn't it quite easy?

Brief guide: use `listdir` to batch process multiple files in a directory, use `max` `mtime` to compare file modification times, use `append` `concat` `endswith` `equals` `format` `join` `startswith` to handle strings, and use `async` `await` `run` to execute commands. Below are detailed API definitions:

|Constants|Description|
|-|-|
|`const enum compiler_type compiler`|Compiler type, can be one of `msvc` `gcc`.|
|`#define dllext`|File extension of shared library, e.g `".dll"` `".so"`|
|`#define exeext`|File extension of executable, e.g `".exe"`|
|`#define libext`|File extension of library, e.g `".lib"` `".a"`|
|`#define objext`|File extension of compiled object, e.g `".obj"` `".o"`|
|`const enum os_type os`|Operating system type, can be one of `windows` `posix`.|
|`#define pathsep`|File system path seperator, , e.g `"\\"` `"/"`|

|Functions|Description|
|-|-|
|`void append(char **dest, ...)`|Append multiple strings sequentially to end of `dest`, `dest` must be dynamically allocated.|
|`void async(const char *cmd)`|Parallel run command line `cmd`. Maximum number of workers equals to num of cpu cores. If return value is not 0, print error message and exit program.|
|`void await()`|Wait for all workers to finish.|
|`char *concat(...)`|Concatenate multiple strings, return string should be freed when used up.|
|`bool endswith(const char *str, ...)`|Determine whether `str` ends with any of rest parameters.|
|`bool equals(const char *str, ...)`|Determine whether `str` are equal to any of rest parameters.|
|`char *format(const char *fmt, ...)`|Format string like `printf`, return string should be freed when used up.|
|`char *join(char *sep, ...)`|Join multiple strings by given seperator `sep`, return string should be freed when used up.|
|`void listdir(const char *dir, void (*callback)(const char *dir, const char *base, const char *ext))`|Iterate all items in directory `dir`, whether `dir` ends with or without path seperator doesn't matter, for each item invoke `callback`, set 3 parameters: `dir` always ends with path seperator. If item is file, combination is complete file path, `ext` will be `""` if file has no extension. If is directory, `dir` will be subdirectory's full path, `base` and `ext` will be `NULL`.|
|`double max(...)`|Take one or more double values, returns maximum one.|
|`double mtime(...)`|Get one or more file modification utc time and returns latest one, value for non-existent file is -DBL_MAX|
|`void run(const char *cmd)`|Run command line `cmd`. If return value is not 0, print error message and exit program.|
|`bool startswith(const char *str, ...)`|Determine whether `str` starts with any of rest parameters.|

Here's an example:

```c
/*
Copyright 2024-2025 ShaJunXing <shajunxing@hotmail.com>

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "../banana-make/make.h"

#define proj "js"
#define bin_dir "bin" pathsep
#define build_dir "build" pathsep
#define src_dir "src" pathsep
#define dll_file_name proj dllext
#define dll_file_path bin_dir dll_file_name
#define lib_file bin_dir proj libext
char *cc = NULL;
char *ld = NULL;

// use macro instead of function to output correct line number
#define compile_library(obj_file, c_file)                                             \
    do {                                                                              \
        char *cmd = compiler == msvc                                                  \
                        ? concat(cc, " /DDLL /DEXPORT /Fo", obj_file, " ", c_file)    \
                        : concat(cc, " -D DLL -D EXPORT -o ", obj_file, " ", c_file); \
        async(cmd);                                                                   \
        free(cmd);                                                                    \
    } while (0)

#define compile_executable(obj_file, c_file)                                \
    do {                                                                    \
        char *cmd = compiler == msvc                                        \
                        ? concat(cc, " /DDLL /Fo", obj_file, " ", c_file)   \
                        : concat(cc, " -D DLL -o ", obj_file, " ", c_file); \
        async(cmd);                                                         \
        free(cmd);                                                          \
    } while (0)

#define link_executable(exe_file, obj_file)                                                           \
    do {                                                                                              \
        char *cmd =                                                                                   \
            compiler == msvc                                                                          \
                ? concat(ld, " /out:", exe_file, " ", obj_file, " ", lib_file)                        \
                : concat(ld, " -o ", exe_file, " ", obj_file, " -L", bin_dir, " -l:", dll_file_name); \
        async(cmd);                                                                                   \
        free(cmd);                                                                                    \
    } while (0)

#define e(__arg_base) (bin_dir __arg_base exeext)
#define o(__arg_base) (build_dir __arg_base objext)
#define c(__arg_base) (src_dir __arg_base ".c")
#define h(__arg_base) (src_dir __arg_base ".h")

void build() {
    mkdir(bin_dir);
    mkdir(build_dir);
    if (mtime(o("js-base")) < mtime(c("js-base"), h("js-base"))) {
        compile_library(o("js-base"), c("js-base"));
    }
    if (mtime(o("js-data")) < mtime(c("js-data"), h("js-data"), h("js-base"))) {
        compile_library(o("js-data"), c("js-data"));
    }
    if (mtime(o("js-vm")) < mtime(c("js-vm"), h("js-vm"), h("js-data"), h("js-base"))) {
        compile_library(o("js-vm"), c("js-vm"));
    }
    if (mtime(o("test")) < mtime(c("test"), h("test"), h("js-vm"), h("js-data"), h("js-base"))) {
        compile_executable(o("test"), c("test"));
    }
    await();
    if (mtime(dll_file_path) < mtime(o("js-base"), o("js-data"), o("js-vm"))) {
        char *objs = join(" ", o("js-base"), o("js-data"), o("js-vm"));
        char *cmd = compiler == msvc
                        ? concat(ld, " /dll /out:", dll_file_path, " ", objs)
                        : concat(ld, " -shared -o ", dll_file_path, " ", objs);
        async(cmd);
        free(cmd);
        free(objs);
    }
    await();
    if (mtime(e("test")) < mtime(o("test"), dll_file_path)) {
        link_executable(e("test"), o("test"));
    }
    await();
}

void cleanup(const char *dir, const char *base, const char *ext) {
    // printf("cleanup: %s%s%s\n", dir, base, ext);
    if (base) {
        char *file_name = concat(dir, base, ext);
        remove(file_name);
        free(file_name);
    } else {
        listdir(dir, cleanup);
        rmdir(dir);
    }
}

#define cc_msvc "cl /nologo /c /W3 /MD"
#define cc_gcc "gcc -c -Wall"
#define ld_msvc "link /nologo"
#define ld_gcc "gcc -fvisibility=hidden -fvisibility-inlines-hidden -static -static-libgcc"

int main(int argc, char **argv) {
    if (argc == 1 || (argc == 2 && equals(argv[1], "debug"))) {
        cc = compiler == msvc ? cc_msvc : cc_gcc;
        ld = compiler == msvc ? ld_msvc " /debug" : ld_gcc;
        build();
        return EXIT_SUCCESS;
    } else if (argc == 2) {
        if (equals(argv[1], "release")) {
            cc = compiler == msvc ? cc_msvc " /O2 /DNOLOGINFO" : cc_gcc " -O3 -DNOLOGINFO";
            ld = compiler == msvc ? ld_msvc : ld_gcc " -s -Wl,--exclude-all-symbols";
            build();
            return EXIT_SUCCESS;
        } else if (equals(argv[1], "clean")) {
            listdir(bin_dir, cleanup);
            listdir(build_dir, cleanup);
            return EXIT_SUCCESS;
        } else if (equals(argv[1], "-h", "--help")) {
            ;
        } else {
            printf("Invalid target: %s\n", argv[1]);
        }
    } else {
        printf("Too many arguments\n");
    }
    printf("Usage: %s [debug|release|clean|-h|--help], default is debug\n", argv[0]);
    return EXIT_FAILURE;
}

```