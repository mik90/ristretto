#pragma once

#include "KaldiInterface.hpp"

namespace TestUtils {

template <typename T>
kaldi::Vector<T> initializerListToKaldiVector(std::initializer_list<T> list) {
  kaldi::Vector<T> kaldiVector(static_cast<kaldi::MatrixIndexT>(list.size()));
  kaldi::MatrixIndexT vectorIdx = 0;
  for (const auto item : list) {
    kaldiVector(vectorIdx++) = item;
  }

  return kaldiVector;
}

} // namespace TestUtils
