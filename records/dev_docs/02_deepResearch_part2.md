# 2023.05.01-2023.05.10 worklog
## 工作进展
本阶段重点关注性能测试框架的搭建，准备CSR格式的数据集，并编写读写函数，确保能够将数据争取读取到内存中。思考设计一个好的类，尝试使用SIMD加速
## 测试框架搭建
本项目中的性能测试框架借鉴工程测试的思路，在profiling.hpp文件中定义宏，方便统一计数方法，具体实现如下
```cpp
#ifndef PROFILING_HPP
#define PROFILING_HPP

#include "env.hpp"
#include <chrono>

DEF_ENV_PARAM(PROFILING)

#define __TIC__(tag) auto tag##_start = std::chrono::steady_clock::now();

#define __TOC__(tag)                                                           \
  auto tag##_end = std::chrono::steady_clock::now();                           \
  if (ENV_PARAM(PROFILING))                                                    \
   std::cout<< "Execution time of [" << #tag << "]: "                        \
              << std::chrono::duration_cast<std::chrono::microseconds>(        \
                     tag##_end - tag##_start)                                  \
                     .count()                                                  \
              << "us" << std::endl;

#endif // PROFILING_HPP

```
## 引入golden思想

调用Eigen库函数进行矩阵乘法，计算结果与耗时是我们对比的标准，在本项目中引入golden的思想。为提高程序的易用性，编写测试脚本，运行`test_accuracy.sh`脚本会得到下面的结果，可以清晰的看到自己设计的算法与EIgen运行的结果对比，并且计算出来的结果会进行对比，如果计算结果正确，会打印log `Algorithm performs correctly!`
```
SPMV implemented by Eigen
Execution time of [READ_MATRIX]: 146479us
Execution time of [READ_VECTOR]: 983us
Execution time of [OP]: 41us
Execution time of [TOTAL]: 147576us
Sparse matrix dimensions: 5000x2000
SPMV implemented by ./spmv
Execution time of [READ_MATRIX]: 7653us
Execution time of [READ_VECTOR]: 978us
Execution time of [OP]: 72us
Execution time of [TOTAL]: 8755us

SPMV implemented by Eigen
Execution time of [READ_MATRIX]: 939191us
Execution time of [READ_VECTOR]: 2589us
Execution time of [OP]: 180us
Execution time of [TOTAL]: 942026us
Sparse matrix dimensions: 5000x5000
SPMV implemented by ./spmv
Execution time of [READ_MATRIX]: 19422us
Execution time of [READ_VECTOR]: 2442us
Execution time of [OP]: 102us
Execution time of [TOTAL]: 22014us

Algorithm performs correctly!

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

## SIMD加速
将for循环实现的数组元素relu函数用汇编实现，使用向量指令集一次性访问多个数据，减少数据的缓存数量，并用搭建好的测试框架测试SIMD的加速比。
### 代码实现
```
        .text
        .align 2
        .globl relu
     .type relu, @function
relu:
        srli.d          $t1,    $a1,    3
        andi            $t0,    $a1,    0x07
        addi.d          $t3,    $zero,  4
        xvldi           $xr0,   0
.LOOP_RELU:
        beq                     $t1,    $zero,  .REST_RELU
        xvld            $xr1,   $a0,    0
        addi.d          $t1,    $t1,    -1
        xvfmax.s        $xr3,   $xr1,   $xr0
        xvst            $xr3,   $a0,    0
        addi.d          $a0,    $a0,    32
        b                       .LOOP_RELU
.REST_RELU:
        beq                     $t0,    $zero,  .END_RELU
        xvld            $xr1,   $a0,    0
        xvfmax.s        $xr2,   $xr1,   $xr0
        blt                     $t0,    $t3,    .RES_STORE
        vst                     $vr2,   $a0,    0
        addi.d          $t0,    $t0,    -4
        xvpermi.d       $xr2,   $xr2,   0x4E
.RES_STORE:
        beq                     $t0,    $zero,  .END_RELU
        fst.s           $f2,    $a0,    0
        addi.d          $t0,    $t0,    -1
        vbsrl.v         $vr2,   $vr2,   4
        b                       .REST_RELU
.END_RELU:
```
### 遇到的问题
我们在测试SIMD实现的relu函数加速比时遇到了一个很奇怪的问题，下面时问题的描述：

汇编文件顺序
当我用g++ relu.cpp relu.S -o relu 和 g++ relu.S relu.cpp -o relu 分别编译以上文件时，前者功能正常，后者在relu.cpp中的len值较大时功能异常，出现段错误。
复现段错误
1.  g++ relu.S relu.cpp -o relu
2. ./relu 30

正常情况
1. g++ relu.cpp relu.S -o relu
2. ./relu 30
3. ./test_relu_acc.sh

重复汇编

当我远程连接龙芯板子时，假如relu.S文件写错，导致g++ relu.cpp relu.S -o relu 编译运行出现段错误，那么即使你用正确的relu.S重新进行编译，程序依然存在段错误。解决方法是断开与龙芯的ssh连接后重新连接，然后用正确的relu.S文件编译，程序的功能就可以正常了。e
代码
此程序的输入是进行relu计算的元素个数，也就是len
输出是两个文件1.txt和2.txt
cmath库函数得到的relu输出结果，保存在1.txt中，汇编实现的保存在2.txt中

解决方法

在代码最后加上jirl  $r0, $r1, 0x0 
