# COW(Copy On Write)
COW将fork系统调用的复制操作推迟到写操作时，提高了性能
1. 捕获store fault，完成COW操作
2. 建立页的引用计数机制
   - 分配一个表用于查询某页的引用计数
   - 加锁互斥访问这个表
   - 修改页的分配和释放函数
3. 利用页表项的RSW，标志页是否可以使用COW机制