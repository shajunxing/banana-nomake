# Banana NoMake，只需一个.h文件，直接使用C语言写脚本，取代那些令人生厌的gmake、nmake、cmake......

本文使用 [CC BY-NC-ND 4.0](https://creativecommons.org/licenses/by-nc-nd/4.0/) 许可。

[英文版](README.md) | [中文版](README_zhCN.md)

项目地址：<https://github.com/shajunxing/banana-make>

我不喜欢那些构建系统，我认为他们带头违反了他们自己制定的“机制优于策略”和“KISS”原则。为什么要学习那些丑陋死板的规则？图灵完备的编程语言不更好吗？既然C编译器是必备的，那么把必要的功能封装进一个头文件里面，我总结最核心的就这几条：一、**递归遍历文件目录**；二、**比较文件时间**；三、**串行、并行执行命令**，不就能开心地用C语言写脚本了？客户也很高兴，因为他们完全不需要安装额外的构建系统，只需要键入`gcc make.c && ./a.out`或者`cl make.c && make.exe`就行了，多方便？

简要指南：用`listdir`批量处理目录下面多个文件，用`max` `mtime`比较文件修改时间，用`append` `concat` `endswith` `equals` `format` `join` `startswith`处理字符串，用`async await run`执行命令。下面是详细的API定义：

API定义和范例，见英文文档。