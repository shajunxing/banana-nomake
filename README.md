# How to get rid of annoying build systems such as gmake, nmake, cmake

I don't like those build systems, I think they break their own belief "mechanism better than policy" and "keep it simple stupid". Why should I learn those configuration rules? They are not even turing complete programming languages, they are ugly and sucks. Since c compiler is almost fundermental of every operating systems, I wrap "refined primitives" into a single tiny portable header file so that I can write build script using c code. I am satisfied to make it simpler than those annoying rules, and customers will also be happy because they nolonger need to install those annoying softwares, all they need is type `gcc make.c && ./a.out` or in windows `cl make.c && make.exe`, that's enough.

Below are global constants/definitions I wrap in make.h:

|Name|Type|Description|
|-|-|-|
|compiler|enum|Compiler type, can be one of `msvc gcc`.|
|dllext|char *|File extension of shared library, e.g `.dll .so`|
|exeext|char *|File extension of executable, e.g `.exe`|
|libext|char *|File extension of library, e.g `.lib .a`|
|objext|char *|File extension of compiled object, e.g `.obj .o`|
|os|enum|Operating system type, can be one of `windows posix`.|
|pathsep|char *|File system path seperator, , e.g `\\ /`|

And functions/macros:

|Name|Parameters|Return|Description|
|-|-|-|-|
|append|char **dest, char *...|void|Append multiple strings sequentially to end of `dest`, `dest` must be dynamically allocated.|
|concat|char *...|char *|Concatenate multiple strings, return string should be freed when used up.|
|countof|array|size_t|Calculate number of elements in static array.|
|endswith|char *str, char *suffix|int|Determine whether `str` ends with `suffix`.|
|equals|char *str1, char *str2|int|Determine whether `str1 str2` are equal.|
|join|char *sep, char *...|char *|Join multiple strings by given seperator `sep`, return string should be freed when used up.|
|listdir|char *dir, void (*callback)(char *, char *, char *)|void|Iterate all items in directory `dir`, whether `dir` ends with or without path seperator doesn't matter, for each item invoke `callback`, which takes 3 parameters: `dir`, `base` and `ext`, `dir` always ends with path seperator. If item is file, combination is complete file path, `ext` will be `""` if file has no extension. If is directory, `dir` will be subdirectory's full path, `base` and `ext` will be `NULL`.|
|max|double ...|double|Take one or more double values, returns maximum one.|
|mtime|char *...|double|Get one or more file modification utc time and returns latest one.|
|run|char *cmd|void|Print and run command line `cmd`. If return value is not 0, print error message and exit program.|
|startswith|char *str, char *prefix|char *|Determine whether `str` starts with `prefix`.|

Below is an example of make.c, it certainly handles file modification time correctly and compiles incrementally correctly, compare to those sucking makefiles, isn't it quite simple?

```c
#include "../banana-make/make.h"

#define prefix "js"
#define bin_dir "bin" pathsep
#define build_dir "build" pathsep
#define examples_dir "examples" pathsep
#define src_dir "src" pathsep
#define header_file src_dir prefix ".h"
#define dll_file_name prefix dllext
#define dll_file_path bin_dir dll_file_name
#define lib_file bin_dir prefix libext
int debug = 0;
double header_mtime = -DBL_MAX;
char *obj_files = NULL;
char *cc_msvc = NULL;
char *cc_gcc = NULL;
char *link_msvc = NULL;
char *link_gcc = NULL;
int link_required = 0;

void compile_file(const char *src_file, const char *obj_file) {
    char *cmd = compiler == msvc ? concat(cc_msvc, " /DDLL /DEXPORT /Fo", obj_file, " ", src_file) : concat(cc_gcc, " -D DLL -D EXPORT -o ", obj_file, " ", src_file);
    run(cmd);
    free(cmd);
}

void compile(const char *dir, const char *base, const char *ext) {
    if (ext && equals(ext, ".c")) {
        char *src_file = concat(dir, base, ext);
        double src_mtime = mtime(src_file);
        char *obj_file = concat(build_dir, base, objext);
        append(&obj_files, " ", obj_file);
        if (max(header_mtime, src_mtime) > mtime(obj_file)) {
            compile_file(src_file, obj_file);
            link_required = 1;
        }
        free(obj_file);
        free(src_file);
    }
}

void build_example(const char *dir, const char *base, const char *ext) {
    if (ext && equals(ext, ".c")) {
        char *src_file = concat(dir, base, ext);
        double src_mtime = mtime(src_file);
        char *obj_file = concat(build_dir, base, objext);
        char *exe_file = concat(bin_dir, base, exeext);
        if (max(header_mtime, src_mtime) > mtime(exe_file) || link_required) {
            char *cmd;
            compile_file(src_file, obj_file);
            cmd = compiler == msvc ? concat(link_msvc, " /out:", exe_file, " ", obj_file, " ", lib_file) : concat(link_gcc, " -o ", exe_file, " ", obj_file, " -L", bin_dir, " -l:", dll_file_name);
            run(cmd);
            free(cmd);
        }
        free(exe_file);
        free(obj_file);
        free(src_file);
    }
}

void build() {
    header_mtime = mtime(header_file);
    obj_files = (char *)calloc(1, 1);
    if (debug) {
        cc_msvc = "cl /nologo /c /W3 /MD";
        cc_gcc = "gcc -c -Wall";
        link_msvc = "link /nologo /debug";
        link_gcc = "gcc -fvisibility=hidden -fvisibility-inlines-hidden -static -static-libgcc";
    } else {
        cc_msvc = "cl /nologo /c /O2 /W3 /MD";
        cc_gcc = "gcc -c -O3 -Wall";
        link_msvc = "link /nologo";
        link_gcc = "gcc -s -Wl,--exclude-all-symbols -fvisibility=hidden -fvisibility-inlines-hidden -static -static-libgcc";
    }
    listdir(src_dir, compile); // compilation stage
    if (link_required) { // linking stage
        char *cmd = compiler == msvc ? concat(link_msvc, " /dll /out:", dll_file_path, obj_files) : concat(link_gcc, " -shared -o ", dll_file_path, obj_files);
        run(cmd);
        free(cmd);
    }
    free(obj_files);
    listdir(examples_dir, build_example); // examples
}

void cleanup(const char *dir, const char *base, const char *ext) {
    if (base) {
        char *file_name = concat(dir, base, ext);
        printf("Delete %s\n", file_name);
        remove(file_name);
        free(file_name);
    } else {
        listdir(dir, cleanup);
        printf("Delete %s\n", dir);
        rmdir(dir);
    }
}

void clean() {
    listdir(bin_dir, cleanup);
    listdir(build_dir, cleanup);
}

void install() {
    puts("Install");
}

int main(int argc, char **argv) {
    if (argc == 1) {
        build();
    } else if (argc == 2) {
        if (equals(argv[1], "-h") || equals(argv[1], "--help")) {
            printf("Usage: %s [debug|clean|install]\n", argv[0]);
        } else if (equals(argv[1], "debug")) {
            debug = 1;
            build();
        } else if (equals(argv[1], "clean")) {
            clean();
        } else if (equals(argv[1], "install")) {
            install();
        }
    }
}

```
