# 2023.04.21-2023.04.30 worklog
## 工作进展
本阶段分析SPMV性能瓶颈，学习使用gdb调试方法，学习具体的调试指令，包括设置断点、单步运行、监视、查看变量或寄存器中的值，成功写出汇编版本的relu函数，并用gdb单步调试解决`segmentation fault`的问题，完成spmv串行循环实现，尝试在板子上安装eigen库，作为计算结果的golden比较
## 性能瓶颈
### 访存效率瓶颈
稀疏矩阵是指矩阵中零值元素的个数远远多于非零元素个数，矩阵中的零值元素在参与向量乘法运算时不起作用，所以现有的稀疏矩阵存储格式大多只保存非零元素，并利用额外数组保存非零元素的位置信息，确保实现SPMV。
#### 内存访问频繁
具体计算时，每一个行中的非零元素都要与向量中对应位置的元素进行乘加操作，这需要进行大量**内存访问**操作。

CSR作为一种稀疏矩阵的存储结构，由3个数组构成：**值**(**values**)、**列索引**(**col_Index**)、**行索引**（**row_ptr**）。**values**数组表示稀疏矩阵**matrix**中的每一个非零元素，按照从左到右、从上到下的顺序存储非零元素，**col_ptr**数据表示每个非零元素在矩阵**matrix**中对应的列索引，**row_ptr**数组中第i个元素记录了前i-1行包含的非零元素的数量，比如**row_ptr[k]** 表示在行k之前，矩阵中所有元素的数目。

我们看下面基于CSR数据格式中SPMV串行循环实现，为进行一次乘加操作，我们需要通过内存访问获得参与计算的非零元素、该非零元素的列索引、以及向量中该索引对应的元素值，这需要3次内存访问的操作，且每次内存访问的地址都不连续，内存访问的时间远远超过CPU运算的时间，导致整体运算性能降低。基于CSR格式的SPMV是访存密集型的运算操作，其性能优化应当首先解决访存效率带来的瓶颈问题。在本项目中采用SIMD(单指令多数据流)对访存带来的瓶颈问题进行优化，以提升SPMV的运算性能。

```cpp
// naive version
std::vector<float> multiply(const std::vector<float> &vector,
                            const MatrixCSR &matrix) {
...
  for (auto i = 0; i < rows; i++) {
    for (auto j = row_ptr[i]; j < row_ptr[i + 1]; j++) {
      result[i] += values[j] * vector[col_index[j]];
    }
  }
...
}

```
#### 缓存命中率低
同时由于稀疏矩阵的非零元素通常分布在不同的行和列上，因此在计算 SPMV 时需要频繁地从内存中读取不同的数据块，而 CPU 的缓存大小是有限的，可能无法容纳整个数据集，因此**缓存命中率低**也会成为 SPMV 的性能瓶颈，总之，内存的访问效率会影响运算时间，这是影响SPMV性能的一大因素。
### 线程并行运算瓶颈
SPMV 的计算过程可以被视为一系列独立的向量乘法操作，因此可以通过并行计算来提高性能。使用并行加速SPMV计算同样会带来一系列问题，导致各线程无法充分利用CPU资源，从而限制了程序的性能提升。
#### 负载不均衡
由于大多数稀疏矩阵不同的行和列可能包含的非零元素数量差异很大，因此在并行计算时，各线程之间计算任务量有很大的差别，出现负载不均衡，某些线程的空闲状态导致CPU资源无法充分跟利用。在实际的多线程并行性中，整体的计算性能往往由于不平衡的负载而降低。因此，实现负载均衡是提高SPMV性能的另一个关键点。在本项目中采用矩阵分块技术合理分配各线程的计算任务，达到平衡负载的目的。
#### 额外的通信开销
SPMV将同一行中的矩阵乘法结果做累加，累加结果保存到结果向量中。为保证计算结果正确，多线程并行计算中的线程之间需要共享数据，如稀疏矩阵的行指针、列索引和非零元素值、中间结果等，而线程间数据共享需要通过内存访问或者消息传递等方式实现，这可能会带来额外的通信开销，在SPMV优化时应采用合适的策略尽可能降低线程通信开销。
 

## gdb指令使用方法
开发过程中，汇编指令直接对寄存器进行操作，并且设计内存中数据的存取，很容易出现非法地址寻址的问题，导致程序编译不通过、无法运行等问题，可见gdb调试是提升开发效率很重要的一部分，我们团队使用gdb调试relu.S，该汇编函数实现对数组的元素relu操作，将数组的地址和长度作为函数参数传递。下面对用到的调试命令进行总结
```
若想启动gdb调试，需要在编译时加上`-g`参数
example: g++ -g -o mytest test.cpp relu.S
启动gdb
gdb <your execute file>
example: gdb mytest
设置断点
b xxx.cpp:20    //在xxx.cpp文件的第20行设置断点
b main          //在main函数入口地址设置断点
查看断点
info b
取消断点
delete <id of the breakpoint>  // 查看断点命令课指导断点的id
运行程序
run  <the list of elements>  // 如果没有参数传递，只需写run/r，程序会运行到断点处
继续执行
c
单步执行
si
s                //s 不进入函数内部
查看变量
p varibale
查看寄存器
i r <name of register> 
i r all   //查看所有通用寄存器的值

```

## SPMV串行循环实现
```cpp
// naive version
std::vector<float> multiply(const std::vector<float> &vector,
                            const MatrixCSR &matrix) {
...
  for (auto i = 0; i < rows; i++) {
    for (auto j = row_ptr[i]; j < row_ptr[i + 1]; j++) {
      result[i] += values[j] * vector[col_index[j]];
    }
  }
...
}

```

## Eigen库的安装
本项目的`eigen.cpp`使用了`Eigen`库，按照如下流程按照到计算机上，同时我们使用的机器架构为`loongarch64`。
```
wget https://gitlab.com/libeigen/eigen/-/archive/3.4.0/eigen-3.4.0.tar.gz # 下载源代码
tar -zxvf eigen-3.4.0.tar.gz #解压
cd eigen-3.4.0
mkdir build && cd build
cmake ..
sudo make install 
```
