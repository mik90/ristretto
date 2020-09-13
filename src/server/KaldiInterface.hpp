// Based on: online2bin/online2-tcp-nnet3-decode-faster.cc

// Copyright 2014  Johns Hopkins University (author: Daniel Povey)
//           2016  Api.ai (Author: Ilya Platonov)
//           2018  Polish-Japanese Academy of Information Technology (Author: Danijel Korzinek)

// See ../../COPYING for clarification regarding multiple authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//  http://www.apache.org/licenses/LICENSE-2.0
//
// THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION ANY IMPLIED
// WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR PURPOSE,
// MERCHANTABLITY OR NON-INFRINGEMENT.
// See the Apache 2 License for the specific language governing permissions and
// limitations under the License.

// Modified for use in Ristretto
#pragma once

#include <mutex>

#include "feat/wave-reader.h"
#include "fstext/fstext-lib.h"
#include "lat/lattice-functions.h"
#include "nnet3/nnet-utils.h"
#include "online2/online-endpoint.h"
#include "online2/online-nnet2-feature-pipeline.h"
#include "online2/online-nnet3-decoding.h"
#include "online2/online-timing.h"
#include "online2/onlinebin-util.h"
#include "util/kaldi-thread.h"

#include <spdlog/spdlog.h>

namespace mik {

/**
 * Nnet3Data
 * @brief Catch-all class for using online decoding with nnet3. Holds a lot of Kaldi objects
 */
class Nnet3Data {

public:
  // NOLINTNEXTLINE: Easiest to use cmd line args with kaldi
  Nnet3Data(int argc, const char** argv);

  std::string decodeAudio(const std::string& sessionToken, uint32_t audioId,
                          std::unique_ptr<std::string> audioDataPtr);

private:
  std::mutex decoderMutex_;

  // Kaldi data
  kaldi::OnlineNnet2FeaturePipelineConfig featureOpts_;
  kaldi::nnet3::NnetSimpleLoopedComputationOptions decodableOpts_;
  kaldi::LatticeFasterDecoderConfig decoderOpts_;
  kaldi::OnlineEndpointConfig endpointOpts_;
  kaldi::BaseFloat frameShift_;
  kaldi::int32 frameSubsampling_;
  kaldi::TransitionModel transModel_;
  kaldi::nnet3::AmNnetSimple amNnet_;
  fst::Fst<fst::StdArc>* decodeFst_;
  fst::SymbolTable* wordSyms_;
  kaldi::BaseFloat chunkLengthSecs_;
  kaldi::BaseFloat outputPeriod;
  kaldi::BaseFloat sampFreq_;
  int readTimeout_;
  bool produceTime_;
  kaldi::int32 sampCount;
  size_t chunkLen_;
  kaldi::int32 checkPeriod_;
  kaldi::int32 checkCount_;
  kaldi::int32 frameOffset_;
  std::unique_ptr<kaldi::OnlineNnet2FeaturePipeline> featurePipelinePtr_;
  std::unique_ptr<kaldi::SingleUtteranceNnet3Decoder> decoderPtr_;
  std::unique_ptr<kaldi::OnlineSilenceWeighting> silenceWeightingPtr_;
  std::unique_ptr<kaldi::OnlineNnet2FeaturePipelineInfo> featureInfoPtr_;
  std::unique_ptr<kaldi::nnet3::DecodableNnetSimpleLoopedInfo> decodableInfoPtr_;
  std::vector<std::pair<kaldi::int32, kaldi::BaseFloat>> deltaWeights_;
};

kaldi::Vector<kaldi::BaseFloat> stringToKaldiVector(std::unique_ptr<std::string> audioDataPtr);

std::string LatticeToString(const kaldi::Lattice& lat, const fst::SymbolTable& wordSyms);
std::string GetTimeString(kaldi::int32 tBeg, kaldi::int32 tEnd, kaldi::BaseFloat timeUnit);
kaldi::int32 GetLatticeTimeSpan(const kaldi::Lattice& lat);
std::string LatticeToString(const kaldi::CompactLattice& clat, const fst::SymbolTable& wordSyms);

} // namespace mik
