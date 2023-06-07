#include "profiling.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <thread>
#include <vector>

extern "C" double spmv_double(const std::size_t len, const double *values,
                                const std::size_t *col_index,
                                const double *vec);

namespace spmv {
class Triplet {
public:
  std::size_t row;
  std::size_t col;
  double value;
  Triplet(std::size_t row, std::size_t col, double value)
      : row(row), col(col), value(value) {}
};

class MatrixCSR {
private:
  std::vector<std::size_t> row_ptr;
  std::vector<std::size_t> col_index;
  std::vector<double> values;
  std::size_t rows;
  std::size_t cols;
  std::size_t nonzeros;

public:
  MatrixCSR(std::size_t rows, std::size_t cols, std::size_t nonzeros)
      : rows(rows), cols(cols), nonzeros(nonzeros) {
    row_ptr.resize(rows + 1);
    row_ptr[0] = 0;
    row_ptr[rows] = nonzeros;
    col_index.reserve(nonzeros);
    values.reserve(nonzeros);
  }

  void addElement(std::size_t row, std::size_t col, double value) {
    values.push_back(value);
    col_index.push_back(col);
    for (auto i = row + 1; i < rows; i++) {
      row_ptr[i]++;
    }
  }

  template <typename TripletIter>
  void setFromTriplets(TripletIter begin, TripletIter end) {
    col_index.reserve(nonzeros); // 预留存储空间
    values.reserve(nonzeros);

    // 统计每行非零元素个数
    std::vector<std::size_t> num_nonzeros_per_row(rows, 0);
    for (auto it = begin; it != end; ++it) {
      num_nonzeros_per_row[it->row]++;
    }

    // 计算每行非零元素的起始位置
    row_ptr[0] = 0;
    for (std::size_t i = 0; i < rows; ++i) {
      row_ptr[i + 1] = row_ptr[i] + num_nonzeros_per_row[i];
    }

    // 将元素插入到 CSR 矩阵中
    for (auto it = begin; it != end; ++it) {
      col_index.push_back(it->col);
      values.push_back(it->value);
    }
  }

  friend std::ostream &operator<<(std::ostream &os, const MatrixCSR &matrix) {
    os << "row_ptr: ";
    for (auto value : matrix.row_ptr) {
      os << value << " ";
    }
    os << std::endl;

    os << "col_index: ";
    for (auto value : matrix.col_index) {
      os << value << " ";
    }
    os << std::endl;

    os << "values: ";
    for (double value : matrix.values) {
      os << value << " ";
    }
    os << std::endl;

    os << "rows: " << matrix.rows << std::endl;
    os << "cols: " << matrix.cols << std::endl;
    os << "nonzeros: " << matrix.nonzeros << std::endl;

    return os;
  }

  std::size_t getRows() const { return rows; }
  std::size_t getCols() const { return cols; }
  std::size_t getNonzeros() const { return nonzeros; }
  const std::vector<std::size_t> &getRowPtr() const { return row_ptr; }
  const std::vector<std::size_t> &getColIndex() const { return col_index; }
  const std::vector<double> &getValues() const { return values; }
};

std::vector<double> readVectorFromFile(const std::string &filepath) {
  std::ifstream file(filepath);
  if (!file) {
    std::cerr << "Failed to open file: " << filepath << std::endl;
    return std::vector<double>();
  }
  std::size_t size;
  file >> size;
  std::vector<double> vector(size);
  for (std::size_t i = 0; i < size; i++) {
    file >> vector[i];
  }
  return vector;
}

MatrixCSR readMatrixFromFile(const std::string &filepath) {
  std::ifstream file(filepath);
  if (!file) {
    std::cerr << "Failed to open file: " << filepath << std::endl;
    abort();
  }
  std::size_t rows, cols, nonzeros;
  file >> rows >> cols >> nonzeros;
  std::vector<Triplet> tempElements; // 临时存储非零元素的向量
  tempElements.reserve(nonzeros);
  for (std::size_t i = 0; i < nonzeros; i++) {
    std::size_t row, col;
    double value;
    file >> row >> col >> value;
    tempElements.emplace_back(Triplet(row, col, value));
  }
  std::sort(tempElements.begin(), tempElements.end(),
            [](const Triplet &a, const Triplet &b) {
              if (a.row != b.row)
                return a.row < b.row;
              return a.col < b.col;
            });

  MatrixCSR matrix(rows, cols, nonzeros);
  matrix.setFromTriplets(tempElements.begin(), tempElements.end());

  return matrix;
}

void printData(const std::vector<double> &result,
               const std::string &outputFile) {
  std::ofstream resultFile(outputFile);
  if (!resultFile) {
    std::cerr << "Failed to open file: " << outputFile << std::endl;
    abort();
  }
  std::size_t count = result.size();
  for (std::size_t i = 0; i < count; i++) {
    if (i == 0) {
      resultFile << result[i];
    } else {
      resultFile << std::endl << result[i];
    }
  }

  resultFile.close();
}

// naive version
std::vector<double> multiply(const std::vector<double> &vector,
                             const MatrixCSR &matrix) {
  std::size_t cols = matrix.getCols();
  std::size_t rows = matrix.getRows();
  const std::vector<std::size_t> &row_ptr = matrix.getRowPtr();
  const std::vector<std::size_t> &col_index = matrix.getColIndex();
  const std::vector<double> &values = matrix.getValues();
  if (cols != vector.size()) {
    std::cerr << "Matrix and vector dimensions do not match." << std::endl;
    abort();
  }
  std::vector<double> result(rows);
  __TIC__(MUL)
  for (auto i = 0; i < rows; i++) {
    for (auto j = row_ptr[i]; j < row_ptr[i + 1]; j++) {
      result[i] += values[j] * vector[col_index[j]];
    }
  }
  __TOC__(MUL)

  return result;
}

std::vector<double> multiply2(const std::vector<double> &vector,
                              const MatrixCSR &matrix) {
  std::size_t rows = matrix.getRows();
  std::size_t cols = matrix.getCols();
  const std::vector<std::size_t> &row_ptr = matrix.getRowPtr();
  const std::vector<std::size_t> &col_index = matrix.getColIndex();
  const std::vector<double> &values = matrix.getValues();
  if (cols != vector.size()) {
    std::cerr << "Matrix and vector dimensions do not match." << std::endl;
    abort();
  }
  std::vector<double> result(rows, 0.0);
  for (auto i = 0; i < rows; i++) {
    auto start = row_ptr[i];
    auto end = row_ptr[i + 1];
    auto len = end - start;
    result[i] = spmv_double(len, values.data() + start,
                              col_index.data() + start, vector.data());
  }
  return result;
}

// multi-thread OpenMP version
std::vector<double> multiply3(const std::vector<double> &vector,
                              const MatrixCSR &matrix) {
  std::size_t cols = matrix.getCols();
  std::size_t rows = matrix.getRows();
  const std::vector<std::size_t> &row_ptr = matrix.getRowPtr();
  const std::vector<std::size_t> &col_index = matrix.getColIndex();
  const std::vector<double> &values = matrix.getValues();
  if (cols != vector.size()) {
    std::cerr << "Matrix and vector dimensions do not match." << std::endl;
    abort();
  }
  std::vector<double> result(rows);

#pragma omp parallel for
  for (auto i = 0; i < rows; i++) {
    for (auto j = row_ptr[i]; j < row_ptr[i + 1]; j++) {
      result[i] += values[j] * vector[col_index[j]];
    }
  }
  return result;
}

// // multi-thread OpenMP+vec version
std::vector<double> multiply4(const std::vector<double> &vector,
                              const MatrixCSR &matrix) {
  std::size_t rows = matrix.getRows();
  std::size_t cols = matrix.getCols();
  const std::vector<std::size_t> &row_ptr = matrix.getRowPtr();
  const std::vector<std::size_t> &col_index = matrix.getColIndex();
  const std::vector<double> &values = matrix.getValues();
  if (cols != vector.size()) {
    std::cerr << "Matrix and vector dimensions do not match." << std::endl;
    abort();
  }
  std::vector<double> result(rows, 0.0);
#pragma omp parallel for
  for (auto i = 0; i < rows; i++) {
    auto start = row_ptr[i];
    auto end = row_ptr[i + 1];
    auto len = end - start;
    result[i] = spmv_double(len, values.data() + start,
                              col_index.data() + start, vector.data());
  }
  return result;
}

// albus balance
std::size_t binary_search(const std::vector<std::size_t> &row_ptr,
                          std::size_t num, std::size_t end) {
  std::size_t l, r, h, t = 0;
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

void albus_balance(const MatrixCSR &matrix, std::vector<std::size_t> &start,
                   std::vector<std::size_t> &end,
                   std::vector<std::size_t> &start1,
                   std::vector<std::size_t> &end1, std::vector<double> &mid_ans,
                   std::size_t thread_nums) {
  std::size_t rows = matrix.getRows();
  std::size_t nonzeronums = matrix.getNonzeros();
  const std::vector<std::size_t> &row_ptr = matrix.getRowPtr();

  std::size_t tmp;
  start[0] = 0;
  start1[0] = 0;
  end[thread_nums - 1] = rows;
  end1[thread_nums - 1] = 0;
  std::size_t tt = nonzeronums / thread_nums;
  for (std::size_t i = 1; i < thread_nums; i++) {
    tmp = tt * i;
    start[i] = binary_search(row_ptr, tmp, rows);
    start1[i] = tmp - row_ptr[start[i]];
    end[i - 1] = start[i];
    end1[i - 1] = start1[i];
  }
}

void thread_block(std::size_t thread_id, std::size_t start, std::size_t end,
                  std::size_t start2, std::size_t end2,
                  const std::vector<std::size_t> &row_ptr,
                  const std::vector<std::size_t> &col_index,
                  const std::vector<double> &values,
                  std::vector<double> &mtx_ans, std::vector<double> &mid_ans,
                  const std::vector<double> &vector) {

  std::size_t start1, end1, num, Thread, i, j;
  double sum;
  switch (start < end) {
  case true: {
    mtx_ans[start] = 0.0;
    mtx_ans[end] = 0.0;
    start1 = row_ptr[start] + start2;
    start++;
    end1 = row_ptr[start];
    num = end1 - start1;
    Thread = thread_id << 1;
    sum = 0.0;
#pragma unroll(8)
    for (j = start1; j < end1; j++) {
      sum += values[j] * vector[col_index[j]];
    }
    mid_ans[Thread] = sum;
    start1 = end1;

    for (i = start; i < end; ++i) {
      end1 = row_ptr[i + 1];
      sum = 0.0;
#pragma simd
      for (j = start1; j < end1; j++) {
        sum += values[j] * vector[col_index[j]];
      }
      mtx_ans[i] = sum;
      start1 = end1;
    }
    start1 = row_ptr[end];
    end1 = start1 + end2;
    sum = 0.0;
#pragma unroll(8)
    for (j = start1; j < end1; j++) {
      sum += values[j] * vector[col_index[j]];
    }
    mid_ans[Thread | 1] = sum;
    return;
  }
  default: {
    mtx_ans[start] = 0.0;
    sum = 0.0;
    Thread = thread_id << 1;
    start1 = row_ptr[start] + start2;
    end1 = row_ptr[end] + end2;
#pragma unroll(8)
    for (j = start1; j < end1; j++) {
      sum += values[j] * vector[col_index[j]];
    }
    mid_ans[Thread] = sum;
    mid_ans[Thread | 1] = 0.0;
    return;
  }
  }
}

void thread_block(std::size_t thread_id, std::size_t start, std::size_t end,
                  std::size_t start2, std::size_t end2,
                  const std::vector<std::size_t> &row_ptr,
                  const std::vector<std::size_t> &col_index,
                  const std::vector<double> &values,
                  std::vector<double> &mtx_ans, std::vector<double> &mid_ans,
                  const std::vector<double> &vector) {

  std::size_t start1, end1, num, Thread, i;
  double sum;
  switch (start < end) {
  case true: {
    mtx_ans[start] = 0.0;
    mtx_ans[end] = 0.0;
    start1 = row_ptr[start] + start2;
    start++;
    end1 = row_ptr[start];
    num = end1 - start1;
    Thread = thread_id << 1;
    mid_ans[Thread] = spmv_double(num, values.data() + start1,
                                  col_index.data() + start1, vector.data());
    start1 = end1;

    for (i = start; i < end; ++i) {
      end1 = row_ptr[i + 1];
      num = end1 - start1;
      sum = spmv_double(num, values.data() + start1, col_index.data() +
      start1,
                        vector.data());
      mtx_ans[i] = sum;
      start1 = end1;
    }
    start1 = row_ptr[end];
    end1 = start1 + end2;
    mid_ans[Thread | 1] = spmv_double(end2, values.data() + start1,
                                      col_index.data() + start1,
                                      vector.data());
    return;
  }
  default: {
    mtx_ans[start] = 0.0;
    Thread = thread_id << 1;
    start1 = row_ptr[start] + start2;
    num = end2 - start2;
    mid_ans[Thread] = spmv_double(num, values.data() + start1,
                                  col_index.data() + start1, vector.data());
    mid_ans[Thread | 1] = 0.0;
    return;
  }
  }
}

// multi-thread OpenMP  && ALBUS version
std::vector<double> multiply5(const std::vector<double> &vector,
                              const MatrixCSR &matrix) {
  std::size_t cols = matrix.getCols();
  std::size_t rows = matrix.getRows();
  std::size_t nonzeronums = matrix.getNonzeros();

  std::size_t thread_nums = std::thread::hardware_concurrency();

  const std::vector<std::size_t> &row_ptr = matrix.getRowPtr();
  const std::vector<std::size_t> &col_index = matrix.getColIndex();
  const std::vector<double> &values = matrix.getValues();
  if (cols != vector.size()) {
    std::cerr << "Matrix and vector dimensions do not match." << std::endl;
    abort();
  }
  // mid_ans final_ans
  std::vector<double> result(rows + 1, 0.0);
  std::vector<double> result_mid(thread_nums * 2, 0.0);
  // store left boundary row index of every thread
  std::vector<std::size_t> start(thread_nums, 0);
  // store right boundary row index of every thread
  std::vector<std::size_t> end(thread_nums, 0);
  // store the count of values in row start[thread_id]
  std::vector<std::size_t> start1(thread_nums, 0);
  // store the count of values in row end[thread_id]
  std::vector<std::size_t> end1(thread_nums, 0);

  albus_balance(matrix, start, end, start1, end1, result_mid, thread_nums);

  std::size_t i;
#pragma omp parallel private(i)
  {
#pragma omp for schedule(static) nowait
    for (i = 0; i < thread_nums; ++i) {
      thread_block(i, start[i], end[i], start1[i], end1[i], row_ptr, col_index,
                   values, result, result_mid, vector);
    }
  }
  result[0] = result_mid[0];
  std::size_t sub;
  //  #pragma unroll(32)
  for (std::size_t i = 1; i < thread_nums; ++i) {
    sub = i << 1;
    std::size_t tmp1 = start[i];
    std::size_t tmp2 = end[i - 1];
    if (tmp1 == tmp2) {
      result[tmp1] += (result_mid[sub - 1] + result_mid[sub]);
    } else {
      result[tmp1] += result_mid[sub];
      result[tmp2] += result_mid[sub - 1];
    }
  }
  return result;
}

std::vector<double> multiply6(const std::vector<double> &vector,
                             const MatrixCSR &matrix) {
  std::size_t cols = matrix.getCols();
  std::size_t rows = matrix.getRows();
  const std::vector<std::size_t> &row_ptr = matrix.getRowPtr();
  const std::vector<std::size_t> &col_index = matrix.getColIndex();
  const std::vector<double> &values = matrix.getValues();
  
  if (cols != vector.size()) {
    std::cerr << "Matrix and vector dimensions do not match." << std::endl;
    abort();
  }

  std::vector<double> result(rows);

  for (auto i = 0; i < rows; i++) {
    for (auto j = row_ptr[i]; j < row_ptr[i + 1]; j++) {
      result[i] += values[j] * vector[col_index[j]];
    }
  }

  return result;
}

} // namespace spmv
