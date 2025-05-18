# Banana Make，只用一个make.h文件，取代那些令人生厌的gmake、nmake、cmake......构建系统

本文使用 [CC BY-NC-ND 4.0](https://creativecommons.org/licenses/by-nc-nd/4.0/) 许可。

[英文版](README.md) | [中文版](README_zhCN.md)

项目地址：<https://github.com/shajunxing/banana-make>

我不喜欢那些构建系统，我认为他们带头违反了他们自己制定的“机制优于策略”和“KISS”原则。为什么要学习那些丑陋死板的规则？图灵完备的编程语言不更好吗？既然C编译器是必备的，那么把必要的功能封装进一个头文件里面，我总结最核心的就这几条：一、**递归遍历文件目录**；二、**比较文件时间**；三、**串行、并行执行命令**，不就能开心地用C语言写脚本了？客户也很高兴，因为他们完全不需要安装额外的构建系统，只需要键入`gcc make.c && ./a.out`或者`cl make.c && make.exe`就行了，多方便？

简要指南：用`listdir`批量处理目录下面多个文件，用`max mtime`比较文件修改时间，用`append concat endswith equals join startswith`处理字符串，用`async await run`执行命令。下面是详细的API定义：

API定义，见英文文档，下面是一个范例：

```c
#include "../banana-make/make.h"

#define prefix "js"
#define bin_dir "bin" pathsep
#define build_dir "build" pathsep
#define src_dir "src" pathsep
#define examples_dir "examples" pathsep
#define private_dir "private" pathsep
#define library_header_file src_dir prefix ".h"
#define dll_file_name prefix dllext
#define dll_file_path bin_dir dll_file_name
#define lib_file bin_dir prefix libext
char *cc_msvc = NULL;
char *cc_gcc = NULL;
char *link_msvc = NULL;
char *link_gcc = NULL;
double library_header_mtime = -DBL_MAX;
char *library_obj_files = NULL;
int library_link_required = false;

void compile_file(const char *c_file, const char *obj_file) {
    char *cmd = compiler == msvc ? concat(cc_msvc, " /DDLL /DEXPORT /Fo", obj_file, " ", c_file) : concat(cc_gcc, " -D DLL -D EXPORT -o ", obj_file, " ", c_file);
    async(cmd);
    free(cmd);
}

void compile_library(const char *dir, const char *base, const char *ext) {
    if (ext && equals(ext, ".c")) {
        char *c_file = concat(dir, base, ext);
        char *h_file = concat(dir, base, ".h");
        char *obj_file = concat(build_dir, base, objext);
        append(&library_obj_files, " ", obj_file);
        if (max(library_header_mtime, mtime(c_file), mtime(h_file)) > mtime(obj_file)) {
            compile_file(c_file, obj_file);
            library_link_required = true;
        }
        free(obj_file);
        free(h_file);
        free(c_file);
    }
}

void compile_executables(const char *dir, const char *base, const char *ext) {
    if (ext && equals(ext, ".c")) {
        char *c_file = concat(dir, base, ext);
        char *obj_file = concat(build_dir, base, objext);
        if (mtime(c_file) > mtime(obj_file) || library_link_required) {
            compile_file(c_file, obj_file);
        }
        free(obj_file);
        free(c_file);
    }
}

void link_executables(const char *dir, const char *base, const char *ext) {
    if (ext && equals(ext, ".c")) {
        char *obj_file = concat(build_dir, base, objext);
        char *exe_file = concat(bin_dir, base, exeext);
        if (mtime(obj_file) > mtime(exe_file) || library_link_required) {
            char *cmd;
            cmd = compiler == msvc ? concat(link_msvc, " /out:", exe_file, " ", obj_file, " ", lib_file) : concat(link_gcc, " -o ", exe_file, " ", obj_file, " -L", bin_dir, " -l:", dll_file_name);
            async(cmd);
            free(cmd);
        }
        free(exe_file);
        free(obj_file);
    }
}

void build() {
    library_header_mtime = mtime(library_header_file);
    library_obj_files = (char *)calloc(1, 1);
    listdir(src_dir, compile_library);
    listdir(examples_dir, compile_executables);
    listdir(private_dir, compile_executables);
    await();
    if (library_link_required || mtime(dll_file_path) == 0) {
        char *cmd = compiler == msvc ? concat(link_msvc, " /dll /out:", dll_file_path, library_obj_files) : concat(link_gcc, " -shared -o ", dll_file_path, library_obj_files);
        async(cmd);
        free(cmd);
    }
    free(library_obj_files);
    await();
    listdir(examples_dir, link_executables);
    listdir(private_dir, link_executables);
    await();
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
    enum {
        debug,
        ndebug,
        release,
        clean,
        install,
        help
    } target;
    if (argc == 1) {
        target = debug;
    } else if (argc == 2) {
        if (equals(argv[1], "debug")) {
            target = debug;
        } else if (equals(argv[1], "ndebug")) {
            target = ndebug;
        } else if (equals(argv[1], "release")) {
            target = release;
        } else if (equals(argv[1], "clean")) {
            target = clean;
        } else if (equals(argv[1], "install")) {
            target = install;
        } else {
            target = help;
        }
    } else {
        target = help;
    }
    switch (target) {
    case debug:
        cc_msvc = "cl /nologo /c /W3 /MD";
        cc_gcc = "gcc -c -Wall -std=gnu2x";
        link_msvc = "link /nologo /debug";
        link_gcc = "gcc -fvisibility=hidden -fvisibility-inlines-hidden -static -static-libgcc";
        build();
        return EXIT_SUCCESS;
    case ndebug:
        cc_msvc = "cl /nologo /c /W3 /MD /DNDEBUG";
        cc_gcc = "gcc -c -Wall -std=gnu2x -DNDEBUG";
        link_msvc = "link /nologo /debug";
        link_gcc = "gcc -fvisibility=hidden -fvisibility-inlines-hidden -static -static-libgcc";
        build();
        return EXIT_SUCCESS;
    case release:
        cc_msvc = "cl /nologo /c /O2 /W3 /MD /DNDEBUG";
        cc_gcc = "gcc -c -O3 -Wall -std=gnu2x -DNDEBUG";
        link_msvc = "link /nologo";
        link_gcc = "gcc -s -Wl,--exclude-all-symbols -fvisibility=hidden -fvisibility-inlines-hidden -static -static-libgcc";
        build();
        return EXIT_SUCCESS;
    case clean:
        listdir(bin_dir, cleanup);
        listdir(build_dir, cleanup);
        return EXIT_SUCCESS;
    case install:
        puts("Install");
        return EXIT_SUCCESS;
    default:
        printf("Usage: %s [debug|ndebug|release|clean|install]\n", argv[0]);
        return EXIT_FAILURE;
    }
}
```