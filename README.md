# Banana Make, using only one make.h file, replaces those annoying build systems like gmake, nmake, cmake...

This article is openly licensed via [CC BY-NC-ND 4.0](https://creativecommons.org/licenses/by-nc-nd/4.0/).

[English Version](README.md) | [Chinese Version](README_zhCN.md)

Project Address: <https://github.com/shajunxing/banana-make>

I don't like those build systems, I think they break their own belief "mechanism better than policy" and "keep it simple stupid". Why should one learn those ugly and rigid rules? Wouldn't a Turing-Complete programming language be better? Since C compiler is essential, encapsulate necessary functions into a header file, most important points I summarized as follows: 1. **Recursive traversal of file and directories**; 2. **Comparison of file timestamps**; 3. **Serial and parallel execution of commands**, then I can happily write scripts in C, right? Customers would be happy too, as they won't need to install any additional build systems, they can simply type `gcc make.c && ./a.out` or `cl make.c && make.exe`, isn't it quite easy?

Brief guide: use `listdir` to batch process multiple files in a directory, use `max mtime` to compare file modification times, use `append concat endswith equals join startswith` to handle strings, and use `async await run` to execute commands. Below are the detailed API definitions:

|Constants|Description|
|-|-|
|const enum compiler_type compiler|Compiler type, can be one of `msvc gcc`.|
|#define dllext|File extension of shared library, e.g `".dll" ".so"`|
|#define exeext|File extension of executable, e.g `".exe"`|
|#define libext|File extension of library, e.g `".lib" ".a"`|
|#define objext|File extension of compiled object, e.g `".obj" ".o"`|
|const enum os_type os|Operating system type, can be one of `windows posix`.|
|#define pathsep|File system path seperator, , e.g `"\\" "/"`|

|Functions|Description|
|-|-|
|void append(char **dest, ...)|Append multiple strings sequentially to end of `dest`, `dest` must be dynamically allocated.|
|void async(const char *cmd)|Parallel run command line `cmd`. Maximum number of workers equals to num of cpu cores. If return value is not 0, print error message and exit program.|
|void await()|Wait for all workers to finish.|
|char * concat(...)|Concatenate multiple strings, return string should be freed when used up.|
|#define countof(__arg_0)|Calculate number of elements in static array.|
|bool endswith(const char *str, const char *suffix)|Determine whether `str` ends with `suffix`.|
|bool equals(const char *str1, const char *str2)|Determine whether `str1 str2` are equal.|
|char * join(char *sep, ...)|Join multiple strings by given seperator `sep`, return string should be freed when used up.|
|void listdir(const char *dir, void (*callback)(const char *dir, const char *base, const char *ext))|Iterate all items in directory `dir`, whether `dir` ends with or without path seperator doesn't matter, for each item invoke `callback`, set 3 parameters: `dir` always ends with path seperator. If item is file, combination is complete file path, `ext` will be `""` if file has no extension. If is directory, `dir` will be subdirectory's full path, `base` and `ext` will be `NULL`.|
|double max(...)|Take one or more double values, returns maximum one.|
|double mtime(...)|Get one or more file modification utc time and returns latest one, value for non-existent file is -DBL_MAX|
|void run(const char *cmd)|Run command line `cmd`. If return value is not 0, print error message and exit program.|
|bool startswith(const char *str, const char *prefix)|Determine whether `str` starts with `prefix`.|

Here's an example:

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