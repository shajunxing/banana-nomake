# Banana NoMake，只需一个.h文件，直接使用C语言写脚本，取代那些令人生厌的gmake、nmake、cmake......

本文使用 [CC BY-NC-ND 4.0](https://creativecommons.org/licenses/by-nc-nd/4.0/) 许可。

[英文版](README.md) | [中文版](README_zhCN.md)

项目地址：<https://github.com/shajunxing/banana-nomake>

我不喜欢那些构建系统，我认为他们带头违反了他们自己制定的“机制优于策略”和“KISS”原则。为什么要学习那些丑陋死板的规则？图灵完备的编程语言不更好吗？既然C编译器是必备的，那么把必要的功能封装进一个头文件里面，不就能开心地用C语言写脚本了？我总结最核心的就这两条：

1. **比较文件时间**
2. **串行、并行执行命令**

客户也很高兴，因为他们完全不需要安装额外的构建系统，只需要键入`gcc 脚本.c && ./a.out`或者`cl 脚本.c && 脚本.exe`就行了，多方便？

比如某个项目，源代码位于`src`目录，因为C语言的字符串字面量支持直接拼接，所以可以很方便地定义出各文件、目录和命令行。

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

传统makefile的依赖关系，比如`example_exe: example_obj var_h js_data_h js_common_h`，其实就是比较冒号前后的文件的修改时间，如果前面的小于后面的，则执行接下来的命令，那么直接用C写出来，语义更清晰直观。

```c
if (mtime(example_exe) < mtime(example_obj, var_h, js_data_h, js_common_h)) {
    run(link_example);
}
```

因为C是图灵完备的编程语言，可以轻松实现更复杂更灵活的逻辑，这是那些make系统完全无法比的。以下是完整的例子，其中用到`async()` `await()`并发执行命令，用C写起来所见即所得。而那些make系统即使能够实现，配置规则也会晦涩难懂。

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

以下是详细的API说明：

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