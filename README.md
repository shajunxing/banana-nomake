# How to get rid of annoying build systems such as gmake, nmake, cmake

I don't like those build systems, I think they break their own belief "mechanism better than policy" and "keep it simple stupid". Why should I learn those configuration rules? They are not even turing complete programming languages, they are ugly and sucks. Since c compiler is almost fundermental of every operating systems, I wrap "refined primitives" into a single tiny portable header file so that I can write build script using c code. I am satisfied to make it simpler than those annoying rules, and customers will also be happy because they nolonger need to install those annoying softwares, all they need is type `gcc make.c && ./a.out` or in windows `cl make.c && make.exe`, that's enough.

Below are global constants I wrap in make.h:

|Name|Type|Description|
|-|-|-|
|compiler|enum|Compiler type, can be one of `msvc gcc`.|
|dllext|char *|File extension of linked shared library, e.g `.dll .so`|
|exeext|char *|File extension of linked executable, e.g `.exe`|
|objext|char *|File extension of compiled object, e.g `.obj .o`|
|os|enum|Operating system type, can be one of `windows posix`.|
|pathsep|char *|File system path seperator, , e.g `\\ /`|

And functions or macros:

|Name|Parameters|Return|Description|
|-|-|-|-|
|append|char **dest, char *src|void|Append `src` to end of `dest`, `dest` must be dynamically allocated.|
|concat|char *...|char *|Concatenate multiple strings, return string should be freed when used up.|
|endswith|char *str, char *suffix|int|Determine whether `str` ends with `suffix`.|
|equals|char *str1, char *str2|int|Determine whether `str1 str2` are equal.|
|join|char *sep, char *...|char *|Join multiple strings by given seperator `sep`, return string should be freed when used up.|
|listdir|char *dir, void (*callback)(char *, char *, char *)|void|Iterate all items in directory `dir`, whether `dir` ends with or without path seperator doesn't matter, for each item invoke `callback`, which takes 3 parameters: `dir`, `base` and `ext`, `dir` always ends with path seperator. If item is file, combination is complete file path, `ext` will be `""` if file has no extension. If is directory, `dir` will be subdirectory's full path, `base` and `ext` will be `NULL`.|
|max|double ...|double|Take one or more double values, returns maximum one.|
|mtime|char *...|double|Get one or more file modification utc time and returns latest one.|
|run|char *cmd|void|Print and run command line `cmd`. If return value is not 0, print error message and exit program.|
|startswith|char *str, char *prefix|char *|Determine whether `str` starts with `prefix`.|

Below is an example of make.c, compare to those sucking makefiles, isn't it quite simple?

```c
#include "make.h"

char *src_dir = NULL;
char *dll = NULL;
char *hdr = NULL;
double hdr_mtime = -DBL_MAX;
int debug = 0;
double latest_src_mtime = -DBL_MAX;
char *obj_files = NULL;

void compile(const char *dir, const char *base, const char *ext) {
    if (ext && equals(ext, ".c")) {
        char *src = concat(dir, base, ext);
        double src_mtime = mtime(src);
        char *obj = concat(dir, base, objext);
        if (src_mtime > latest_src_mtime) {
            latest_src_mtime = src_mtime;
        }
        append(&obj_files, obj);
        append(&obj_files, " ");
        if (max(hdr_mtime, src_mtime) > mtime(obj)) {
            char *cmd = compiler == msvc ? concat("cl.exe /nologo /c /O2 /MD /wd4819 /Fo", obj, " ", src) : concat("gcc -c -s -O3 -Wall -std=gnu2x -Wl,--exclude-all-symbols -static -static-libgcc -D NDEBUG -shared -D DLL -D EXPORT -o ", obj, " ", src);
            run(cmd);
            free(cmd);
        }
        free(obj);
        free(src);
    }
}

void build() {
    obj_files = (char *)calloc(1, 1);
    listdir(src_dir, compile); // compilation stage
    if (max(hdr_mtime, latest_src_mtime) > mtime(dll)) { // linking stage
        char *cmd = compiler == msvc ? concat("cl.exe /nologo /c /O2 /MD /wd4819 /Fo") : concat("gcc -s -O3 -Wall -std=gnu2x -Wl,--exclude-all-symbols -static -static-libgcc -D NDEBUG -shared -D DLL -D EXPORT -o ", dll, " ", obj_files);
        run(cmd);
        free(cmd);
    }
    free(obj_files);
}

void cleanup(const char *dir, const char *base, const char *ext) {
    if (ext && equals(ext, objext)) {
        char *obj = concat(dir, base, objext);
        remove(obj);
        free(obj);
    }
}

void clean() {
    listdir(src_dir, cleanup);
    remove(dll);
}

void install() {
    puts("install");
}

int main(int argc, char **argv) {
    src_dir = concat(".", pathsep, "src", pathsep);
    dll = concat(".", pathsep, "bin", pathsep, "var", dllext);
    hdr = concat(src_dir, pathsep, "var.h");
    hdr_mtime = mtime(hdr);
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
    free(hdr);
    free(dll);
    free(src_dir);
    return 0;
}

```
