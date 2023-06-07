# 2023.05.21-2023.06.07 worklog
## 工作进展
本阶段解决负载不均衡带来的性能不稳定问题，参考现有的优化方法，在设计中引入一种基于线程数量的任务划分策略，使得各线程负载均衡，测试结果表明该方法有效解决性能不稳定的问题。编写技术文档，并进行迭代测试，进行项目最后的收尾工作。
## 基于线程数的任务划分策略
#### 负载均衡策略
在本项目中，为了充分利用CPU多核处理的优势，将计算任务按照线程数划分，将稀疏矩阵中所有的非零元素平均地划分到每个线程中，具体计算方式如下。

```math
n=\begin{cases} nzz/tnums&(0\leq id \lt tnums-1)\\nzz-(nzz/tnums)*(tnums-1)&(id= tnums-1)&\end{cases}
```

`nzz`表示矩阵中非零元素的数量，`id`表示线程标识号，`tnums`表示CPU线程个数。以8×8的稀疏矩阵为例，演示基于线程数量的任务划分策略的详细划分过程。下面的矩阵中共有34个非零元素，这意味着CSR存储格式下`values`、`col_index`数组大小是34。

![](https://markdown.liuchengtu.com/work/uploads/upload_54dacbf1539ad161a59c645016124d4f.png)

龙芯3C5000系列处理器右4个线程，tnums = 4，nzz = 34， 根据上面公式对各线程中要处理的`values`、`col_index`数组划分，就可以得到下面的结果

![](https://markdown.liuchengtu.com/work/uploads/upload_5f6e6405d7341188dd9fde5a1233a440.png)


![](https://markdown.liuchengtu.com/work/uploads/upload_1e610a4a39d636c6eb4563537932f5f6.png)

该任务划分策略将计算机需要处理的负载绝对均匀地分配给每个线程，并在每个线程处理后进行统一的边界数据处理，该方法显著减少了块间边界数据处理的次数。在该项目边界数据处理次数等于线程数。下面伪代码中可以看出各线程运行结束后，边界元素计算结果保存在中间结果数组`result_mid`中，并行加速结束后，统一处理边界数据，从而得到最终的运算结果向量。

```cpp
std::vector<float> multiply(const std::vector<float> &vector,
                            const MatrixCSR &matrix) {
    
    ...

    #pragma omp parallel
    {
    #pragma omp for schedule(static) nowait
    for (int i = 0; i < thread_nums; ++i) {
        thread_block1(i, start[i], end[i], start1[i], end1[i], row_ptr, col_index,
                    values, result, result_mid, vector);
    }
    }
    result[0] = result_mid[0];
    int sub;
    for (int i = 1; i < thread_nums; ++i) {
    sub = i << 1;
    int tmp1 = start[i];
    int tmp2 = end[i - 1];
    if (tmp1 == tmp2) {
        result[tmp1] += (result_mid[sub - 1] + result_mid[sub]);
    } else {
        result[tmp1] += result_mid[sub];
        result[tmp2] += result_mid[sub - 1];
    }

    ...
}
 
```

为保证程序正确执行，在任务划分的同时需要记录每一组非零向量左右边界元素在原矩阵中的行索引，同时要记录每一组非零元素左边界在`values`数组中的索引，为降低时间复杂度，在本项目中采用二分的思想确定左右边界的行索引信息。
## 项目迭代测试
`test_performance.sh`脚本使用了22个不同类型的稀疏矩阵用来评估程序相比常规的基于CSR格式的SPMV算法的加速比，记录算子运行的平均时间。用法为：
> bash test_performance.sh ${可执行程序路径} ${迭代次数}

例如，运行此命令`bash test_performance.sh ./spmv4 1`，脚本会执行1次spmv和spmv4这个程序，计算原始的spmv算法执行的平均时间，计算使用了负载均衡程序的spmv4的平均时间，计算spmv4相对于spmv的加速比。

使用**tmux**命令创建session，确保程序在后台正常运行，避免窗口关闭带来程序运行终端的问题