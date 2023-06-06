# 2023.05.11-2023.05.20 worklog
## 工作进展
本阶段分析使用SIMD优化实现基于龙芯向量指令集的double、float类型的数据spmv，并进行性能测试，发现性能测试得到的结果很不稳定，有时候比naive方法快，有时候比naive方法慢很多，并且navie算法比eigen慢很多，尝试定位原因，发现是参数值传递带来的性能损失；最后为了充分利用3C5000CPU资源，尝试多线程加速

## 针对float数据的SIMD优化
```
        .text
        .align 2
        .globl spmv_float
    .type spmv_float, @function
spmv_float:
    fsub.d      $f0,    $f0,    $f0
        srli.d          $t1,    $a0,    3
        andi            $t0,    $a0,    0x07
        addi.d          $t3,    $zero,  4
.LOOP_MULX:
        beq                 $t1,        $zero,  .RES_MULX
        xvld            $xr1,   $a1,    0
        addi.d          $a1,    $a1,    32
        xvpermi.q       $xr4,   $xr1,   1
        xvld        $xr2,   $a2,    0
        addi.d          $a2,    $a2,    32
        xvsllwil.du.wu                  $xr9,   $xr2,   2
    xvpermi.d                           $xr2,   $xr2,   0xb1
        xvsllwil.du.wu                  $xr10,  $xr2,   2
        xvpickve2gr.du                  $t2,    $xr9,   0
    fldx.s                  $f3,   $a3,    $t2
        xvpickve2gr.du              $t2,    $xr9,   1
    fldx.s                          $f6,    $a3,        $t2
        xvinsve0.w              $xr3,   $xr6,   1
        xvpickve2gr.du          $t2,    $xr10,  0
    fldx.s                      $f6,    $a3,    $t2
        xvinsve0.w              $xr3,   $xr6,   2
        xvpickve2gr.du                  $t2,    $xr10,  1
        fldx.s                      $f6,    $a3,        $t2
        xvinsve0.w              $xr3,   $xr6,   3
    vfmul.s                 $vr3,   $vr1,   $vr3
    xvpickve2gr.du          $t2,    $xr9,   2
        fldx.s                  $f7,   $a3,    $t2
        xvpickve2gr.du              $t2,    $xr9,   3
    fldx.s                          $f8,    $a3,        $t2
        xvinsve0.w              $xr7,   $xr8,   1
        xvpickve2gr.du                  $t2,    $xr10,  2
    fldx.s                          $f8,    $a3,        $t2
        xvinsve0.w              $xr7,   $xr8,   2
        xvpickve2gr.du                  $t2,    $xr10,   3
        fldx.s                      $f8,    $a3,        $t2
        xvinsve0.w              $xr7,   $xr8,   3
    vfmadd.s        $vr3,   $vr7,   $vr4,   $vr3
        fadd.s          $f0,    $f0,    $f3
        xvpickve.w      $xr5,   $xr3,   1
        fadd.s          $f0,    $f0,    $f5
        xvpickve.w      $xr5,   $xr3,   2
        fadd.s          $f0,    $f0,    $f5
        xvpickve.w      $xr5,   $xr3,   3
        fadd.s          $f0,    $f0,    $f5
        addi.d          $t1,     $t1,     -1
        b               .LOOP_MULX
.RES_MULX:
        xvld            $xr1,   $a1,    0
        xvld        $xr2,   $a2,    0
        xvpermi.q       $xr4,   $xr1,   1
        xvsllwil.du.wu                  $xr9,   $xr2,   2
    xvpermi.d                           $xr2,   $xr2,   0xb1
        xvsllwil.du.wu                  $xr10,  $xr2,   2
        blt                     $t0,    $t3,    .MULX_LESS4_1
        xvpickve2gr.du                  $t2,    $xr9,   0
    fldx.s                  $f3,   $a3,    $t2
        xvpickve2gr.du              $t2,    $xr9,   1
    fldx.s                          $f6,    $a3,        $t2
        xvinsve0.w              $xr3,   $xr6,   1
        vfmul.s                                 $vr3,   $vr1,   $vr3
        xvpickve2gr.du                  $t2,    $xr10,  0
    fldx.s                              $f6,    $a3,    $t2
        xvpickve2gr.du                  $t2,    $xr10,  1
        fldx.s                      $f7,    $a3,        $t2
        xvinsve0.w              $xr6,   $xr7,   1
        vpermi.w                                $vr1,   $vr1,   0x0e
        vfmadd.s                                $vr3,   $vr6,   $vr1,   $vr3
        fadd.s          $f0,    $f0,    $f3
        xvpickve.w      $xr5,   $xr3,   1
        fadd.s          $f0,    $f0,    $f5
        addi.d                  $t0,    $t0,    -4
        b                               .MULX_LESS4_2
.MULX_LESS4_1:
        beq                 $t0,        $zero,  .END
        xvpickve2gr.du                  $t2,    $xr9,   0
    fldx.s                  $f3,   $a3,    $t2
        fmadd.s                                 $f0,    $f3,    $f1,    $f0
        addi.d                                  $t0,    $t0,    -1
        beq                 $t0,        $zero,  .END
        xvpickve2gr.du              $t2,    $xr9,   1
    fldx.s                          $f3,    $a3,        $t2
        vpermi.w                                $vr1,   $vr1,   0x39
        fmadd.s                                 $f0,    $f3,    $f1,    $f0
        addi.d                                  $t0,    $t0,    -1
        beq                 $t0,        $zero,  .END
        xvpickve2gr.du                  $t2,    $xr10,  0
    fldx.s                              $f3,    $a3,    $t2
        vpermi.w                                $vr1,   $vr1,   0x39
        fmadd.s                                 $f0,    $f3,    $f1,    $f0
        addi.d              $t0,        $t0,    -1
        beq                 $t0,        $zero,  .END
.MULX_LESS4_2:
        beq                 $t0,        $zero,  .END
        xvpickve2gr.du              $t2,    $xr9,   2
        fldx.s                  $f7,   $a3,    $t2
        fmadd.s                                 $f0,    $f7,    $f4,    $f0
        addi.d                                  $t0,    $t0,    -1
        beq                 $t0,        $zero,  .END
        xvpickve2gr.du              $t2,    $xr9,   3
    fldx.s                          $f3,    $a3,        $t2
        vpermi.w                                $vr4,   $vr4,   0x39
        fmadd.s                                 $f0,    $f3,    $f4,    $f0
        addi.d                                  $t0,    $t0,    -1
        beq                 $t0,        $zero,  .END
        xvpickve2gr.du                  $t2,    $xr10,  2
    fldx.s                              $f3,    $a3,    $t2
        vpermi.w                                $vr4,   $vr4,   0x39
        fmadd.s                                 $f0,    $f3,    $f4,    $f0
        beq                 $t0,        $zero,  .END
.END:
        jr $ra
```

## 针对double数据的SIMD优化
```
        .text
        .align 2
        .globl spmv_loongson
        .type spmv_loongson, @function

spmv_loongson:
        fsub.d      $f0,    $f0,    $f0
        beq             $a0,    $zero,  .END
        srli.d          $t1,    $a0,    2
        andi            $t0,    $a0,    0x03

.LOOP_MULX:
        beq                     $t1,    $zero,  .REST_MULX
        xvld            $xr1,   $a1,    0
        addi.d          $a1,    $a1,    32
        xvld        $xr2,   $a2,    0
        addi.d          $a2,    $a2,    32
        b           .LOAD_VEC
.RET_LOOP:
        xvfmul.d        $xr4,   $xr1,   $xr3
        fadd.d          $f0,    $f0,    $f4
        xvpickve.d      $xr5,   $xr4,   1
        fadd.d          $f0,    $f0,    $f5
        xvpickve.d      $xr5,   $xr4,   2
        fadd.d          $f0,    $f0,    $f5
        xvpickve.d      $xr5,   $xr4,   3
        fadd.d          $f0,    $f0,    $f5
        addi.d          $t1,     $t1,   -1
        b               .LOOP_MULX

.REST_MULX:
        beq                     $t0,    $zero,  .END
        fld.d       $f1,    $a1,    0
        addi.d      $a1,    $a1,    8
        ld.d            $t2,    $a2,    0
        addi.d          $a2,    $a2,    8
        slli.d      $t2,        $t2,    3
        fldx.d          $f6,    $a3,    $t2
        fmul.d      $f7,    $f6,    $f1
        fadd.d      $f0,    $f0,    $f7
        addi.d          $t0,    $t0,    -1
        b           .REST_MULX

.END:
        jr $ra

.LOAD_VEC:
        xvslli.d                        $xr2,   $xr2,   3
        xvpickve2gr.du          $t2,    $xr2,   0
        fldx.d                      $f6,    $a3,    $t2
        xvinsve0.d          $xr3,   $xr6,   0
        xvpickve2gr.du          $t2,    $xr2,   1
        fldx.d                      $f6,    $a3,    $t2
        xvinsve0.d          $xr3,   $xr6,   1
        xvpickve2gr.du          $t2,    $xr2,   2
        fldx.d                  $f6,    $a3,    $t2
        xvinsve0.d          $xr3,   $xr6,   2
        xvpickve2gr.du          $t2,    $xr2,   3
        fldx.d                      $f6,    $a3,    $t2
        xvinsve0.d          $xr3,   $xr6,   3
        b                                       .RET_LOOP

```

## OpenMP多线程加速

OpenMP（Open Multi-Processing）是一种用于并行计算的编程模型和应用程序接口（API）。它采用的主要策略是通过将计算任务分解成多个子任务，然后将这些子任务分配给不同的线程来实现并行执行。通过特定的编译指令实现并行化，无需显式地管理线程的创建和同步。

在SPMV并行加速中，使用 
```#pragma omp parallel for```编译指令将外层循环并行化，使得每个线程可以计算矩阵的不同行，OpenMP会自动将循环迭代分配给不同的线程执行。由于矩阵不同行的计算是独立的，不涉及线程之间数据同步的问题，因此可以避免线程间数据竞争的问题。

```cpp
// Openmp + SIMD version
std::vector<float> multiply(const std::vector<float> &vector,
                            const MatrixCSR &matrix) {
  ...
  
  #pragma omp parallel for
  for (auto i = 0; i < rows; i++) {
      int start =  row_ptr[1];
      int end = row_ptr[i+1];
      int len = start - end ;
      result[i] = calculate(len, values+start, &col_index[start], vector);
      
  ...
      
  }
```

但多线程并行处理不可避免地带来负载不平衡的问题。这主要是由矩阵数据的分布特性决定的。如果矩阵的非零元素分布不均匀，某些行具有较多的非零元素，而其他行只有很少的非零元素。这导致某些线程处理更多的非零元素，而其他线程处理较少的非零元素，某些线程提前进入空闲态，导致CPU资源不能充分利用。这是我们后面重点关注的问题

## 遇到问题与解决方法

**值传递带来性能损失**
* 问题描述

我们写好naive方法与Eigen库方法后开始测试，naive方法测出来的耗时比Eigen方法慢两倍以上，检查代码逻辑也没有发现什么问题。

* 解决方法

一开始认为时Eigen库自动开启了向量指令加速，但我们实现SIMD优化后，测试结果还是比Eigen慢很多，去检查代码后发现原因在于函数接口调用时参数写成了值传递，值传递会进行整体的拷贝，占用大量的时间，改成引用传递后边解决了这个问题，并且每次耗时趋于稳定

