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

#include <spdlog/spdlog.h>

namespace kaldi {

struct Nnet3Config {
  BaseFloat chunk_length_secs = 0.18;
  BaseFloat output_period = 1;
  BaseFloat samp_freq = 16000.0;
  bool produce_time = false;
  std::string nnet3_model_filename;
  std::string fst_hclg_filename;
  std::string word_syms_tbl_filename;
};

// TODO Needs a better name
struct Nnet3Data {
  OnlineNnet2FeaturePipelineInfo feature_info;
  BaseFloat frame_shift;
  int32 frame_subsampling;

  TransitionModel trans_model;
  nnet3::AmNnetSimple am_nnet;
  nnet3::DecodableNnetSimpleLoopedInfo decodable_info;

  fst::Fst<fst::StdArc>* decode_fst;
  fst::SymbolTable* word_syms;
  Nnet3Data(OnlineNnet2FeaturePipelineConfig feature_opts,
            nnet3::NnetSimpleLoopedComputationOptions decodable_opts,
            const std::string& nnet3_model_filename)
      : feature_info(feature_info_in),
        frame_shift(feature_info.FrameShiftInSeconds()),
                    frame_subsampling(decodable_opts.frame_subsampling_factor)) {

    SPDLOG_INFO("Loading AM...");
    {
      bool binary;
      Input ki(nnet3_rxfilename, &binary);
      trans_model.Read(ki.Stream(), binary);
      am_nnet.Read(ki.Stream(), binary);
      SetBatchnormTestMode(true, &(am_nnet.GetNnet()));
      SetDropoutTestMode(true, &(am_nnet.GetNnet()));
      nnet3::CollapseModel(nnet3::CollapseModelConfig(), &(am_nnet.GetNnet()));
    }

    decodable_info = nnet3::DecodableNnetSimpleLoopedInfo(decodable_opts, &am_nnet);

    SPDLOG_INFO("Loading FST...");

    fst::Fst<fst::StdArc>* decode_fst = ReadFstKaldiGeneric(fst_rxfilename);

    fst::SymbolTable* word_syms = NULL;
    if (!word_syms_filename.empty()) {
      if (!(word_syms = fst::SymbolTable::ReadText(word_syms_filename))) {
        SPDLOG_ERROR("Could not read symbol table from file {}", word_syms_filename);
      }
    }
  }
};

int runDecodeServer(int argc, char* argv[]);
std::string LatticeToString(const Lattice& lat, const fst::SymbolTable& word_syms);
std::string GetTimeString(int32 t_beg, int32 t_end, BaseFloat time_unit);
int32 GetLatticeTimeSpan(const Lattice& lat);
std::string LatticeToString(const CompactLattice& clat, const fst::SymbolTable& word_syms);
} // namespace kaldi