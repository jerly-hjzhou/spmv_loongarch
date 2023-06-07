#include "spmv_double.hpp"
#include <algorithm>
#include <iomanip>

DEF_ENV_PARAM(ENABLE_DEBUG)

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
  spmv::MatrixCSR matrix = spmv::readMatrixFromFile(matrixFile);
  if (ENV_PARAM(ENABLE_DEBUG)) {
    std::cout << matrix;
  }
  __TOC__(READ_MATRIX)

  __TIC__(READ_VECTOR)
  std::vector<double> vector = spmv::readVectorFromFile(vectorFile);
  __TOC__(READ_VECTOR)

  __TIC__(OP)
  std::vector<double> result = spmv::multiply3(vector, matrix);
  __TOC__(OP)
  __TOC__(TOTAL)

  spmv::printData(result, outputFile);

  return 0;
}
