#pragma once

#include "KaldiInterface.hpp"

namespace TestUtils {

template <typename T>
kaldi::Vector<T> initializerListToKaldiVector(std::initializer_list<T> list) {
  kaldi::Vector<T> k_vector(static_cast<kaldi::MatrixIndexT>(list.size()));
  kaldi::MatrixIndexT vector_idx = 0;
  for (const auto item : list) {
    k_vector(vector_idx++) = item;
  }

  return k_vector;
}

} // namespace TestUtils
