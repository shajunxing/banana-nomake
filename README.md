# How to get rid of annoying build systems such as gmake, nmake, cmake

This article is openly licensed via [CC BY-NC-ND 4.0](https://creativecommons.org/licenses/by-nc-nd/4.0/).

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

See example of [Banana JS](https://github.com/shajunxing/banana-js)