# Uthread(实现用户态线程)
主要难度在切换上下文。包括通用寄存器，堆栈指针，PC等等
## 确定保存上下文的方案
```c
struct thread {
  char       stack[STACK_SIZE]; /* the thread's stack */
  int        state;             /* FREE, RUNNING, RUNNABLE */
};
struct thread all_thread[MAX_THREAD];
```
根据xv6预先定义的线程结构体，可以知道每个线程都有自己的栈，这个栈实际上是在程序的bss段。
由于riscv没有段寄存器，所以不需要修改段寄存器。

保存寄存器时，需要将所有寄存器都存放到一个安全的位置，我选的安全的位置是sp往下的一块内存（地址比sp低）
选这块内存的原因是
1. 根据函数跋和函数序，函数只会操作栈帧内的内存，栈帧之外的不会操作，所以是安全的
2. 不需要额外开辟一块专门保存寄存器的内存
3. 模仿x86的上下文切换，保存时将寄存器全部压栈，恢复时依次弹栈即可
    但是riscv作为精简指令集，没有指令能一次性完成压栈操作。
    随后我发现，修改sp是没有必要的!

采用以上的方案，恢复上下文时必须先恢复sp，才能恢复其他寄存器
所以必须额外开辟一块内存专门存放sp的值
```diff
struct thread {
+  uint64     sp;
  char       stack[STACK_SIZE]; /* the thread's stack */
  int        state;             /* FREE, RUNNING, RUNNABLE */
};
```
## 线程初次启动时的准备
参考操作系统启动时，以及exec系统调用为进程的启动准备环境，
线程启动时，主要是需要一个栈的环境，也就是堆栈指针
此外还需要准备pc跳转到的地址，也就是函数指针

