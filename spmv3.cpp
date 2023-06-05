#include "spmv.hpp"
#include <algorithm>
#include <iomanip>


DEF_ENV_PARAM(ENABLE_DEBUG)

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

spmv::MatrixCSR readMatrixFromFile(const std::string &filepath) {
  std::ifstream file(filepath);
  if (!file) {
    std::cerr << "Failed to open file: " << filepath << std::endl;
    abort();
  }
  int rows, cols, nonzeros;
  file >> rows >> cols >> nonzeros;
  std::vector<spmv::Triplet> tempElements; // 临时存储非零元素的向量
  tempElements.reserve(nonzeros);
  for (int i = 0; i < nonzeros; i++) {
    int row, col;
    float value;
    file >> row >> col >> value;
    tempElements.emplace_back(spmv::Triplet(row, col, value));
  }
  std::sort(tempElements.begin(), tempElements.end(),
            [](const spmv::Triplet &a, const spmv::Triplet &b) {
              if (a.row != b.row)
                return a.row < b.row;
              return a.col < b.col;
            });

  spmv::MatrixCSR matrix(rows, cols, nonzeros);
  matrix.setFromTriplets(tempElements.begin(), tempElements.end());

  return matrix;
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
  spmv::MatrixCSR matrix = readMatrixFromFile(matrixFile);
  if (ENV_PARAM(ENABLE_DEBUG)) {
    std::cout << matrix;  
  }
  __TOC__(READ_MATRIX)

  __TIC__(READ_VECTOR)
  std::vector<float> vector = readVectorFromFile(vectorFile);
  __TOC__(READ_VECTOR)

  __TIC__(OP)
  std::vector<float> result = multiply3(vector, matrix);
  __TOC__(OP)
  __TOC__(TOTAL)

  spmv::printData(result, outputFile);

  return 0;
}
