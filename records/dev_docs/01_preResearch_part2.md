# 2023.04.11-2023.04.20 worklog
## 工作进展
本阶段主要进行开发环境的搭建与汇编语言的学习与使用，尝试在开发板上跑通一个最简单的c++程序，确保开发环境是好的，由于之前没有使用过汇编语言开发，本阶段用汇编语言实现基本的for循环，从中理解运算、移位、访存指令的具体使用方法
## 环境搭建
本次开发需要有gcc开发编译环境，但我们拿到板子的时候是没有的，用在linux上安装gcc的方式进行安装：
```
yum install gcc
yum install g++
```
输入密码后总是提示`permission denied`, 即使命令前面加上`sudo`也不行，之后直接切换到root用户登录，重新执行上面的命令就可以安装了，安装完成后，写了一个简单的cpp程序来验证环境是否安装成功
```
// in test.cpp
#include <iostream>
using namespace std;

int main()
{
    cout<<"hello loongson"<<endl;
    return 0;
}
```
编译测试程序并运行
```
g++ -o envtest test.cpp

./envtest
```
程序能够顺利执行，至此认为环境搭建成功。为了防止在开发过程中误删系统文件，后续开发仍然切换到普通用户下进行。

## 基于loongson Arch 汇编指令学习
参考资料：https://github.com/loongson/LoongArch-Documentation/releases/latest/download/LoongArch-Vol1-v1.02-CN.pdf
我们队伍针对提供的技术文档，总结常用的指令使用说明，总结内容在该链接下：https://spmv.yuque.com/org-wiki-spmv-bw43uy/bgtx7o/es0etw8w24zr0w47?singleDoc# 《龙芯架构指令集》
该链接中部分内容如下：
|指令| 类型| 使用说明|
|---|---|---|
|srli.d $rd,  $rj, ui6	|移位|将通用寄存器rj中[63:0]位数据逻辑右移{ui6}位，移位结果写入通用寄存器rd中|
|fld.d $fd,  $rj,  si12	|访存|从内存（$rj + 12位符号扩展立即数）取回一个8B的数据写入浮点寄存器fd。
## 实现基于汇编的加法运算
### 代码
```
    .text
    .align 2
    .globl add_f
    .type add_f, @function
add_f:
    add.w   $a0,    $a0,    $a1
    add.w   $a0,    $a0,    $a2
    add.w   $a0,    $a0,    $a3
    jr      $r1

```
### 运行结果
```
5.03131e-314
```

## 确定测试数据集
我们从[ Sparse Suite Collection](https://sparse.tamu.edu/),选择了22个具有代表性的稀疏矩阵。Sparse Suite Collection收集了大量在实际场景应用广泛的稀疏矩阵。这些矩阵集广泛用于稀疏矩阵算法的开发和性能评估，因为人工生成的矩阵会对实验产生一些误导性。 `test_performance`脚本使用的数据集下载地址点击[这里](https://pan.baidu.com/s/1xqiqJ3GySV2QYSnj4xEhYA?pwd=c57w)。数据集中的稀疏矩阵特点见下表。括号中的内容是矩阵名字的简称，用于画性能比较图，方便作为横坐标。


| 矩阵名称 | 行 x 列 | 非零元素个数 | 稀疏度 |
| --- | --- | --- | --- |
| af_shell1(**afs**) | 504855 x 504855 | 9046868 | 3.55e-05 |
| amazon0601(**ama**) | 403394 x 403394 | 3387388 | 2.08e-05 |
| as-Skitter(**ass**) | 1696415 x 1696415 | 11095298 | 3.86e-06 |
| ASIC_680k(**asi**) | 682862 x 682862 | 3871773 | 8.303e-06 |
| boneS10(**bon**) | 914898 x 914898 | 55468422 | 6.63e-05 |
| com-Youtube(**com**) | 1134890 x 1134890 | 5975248 | 4.639e-06 |
| delaunay_n19(**del**) | 524288 x 524288 | 3145646 | 1.14e-05 |
| FEM_3D_thermal2(**fem**) | 147900 x 147900 | 3489300 | 1.59e-04 |
| hugetric-00020(**hug**) | 7122792 x 7122792 | 21361554 | 4.21e-07 |
| in-2004(**in**) | 1382908 x 1382908 | 16917053 | 8.846e-06 |
| ldoor(**ldr**) | 952203 x 952203 | 23737339 | 2.62e-05 |
| mc2depi(**mc**) | 525825 x 525825 | 2100225 | 7.59e-06 |
| memchip(**mem**) | 2707524 x 2707524 | 14810202 | 2.02e-06 |
| parabolic_fem(**par**) | 525825 x 525825 | 3674625 | 1.33e-05 |
| pkustk14(**pku**) | 151926 x 151926 | 14836504 | 6.428e-04 |
| poisson3Db(**poi**) | 85623 x 85623 | 2374949 | 3.24e-04 |
| rajat31(**raj**) | 4690002 x 4690002 | 20316253 | 9.24e-07 |
| roadNet-TX(**roa**) | 1393383 x 1393383 | 3843320 | 1.98e-06 |
| sx-stackoverflow(**sxs**) | 2601977 x 2601977 | 36233450 | 5.352e-06 |
| thermomech_dK(**the**) | 204316 x 204316 | 2846228 | 6.81e-05 |
| web-Google(**wbg**) | 916428 x 916428 | 5105039 | 6.08e-06 |
| webbase-1M(**wbs**) | 1000005 x 1000005 | 3105536 | 3.106e-06 |
