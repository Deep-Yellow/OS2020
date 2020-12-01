## 操作系统第三次作业

#### 题目要求如根目录下第三次作业描述所示

#### 基于Orange's 一个操作系统的实现附录代码 Chapter7/n

#### 对任务流程的理解

![](https://os-pichost.oss-cn-beijing.aliyuncs.com/OSimg1.png?Expires=1606824975&OSSAccessKeyId=TMP.3KdW7t74aCgSw3G8Mw8u8wPVBQtX2GsMdc7ic9hkyxQEcKxrWPbK3faUWAyG65xqwMda7LVUy8LG81HdBtrLxDNtTB6abX&Signature=13lG5SE5Ep69QITJlHOyNBkRNS8%3D)

1. task tty初始化tty 并调用tty_do_read
2. tty_do_read很简单，他只是调用keyboard_read的一个代理（其实是原书中为了搞多窗口用来识别现在的输入对应哪个窗口）
3. keyboard_read处理make code和break code，然后把识别结果告诉in_process
4. in_process得到键盘输入结果，并根据需要加工，比如F1~F12等特殊字符不做输出，然后根据要求，如果是特殊字符就丢掉不管，如果是可输出字符就把它放到一个缓冲区（通过put_key)
5. tty_doread就是检测到当前缓冲区还有东西没输出，那就输出缓冲区里的东西
6. tty_doread输出的方式是通过调用console.c里的out_char，函数如其名，一次输出一个字符

#### 碰到的问题：

​	启动后黑屏在终端输入c

​	如出现键盘输入无效的情况，要根据操作系统的版本（我用的Ubuntu13）来更改bochsrc里的keyboard_mapping

​	运行虚拟机无显示窗口的话可能是display_sdl的问题

​	Makefile里的镜像制作后的挂载点根据需要更改

#### 实现概要

	1. 设置一个标志位，并用作outchar函数的传参，用来区分当前的输入模式，如：普通输入；输入了TAB；第一次按ESC，进入查找；ESC后的回车->显示查找结果；第二次按ESC后，推出显示查找结果
 	2. 20s刷新：分别在tty初始化时和tty_write时通过getticks()记录时间戳
 	3. TAB：通过颜色来与普通空格进行区分
 	4. 查找：找到之前输入的：1. 字符每位都和带查找字符串匹配 2. （（字符的颜色是DEFAULT ） ||  TAB的情况）

