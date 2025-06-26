# Banana NoMake, only one .h required, direct write script with C, replace those annoying gmake, nmake, cmake...

This article is openly licensed via [CC BY-NC-ND 4.0](https://creativecommons.org/licenses/by-nc-nd/4.0/).

[English Version](README.md) | [Chinese Version](README_zhCN.md)

Project Address: <https://github.com/shajunxing/banana-nomake>

I don't like those build systems, I think they break their own belief "mechanism better than policy" and "keep it simple stupid". Why should one learn those ugly and rigid rules? Wouldn't a Turing-Complete programming language be better? Since C compiler is essential, encapsulate necessary functions into a header file, then I can happily write scripts in C, right? Most important points I summarized as follows: 

1. **Comparison of file timestamps**
2. **Serial and parallel execution of commands**

Customers would be happy too, as they won't need to install any additional build systems, they can simply type `gcc script.c && ./a.out` or `cl script.c && script.exe`, isn't it quite easy?

For example, in a certain project, source code is in `src` directory, since C language string literals support direct concatenation, it's really easy to define file names, directories, and command lines.

```c
#define src_dir "src" pathsep
// ...
#define var_h src_dir "var.h"
#define var_c src_dir "var.c"
// ...
#if compiler == msvc
    #define cc "cl /nologo /c /W3 /MD /Zp /utf-8 /std:clatest /O2 /Fo"
    #define ld "link /nologo /incremental:no /nodefaultlib /out:"
// ...
```

Dependency relationship in traditional makefile, like `example_exe: example_obj var_h js_data_h js_common_h`, basically just compares modification times of files before and after colon. If before is older than after, it runs following commands. So, if you write it directly in C, it becomes a lot clearer and more straightforward.

```c
if (mtime(example_exe) < mtime(example_obj, var_h, js_data_h, js_common_h)) {
    run(link_example);
}
```

Because C is Turing-Complete programming language, it can easily implement more complex and flexible logic, which is something those make systems can't compare to. Here's a complete example where I use `async()` and `await()` to execute commands parallely, and it's pretty straightforward in C. On the other hand, even if those make systems could pull it off, configuration rules would be all over the place and hard to understand.

```c
#include "../banana-nomake/make.h"

#define bin_dir "bin" pathsep
#define build_dir "build" pathsep
#define src_dir "src" pathsep
#define banana_script_src_dir ".." pathsep "banana-script" pathsep "src" pathsep
#define js_common_h banana_script_src_dir "js-common.h"
#define js_common_c banana_script_src_dir "js-common.c"
#define js_common_o build_dir "js_common" objext
#define js_data_h banana_script_src_dir "js-data.h"
#define js_data_c banana_script_src_dir "js-data.c"
#define js_data_o build_dir "js_data" objext
#define var_h src_dir "var.h"
#define var_c src_dir "var.c"
#define var_o build_dir "var" objext
#define example_c src_dir "example.c"
#define example_o build_dir "example" objext
#define example_exe bin_dir "example" exeext
#if compiler == msvc
    #define cc "cl /nologo /c /W3 /MD /Zp /utf-8 /std:clatest /O2 /Fo"
    #define ld "link /nologo /incremental:no /nodefaultlib /out:"
    #define extra_libs " msvcrt.lib libvcruntime.lib ucrt.lib kernel32.lib user32.lib"
#else
    #define cc "gcc -c -Wall -O3 -o "
    #define ld "gcc -fvisibility=hidden -fvisibility-inlines-hidden -static -static-libgcc -s -Wl,--exclude-all-symbols -o "
    #define extra_libs ""
#endif
#define compile_js_common cc js_common_o " " js_common_c
#define compile_js_data cc js_data_o " " js_data_c
#define compile_var cc var_o " " var_c
#define compile_example cc example_o " " example_c
#define link_example ld example_exe " " example_o " " var_o " " js_data_o " " js_common_o extra_libs

void build() {
    mkdir(bin_dir);
    mkdir(build_dir);
    // DON'T compare obj because it is generated asynchronously
    if (mtime(js_common_o) < mtime(js_common_h, js_common_c)) {
        async(compile_js_common);
    }
    if (mtime(js_data_o) < mtime(js_data_h, js_data_c, js_common_h)) {
        async(compile_js_data);
    }
    if (mtime(var_o) < mtime(var_h, var_c, js_data_h, js_common_h)) {
        async(compile_var);
    }
    if (mtime(example_o) < mtime(example_c, var_h, js_data_h, js_common_h)) {
        async(compile_example);
    }
    await();
    if (mtime(example_exe) < mtime(example_o, var_o, js_data_o, js_common_o)) {
        run(link_example);
    }
}

void cleanup(const char *dir, const char *base, const char *ext) {
    if (base) {
        char *file_name = concat(dir, base, ext);
        remove(file_name);
        free(file_name);
    } else {
        listdir(dir, cleanup);
        rmdir(dir);
    }
}

int main(int argc, char **argv) {
    if (argc == 1) {
        build();
        return EXIT_SUCCESS;
    } else if (argc == 2) {
        if (equals(argv[1], "clean")) {
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
    printf("Usage: %s [clean|-h|--help]\n", argv[0]);
    return EXIT_FAILURE;
}
```

Below are detailed API description:

|Constants|Description|
|-|-|
|`#define compiler`|Compiler type, can be one of enumeration `compiler_type` of `msvc` `gcc`.|
|`#define dllext`|File extension of shared library, e.g `".dll"` `".so"`|
|`#define exeext`|File extension of executable, e.g `".exe"`|
|`#define libext`|File extension of library, e.g `".lib"` `".a"`|
|`#define objext`|File extension of compiled object, e.g `".obj"` `".o"`|
|`#define os`|Operating system type, can be one of enumeration `os_type` of `windows` `posix`.|
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

