# 2023.04.21-2023.04.30 worklog
## 工作进展
本阶段分析SPMV性能瓶颈，学习使用gdb调试方法，学习具体的调试指令，包括设置断点、单步运行、监视、查看变量或寄存器中的值，成功写出汇编版本的relu函数，并用gdb单步调试解决`segmentation fault`的问题，完成spmv串行循环实现，尝试在板子上安装eigen库，作为计算结果的golden比较
## 汇编指令学习
* 我们小组希望通过简单的relu函数SIMD实现，快速上手龙芯基础、扩展向量指令集，所以按照提供的培训指导编写完成了SIMD版本的relu函数，并准备了数据集测试，大概估算了一下一下加速比，运行出来的结果显示加速比能达到4倍以上，这给了我们极大的信息，我们把龙芯lasx、lsx用到SPMV上，预计能起到很好的加速效果，于是我们先从double类型数据入手，开始尝试写SIMD实现的SPMV
### 解决汇编与cpp程序之间参数传递问题
* 首先我们解决了cpp程序跟汇编程序之间参数传递的问题，龙芯架构中ABI规定了参数传递的规范规定通用寄存器`a0 ~ a7`、浮点寄存器 `fa0 ~ fa7`和栈来传递参数，并通过寄存器`a0、a1`或者`f0、f1`将结果返回，如果寄存器数量不够用可以将要传递的数据存到栈中，通过栈指针传递数据。我们决定将相同行非零元素的乘加运算采用SIMD实现，分析完后发现函数的接口只需要传递计算非零元素的个数、values数组、row_ptr数组、col_index数组、vector数组，5个参数节能完成，返回值只有一个累加结果，所以在本次设计过程中无需使用栈指针来传参
### 解决程序实现时地址更新的问题
当我们把参数正确传递后，又遇到了新的问题，传进来的参数是数组指针，是数据在内存中存储的地址，vld指令每次都是基地址加上偏移量的方式去内存中取数据放到寄存器中，所以为了保证多次访问数据正常，我们需要对地址及时更新，地址更新就需要指导数据在内存中占多少个字节，数据是不是内存对齐的，数据是大端对齐还是小端对齐。查阅资料后了解到float数据占4字节，double数据占8字节，他们都是小端对齐的。这样如果用vld 一次取128位，取了4个float数、2个double数；如果用`xvld`命令一次取256位，相应的取出来数量也增加位原来的2倍。

* ！这里我们踩了一个坑，地址是64位的，每次更新地址的时候应该用`.d`后缀。例如
```
addi.d $a0, $a0, 32
```
我们当时忽略了这一点，用的`.wu`后缀，导致算出来的地址不在有效范围内，从而出现段错误，导致程序异常

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
