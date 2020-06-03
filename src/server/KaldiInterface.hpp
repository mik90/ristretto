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

namespace kaldi {

class Nnet3Data {
public:
  OnlineNnet2FeaturePipelineConfig feature_opts;
  nnet3::NnetSimpleLoopedComputationOptions decodable_opts;
  LatticeFasterDecoderConfig decoder_opts;
  OnlineEndpointConfig endpoint_opts;
  BaseFloat frame_shift;
  int32 frame_subsampling;
  TransitionModel trans_model;
  nnet3::AmNnetSimple am_nnet;
  fst::Fst<fst::StdArc>* decode_fst;
  fst::SymbolTable* word_syms;
  BaseFloat chunk_length_secs;
  BaseFloat output_period;
  BaseFloat samp_freq;
  int port_num;
  int read_timeout;
  bool produce_time;
  int32 samp_count;
  size_t chunk_len;
  int32 check_period;
  int32 check_count;
  int32 frame_offset;
  std::unique_ptr<OnlineNnet2FeaturePipeline> feature_pipeline_ptr;
  std::unique_ptr<SingleUtteranceNnet3Decoder> decoder_ptr;
  std::unique_ptr<OnlineSilenceWeighting> silence_weighting_ptr;
  std::vector<std::pair<int32, BaseFloat>> delta_weights;

  Nnet3Data(int argc, char* argv[]);

  std::string decodeAudioChunk(std::unique_ptr<std::string> audioData);
};

int runDecodeServer(int argc, char* argv[]);
std::string LatticeToString(const Lattice& lat, const fst::SymbolTable& word_syms);
std::string GetTimeString(int32 t_beg, int32 t_end, BaseFloat time_unit);
int32 GetLatticeTimeSpan(const Lattice& lat);
std::string LatticeToString(const CompactLattice& clat, const fst::SymbolTable& word_syms);
} // namespace kaldi