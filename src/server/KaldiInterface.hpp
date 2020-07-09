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

#include "fstext/fstext-lib.h"
#include "lat/lattice-functions.h"

#include <mutex>
#include <spdlog/spdlog.h>

namespace kaldi {

// Catch-all class for using online decoding with nnet3
class Nnet3Data {
public:
  Nnet3Data(int argc, char* argv[]);

  // Allows async functions to use the Nnet3 decoder
  auto getDecoderLambda() {
    SPDLOG_DEBUG("Creating a decoder lambda");
    return [this](std::unique_ptr<std::string> audioData) {
      return this->decodeAudio(std::move(audioData));
    };
  }

protected:
  std::string decodeAudio(std::unique_ptr<std::string> audioData);

private:
  OnlineNnet2FeaturePipelineConfig featureOpts;
  nnet3::NnetSimpleLoopedComputationOptions decodableOpts;
  LatticeFasterDecoderConfig decoderOpts;
  OnlineEndpointConfig endpointOpts;
  BaseFloat frameShift;
  int32 frameSubsampling;
  TransitionModel transModel;
  nnet3::AmNnetSimple amNnet;
  fst::Fst<fst::StdArc>* decodeFst;
  fst::SymbolTable* wordSyms;
  BaseFloat chunkLengthSecs;
  BaseFloat outputPeriod;
  BaseFloat sampFreq;
  int portNum;
  int readTimeout;
  bool produceTime;
  int32 sampCount;
  size_t chunkLen;
  int32 checkPeriod;
  int32 checkCount;
  int32 frameOffset;
  std::unique_ptr<OnlineNnet2FeaturePipeline> featurePipelinePtr;
  std::unique_ptr<SingleUtteranceNnet3Decoder> decoderPtr;
  std::unique_ptr<OnlineSilenceWeighting> silenceWeightingPtr;
  std::unique_ptr<OnlineNnet2FeaturePipelineInfo> featureInfoPtr;
  std::unique_ptr<nnet3::DecodableNnetSimpleLoopedInfo> decodableInfoPtr;
  std::vector<std::pair<int32, BaseFloat>> deltaWeights;

  std::mutex decoderMutex;
};

Vector<BaseFloat> deserializeAudioData(std::unique_ptr<std::string> audioDataPtr);

int runDecodeServer(int argc, char* argv[]);

std::string LatticeToString(const Lattice& lat, const fst::SymbolTable& wordSyms);
std::string GetTimeString(int32 tBeg, int32 tEnd, BaseFloat timeUnit);
int32 GetLatticeTimeSpan(const Lattice& lat);
std::string LatticeToString(const CompactLattice& clat, const fst::SymbolTable& wordSyms);

} // namespace kaldi