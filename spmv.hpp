#include "profiling.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <thread>
#include <vector>

extern "C" float spmv_float(const int len, const float *values,
                            const int *col_index, const float *vec);

namespace spmv {
class Triplet {
public:
  int row;
  int col;
  float value;
  Triplet(int row, int col, float value) : row(row), col(col), value(value) {}
};

class MatrixCSR {
private:
  std::vector<int> row_ptr;
  std::vector<int> col_index;
  std::vector<float> values;
  int rows;
  int cols;
  int nonzeros;

public:
  MatrixCSR() = default;
  MatrixCSR(int rows, int cols, int nonzeros)
      : rows(rows), cols(cols), nonzeros(nonzeros) {
    row_ptr.resize(rows + 1);
    row_ptr[0] = 0;
    row_ptr[rows] = nonzeros;
    col_index.reserve(nonzeros);
    values.reserve(nonzeros);
  }

  void addElement(int row, int col, float value) {
    values.push_back(value);
    col_index.push_back(col);
    for (auto i = row + 1; i < rows; i++) {
      row_ptr[i]++;
    }
  }

  template <typename TripletIter>
  void setFromTriplets(TripletIter begin, TripletIter end) {
    col_index.reserve(nonzeros);
    values.reserve(nonzeros);

    // count non zero in every row
    std::vector<int> num_nonzeros_per_row(rows, 0);
    for (auto it = begin; it != end; ++it) {
      num_nonzeros_per_row[it->row]++;
    }

    row_ptr[0] = 0;
    for (int i = 0; i < rows; ++i) {
      row_ptr[i + 1] = row_ptr[i] + num_nonzeros_per_row[i];
    }

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
    for (float value : matrix.values) {
      os << value << " ";
    }
    os << std::endl;

    os << "rows: " << matrix.rows << std::endl;
    os << "cols: " << matrix.cols << std::endl;
    os << "nonzeros: " << matrix.nonzeros << std::endl;

    return os;
  }

  int getRows() const { return rows; }
  int getCols() const { return cols; }
  int getNonzeros() const { return nonzeros; }
  const std::vector<int> &getRowPtr() const { return row_ptr; }
  const std::vector<int> &getColIndex() const { return col_index; }
  const std::vector<float> &getValues() const { return values; }
};

std::vector<float> readVectorFromFile(const std::string &filepath) {
  std::ifstream file(filepath);
  if (!file) {
    std::cerr << "Failed to open file: " << filepath << std::endl;
    return std::vector<float>();
  }
  int size;
  file >> size;
  std::vector<float> vector(size);
  for (int i = 0; i < size; i++) {
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
  int rows, cols, nonzeros;
  file >> rows >> cols >> nonzeros;
  std::vector<Triplet> tempElements; // 临时存储非零元素的向量
  tempElements.reserve(nonzeros);
  for (int i = 0; i < nonzeros; i++) {
    int row, col;
    float value;
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

void printData(const std::vector<float> &result,
               const std::string &outputFile) {
  std::ofstream resultFile(outputFile);
  if (!resultFile) {
    std::cerr << "Failed to open file: " << outputFile << std::endl;
    abort();
  }
  int count = result.size();
  for (int i = 0; i < count; i++) {
    if (i == 0) {
      resultFile << result[i];
    } else {
      resultFile << std::endl << result[i];
    }
  }

  resultFile.close();
}

// naive version
std::vector<float> multiply(const std::vector<float> &vector,
                            const MatrixCSR &matrix) {
  int cols = matrix.getCols();
  int rows = matrix.getRows();
  const std::vector<int> &row_ptr = matrix.getRowPtr();
  const std::vector<int> &col_index = matrix.getColIndex();
  const std::vector<float> &values = matrix.getValues();
  if (cols != vector.size()) {
    std::cerr << "Matrix and vector dimensions do not match." << std::endl;
    abort();
  }
  std::vector<float> result(rows);

  for (auto i = 0; i < rows; i++) {
    for (auto j = row_ptr[i]; j < row_ptr[i + 1]; j++) {
      result[i] += values[j] * vector[col_index[j]];
    }
  }

  return result;
}

// only simd
std::vector<float> multiply2(const std::vector<float> &vector,
                             const MatrixCSR &matrix) {
  int rows = matrix.getRows();
  int cols = matrix.getCols();
  const std::vector<int> &row_ptr = matrix.getRowPtr();
  const std::vector<int> &col_index = matrix.getColIndex();
  const std::vector<float> &values = matrix.getValues();
  if (cols != vector.size()) {
    std::cerr << "Matrix and vector dimensions do not match." << std::endl;
    abort();
  }
  std::vector<float> result(rows, 0.0);
  for (auto i = 0; i < rows; i++) {
    auto start = row_ptr[i];
    auto end = row_ptr[i + 1];
    auto len = end - start;
    result[i] = spmv_float(len, values.data() + start, &col_index[start],
                           vector.data());
  }
  return result;
}

// multi-thread OpenMP version
std::vector<float> multiply3(const std::vector<float> &vector,
                             const MatrixCSR &matrix) {
  int cols = matrix.getCols();
  int rows = matrix.getRows();
  const std::vector<int> &row_ptr = matrix.getRowPtr();
  const std::vector<int> &col_index = matrix.getColIndex();
  const std::vector<float> &values = matrix.getValues();
  if (cols != vector.size()) {
    std::cerr << "Matrix and vector dimensions do not match." << std::endl;
    abort();
  }
  std::vector<float> result(rows);

#pragma omp parallel for
  for (auto i = 0; i < rows; i++) {
    for (auto j = row_ptr[i]; j < row_ptr[i + 1]; j++) {
      result[i] += values[j] * vector[col_index[j]];
    }
  }
  return result;
}

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

void thread_block1(int thread_id, int start, int end, int start2, int end2,
                   const std::vector<int> &row_ptr,
                   const std::vector<int> &col_index,
                   const std::vector<float> &values,
                   std::vector<float> &mtx_ans, std::vector<float> &mid_ans,
                   const std::vector<float> &vector) {

  int start1, end1, Thread, i, j;
  float sum;
  switch (start < end) {
  case true: {
    mtx_ans[start] = 0.0;
    mtx_ans[end] = 0.0;
    start1 = row_ptr[start] + start2;
    start++;
    end1 = row_ptr[start];
    Thread = thread_id << 1;
    sum = 0.0;
    for (j = start1; j < end1; j++) {
      sum += values[j] * vector[col_index[j]];
    }
    mid_ans[Thread] = sum;
    start1 = end1;

    for (i = start; i < end; ++i) {
      end1 = row_ptr[i + 1];
      sum = 0.0;
      for (j = start1; j < end1; j++) {
        sum += values[j] * vector[col_index[j]];
      }
      mtx_ans[i] = sum;
      start1 = end1;
    }

    start1 = row_ptr[end];
    end1 = start1 + end2;
    sum = 0.0;
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
    for (j = start1; j < end1; j++) {
      sum += values[j] * vector[col_index[j]];
    }
    mid_ans[Thread] = sum;
    mid_ans[Thread | 1] = 0.0;
    return;
  }
  }
}

float calculation(const int start, const int end,
                  const std::vector<float> &values,
                  const std::vector<int> &col_index,
                  const std::vector<float> &vector) {
  int len = end - start;
  float res = 0;
  if ((len) >= 8) {
    res = spmv_float(len, values.data() + start, &col_index[start],
                     vector.data());
  } else {
    for (auto j = start; j < end; j++) {
      res += values[j] * vector[col_index[j]];
    }
  }
  return res;
}

void thread_block2(int thread_id, int start, int end, int start2, int end2,
                   const std::vector<int> &row_ptr,
                   const std::vector<int> &col_index,
                   const std::vector<float> &values,
                   std::vector<float> &mtx_ans, std::vector<float> &mid_ans,
                   const std::vector<float> &vector) {

  int start1, end1, Thread, i, j;
  float sum;
  switch (start < end) {
  case true: {
    mtx_ans[start] = 0.0;
    mtx_ans[end] = 0.0;
    start1 = row_ptr[start] + start2;
    start++;
    end1 = row_ptr[start];
    Thread = thread_id << 1;
    mid_ans[Thread] = calculation(start1, end1, values, col_index, vector);
    start1 = end1;

    for (i = start; i < end; ++i) {
      end1 = row_ptr[i + 1];
      sum = calculation(start1, end1, values, col_index, vector);
      mtx_ans[i] = sum;
      start1 = end1;
    }
    start1 = row_ptr[end];
    end1 = start1 + end2;
    mid_ans[Thread | 1] = calculation(start1, end1, values, col_index, vector);
    return;
  }
  default: {
    mtx_ans[start] = 0.0;
    sum = 0;
    Thread = thread_id << 1;
    start1 = row_ptr[start] + start2;
    end1 = row_ptr[start] + end2;
    mid_ans[Thread] = calculation(start1, end1, values, col_index, vector);
    mid_ans[Thread | 1] = 0.0;
    return;
  }
  }
}

// ALBUS +openmp
std::vector<float> multiply4(const std::vector<float> &vector,
                             const MatrixCSR &matrix) {
  int cols = matrix.getCols();
  int rows = matrix.getRows();
  int nonzeronums = matrix.getNonzeros();

  int thread_nums = std::thread::hardware_concurrency();

  const std::vector<int> &row_ptr = matrix.getRowPtr();
  const std::vector<int> &col_index = matrix.getColIndex();
  const std::vector<float> &values = matrix.getValues();
  if (cols != vector.size()) {
    std::cerr << "Matrix and vector dimensions do not match." << std::endl;
    abort();
  }
  // mid_ans final_ans
  std::vector<float> result(rows + 1, 0.0);
  std::vector<float> result_mid(thread_nums * 2, 0.0);
  // store left boundary row index of every thread
  std::vector<int> start(thread_nums, 0);
  // store right boundary row index of every thread
  std::vector<int> end(thread_nums, 0);
  // count of values in right boundary
  std::vector<int> start1(thread_nums, 0);
  std::vector<int> end1(thread_nums, 0);

  albus_balance(matrix, start, end, start1, end1, thread_nums);

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
  }

  return result;
}

// albus+simd
std::vector<float> multiply5(const std::vector<float> &vector,
                             const MatrixCSR &matrix) {
  int cols = matrix.getCols();
  int rows = matrix.getRows();
  int nonzeronums = matrix.getNonzeros();

  int thread_nums = std::thread::hardware_concurrency();

  const std::vector<int> &row_ptr = matrix.getRowPtr();
  const std::vector<int> &col_index = matrix.getColIndex();
  const std::vector<float> &values = matrix.getValues();
  if (cols != vector.size()) {
    std::cerr << "Matrix and vector dimensions do not match." << std::endl;
    abort();
  }
  // mid_ans final_ans
  std::vector<float> result(rows + 1, 0.0);
  std::vector<float> result_mid(thread_nums * 2, 0.0);
  // store left boundary row index of every thread
  std::vector<int> start(thread_nums, 0);
  // store right boundary row index of every thread
  std::vector<int> end(thread_nums, 0);
  // count of values in right boundary
  std::vector<int> start1(thread_nums, 0);
  // store the count of values in row end[thread_id]
  std::vector<int> end1(thread_nums, 0);

  albus_balance(matrix, start, end, start1, end1, thread_nums);

#pragma omp parallel
  {
#pragma omp for schedule(static) nowait
    for (int i = 0; i < thread_nums; ++i) {
      thread_block2(i, start[i], end[i], start1[i], end1[i], row_ptr, col_index,
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
  }

  return result;
}

} // namespace spmv
