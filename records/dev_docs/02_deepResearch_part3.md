# 2023.05.11-2023.05.20 worklog
## 工作进展
本阶段分析使用SIMD优化实现基于龙芯向量指令集的double、float类型的数据spmv，并进行性能测试，发现性能测试得到的结果很不稳定，有时候比naive方法快，有时候比naive方法慢很多，并且navie算法比eigen慢很多，尝试定位原因，发现是参数值传递带来的性能损失；最后为了充分利用3C5000CPU资源，尝试多线程加速

## 针对float数据的SIMD优化

### 问题定位

* 问题描述： 我们小组实现了基于SIMD的SPMV，测试发现时间消耗并没有很好的提升，开始尝试定位问题。
在项目开发过程中，spmv关键计算步骤中获取vector向量中的元素是需要实现随机存取的，它不像values数组中的数据，一次访存就可以取到好几个有效数据，查阅指令集没有找到可进行随机存取的向量指令，也就是说没有办法减少vector数组的访问次数，于是我们想办法从减少指令条数上来下文章。
* 解决方法：我们发现col_index数组保存的是vector对应的索引值，需要乘以类型对应的字节数才是正确的地址偏移量，所以我们选择针对左移运算，获取col_index数组有效值的操作采用SIMD实现，优化后的代码如下
```
        xvsllwil.du.wu                  $xr9,   $xr2,   2
        xvpermi.d                       $xr2,   $xr2,   0xb1
        xvsllwil.du.wu                  $xr10,  $xr2,   2
        xvpickve2gr.du                  $t2,    $xr9,   0
        fldx.s                          $f3,   $a3,    $t2
        xvpickve2gr.du                  $t2,    $xr9,   1
        fldx.s                          $f6,    $a3,        $t2
        xvinsve0.w                      $xr3,   $xr6,   1
        xvpickve2gr.du                  $t2,    $xr10,  0
        fldx.s                          $f6,    $a3,    $t2
        xvinsve0.w                      $xr3,   $xr6,   2
        xvpickve2gr.du                  $t2,    $xr10,  1
        fldx.s                          $f6,    $a3,        $t2
        xvinsve0.w                      $xr3,   $xr6,   3
        vfmul.s                         $vr3,   $vr1,   $vr3
        xvpickve2gr.du                  $t2,    $xr9,   2
        fldx.s                          $f7,   $a3,    $t2
        xvpickve2gr.du                  $t2,    $xr9,   3
        fldx.s                          $f8,    $a3,    $t2

```
上面的汇编代码实现了将col_index数组中8个int索引值取到寄存器xr2，之后进行扩展后左移，结果分别保存在寄存器xr9和xr10中，这个是vector数组对应的地址偏移量，用于获取对应有效值



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

