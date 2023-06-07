#include "profiling.hpp"
#include <eigen3/Eigen/Sparse>
#include <fstream>
#include <iostream>

typedef Eigen::VectorXd Vector;

Eigen::SparseMatrix<double> readSparseMatrix(const std::string &filename) {
  std::ifstream file(filename);
  int rows, cols, nonZeros;
  file >> rows >> cols >> nonZeros;

  Eigen::SparseMatrix<double> sparseMatrix(rows, cols);
  sparseMatrix.reserve(nonZeros);

  int row, col;
  double value;
  for (int i = 0; i < nonZeros; ++i) {
    file >> row >> col >> value;
    sparseMatrix.insert(row, col) = value;
  }
  sparseMatrix.makeCompressed(); // 将稀疏矩阵压缩

  file.close();

  return sparseMatrix;
}

Vector readVector(const std::string &filename) {
  std::ifstream file(filename);
  int num;
  file >> num;

  Vector vector(num);

  for (int i = 0; i < num; ++i) {
    file >> vector(i);
  }
  file.close();

  return vector;
}

int main(int argc, char *argv[]) {
  if (argc != 4) {
    std::cout << "Usage: ./program_name matrix_file vector_file output_file"
              << std::endl;
    return 1;
  }

  std::string matrixFile = argv[1];
  std::string vectorFile = argv[2];
  std::string outputFile = argv[3];
  __TIC__(TOTAL)
  __TIC__(READ_MATRIX)
  // 从matrix.txt读取稀疏矩阵
  Eigen::SparseMatrix<double> sparseMatrix = readSparseMatrix(matrixFile);
  __TOC__(READ_MATRIX)

  __TIC__(READ_VECTOR)
  // 从vec.txt读取向量
  Vector vector = readVector(vectorFile);
  __TOC__(READ_VECTOR)

  __TIC__(OP)
  // 计算稀疏矩阵向量乘
  Vector result = sparseMatrix * vector;
  __TOC__(OP)
  __TOC__(TOTAL)

  std::cout << "Sparse matrix dimensions: " << sparseMatrix.rows() << "x"
            << sparseMatrix.cols() << std::endl;

  // 将结果保存到txt文件
  std::ofstream resultFile(outputFile);
  resultFile << result;
  resultFile.close();

  return 0;
}
