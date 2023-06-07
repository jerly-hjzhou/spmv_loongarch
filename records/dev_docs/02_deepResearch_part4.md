# 2023.05.21-2023.06.07 worklog
## 工作进展
本阶段解决负载不均衡带来的性能不稳定问题，参考现有的优化方法，在设计中引入一种基于线程数量的任务划分策略，使得各线程负载均衡，测试结果表明该方法有效解决性能不稳定的问题。编写技术文档，并进行迭代测试，进行项目最后的收尾工作。

## 基于线程数的任务划分策略
我们阅读文献，希望从现有文献中找到灵感，来解决性能测试结果不稳定的问题，一方面需要在测试的时候增加迭代次数，另一方面我们准备参考论文中ALBUS的思想，对各线程进行负载均衡，尝试优化计算过程。

### 解决线程之间的通信问题
在不进行负载均衡之前，相同行非零元素的计算是在一个线程中完成的，因此可以认为线程之间是独立的，不涉及到数据的共享，因此不用考虑通信的问题。但现在线程之间处理的数据不是以行为单位了，不同每个线程处理的边界可能会属于同一行，这样需要线程之间配合才能得到正确答案。

为了减少通信开销，与基于CSR5格式的矩阵分块策略不同，我们选择采用一个数据保存边界处理结果，等到线程结束后串行处理边界元素所属行的计算结果。

为了实现上述操作，我们需要记录任务划分后边界元素所属的行索引，为了降低时间复杂度，我们选择二分查找法对数组**row_ptr**进行查找，将左边界保存在**start**数组中，将右边界保存在**end**数组中
```cpp
// use binary search to find the row index of each boundary element for each
// thread
int binary_search(const std::vector<int> &row_ptr, int num, int end) {
  int l, r, h, t = 0;
  l = 0, r = end;
  while (l <= r) {
    h = (l + r) >> 1;
    if (row_ptr[h] >= num) {
      r = h - 1;
    } else {
      l = h + 1;
      t = h;
    }
  }
  return t;
}


```
### 负载均衡策略
在本项目中，为了充分利用CPU多核处理的优势，将计算任务按照线程数划分，将稀疏矩阵中所有的非零元素平均地划分到每个线程中，具体计算方式如下。

```math
n=\begin{cases} nzz/tnums&(0\leq id \lt tnums-1)\\nzz-(nzz/tnums)*(tnums-1)&(id= tnums-1)&\end{cases}
```

`nzz`表示矩阵中非零元素的数量，`id`表示线程标识号，`tnums`表示CPU线程个数。以8×8的稀疏矩阵为例，演示基于线程数量的任务划分策略的详细划分过程。下面的矩阵中共有34个非零元素，这意味着CSR存储格式下`values`、`col_index`数组大小是34。
```cpp
// load balance  by the number of the threads
void albus_balance(const MatrixCSR &matrix, std::vector<int> &start,
                   std::vector<int> &end, std::vector<int> &start1,
                   std::vector<int> &end1, int thread_nums) {
  int rows = matrix.getRows();
  int nonzeronums = matrix.getNonzeros();
  const std::vector<int> &row_ptr = matrix.getRowPtr();

  int tmp;
  start[0] = 0;
  start1[0] = 0;
  end[thread_nums - 1] = rows;
  end1[thread_nums - 1] = 0;
  int tt = nonzeronums / thread_nums;
  for (int i = 1; i < thread_nums; i++) {
    tmp = tt * i;
    start[i] = binary_search(row_ptr, tmp, rows);
    start1[i] = tmp - row_ptr[start[i]];
    end[i - 1] = start[i];
    end1[i - 1] = start1[i];
  }
}
```

该任务划分策略将计算机需要处理的负载绝对均匀地分配给每个线程，并在每个线程处理后进行统一的边界数据处理，该方法显著减少了块间边界数据处理的次数。在该项目边界数据处理次数等于线程数。下面伪代码中可以看出各线程运行结束后，边界元素计算结果保存在中间结果数组`result_mid`中，并行加速结束后，统一处理边界数据，从而得到最终的运算结果向量。


为保证程序正确执行，在任务划分的同时需要记录每一组非零向量左右边界元素在原矩阵中的行索引，同时要记录每一组非零元素左边界在`values`数组中的索引，为降低时间复杂度，在本项目中采用二分的思想确定左右边界的行索引信息。
## 项目收获

历时两个月的项目到这里就接近尾声，在这个过程中我们实现了从0到1的突破，我们跑通了自己写好的汇编程序，深入理解汇编程序工作流程，对CPU线程间的调度更加熟悉。同时通过这次区域赛，也让我们队友之间的配合更加密切，收获了深厚的友谊。

我们深刻意识到工程项目追求的“标准”，定义好标准，定义好做事情的边界，才能更好的推动项目进行。这个比赛锻炼了我们的逻辑思维能力与工程能力，我们在项目过程中开始有意识地思考技术问题，确定技术标准。比如汇编程序的接口要如何定义、如何提高程序设计的通用性、如何实际一个好的类等等。

我们现在只是在使用loongson架构，未来更希望有机会参与国产化CPU的发展，在越来越多的地方看到国产CPU的应用。