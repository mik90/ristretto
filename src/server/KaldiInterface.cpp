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

#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <spdlog/spdlog.h>

#include "KaldiInterface.hpp"

namespace kaldi {

Vector<BaseFloat> convertBytesToFloatVec(std::unique_ptr<std::string> audioData) {

  const auto length = static_cast<MatrixIndexT>(audioData->length());
  Vector<BaseFloat> floatAudioData;
  // Ensure that there's enough space in the output Vector
  floatAudioData.Resize(length, kUndefined);
  SPDLOG_DEBUG("audioData->length():{}", length);
  SPDLOG_DEBUG("After Resize, floatAudioData.SizeInBytes():{}", floatAudioData.SizeInBytes());

  for (auto i = 0; i < length; ++i) {
    // auto c = (*audioData)[static_cast<size_t>(i)];
    auto c = audioData->at(static_cast<size_t>(i));
    floatAudioData(i) = static_cast<BaseFloat>(c);
  }
  SPDLOG_DEBUG("After conversion, floatAudioData.SizeInBytes():{}", floatAudioData.SizeInBytes());
  return floatAudioData;
}

std::string Nnet3Data::decodeAudioChunk(std::unique_ptr<std::string> audioData) {

  Vector<BaseFloat> wave_part = convertBytesToFloatVec(std::move(audioData));

  feature_pipeline_ptr->AcceptWaveform(samp_freq, wave_part);
  samp_count += static_cast<int32>(chunk_len);

  if (silence_weighting_ptr->Active() && feature_pipeline_ptr->IvectorFeature() != nullptr) {
    silence_weighting_ptr->ComputeCurrentTraceback(decoder_ptr->Decoder());
    silence_weighting_ptr->GetDeltaWeights(feature_pipeline_ptr->NumFramesReady(),
                                           frame_offset * decodable_opts.frame_subsampling_factor,
                                           &delta_weights);
    feature_pipeline_ptr->UpdateFrameWeights(delta_weights);
  }

  decoder_ptr->AdvanceDecoding();
  std::string output;

  if (samp_count > check_count) {
    if (decoder_ptr->NumFramesDecoded() > 0) {
      Lattice lat;
      decoder_ptr->GetBestPath(false, &lat);
      TopSort(&lat); // for LatticeStateTimes(),
      std::string msg = LatticeToString(lat, *word_syms);

      // get time-span after previous endpoint,
      if (produce_time) {
        int32 t_beg = frame_offset;
        int32 t_end = frame_offset + GetLatticeTimeSpan(lat);
        msg = GetTimeString(t_beg, t_end, frame_shift * static_cast<BaseFloat>(frame_subsampling)) +
              " " + msg;
      }

      SPDLOG_INFO("Temporary transcript: {}", msg);
      output += msg;
    }
    check_count += check_period;
  }

  if (decoder_ptr->EndpointDetected(endpoint_opts)) {
    decoder_ptr->FinalizeDecoding();
    frame_offset += decoder_ptr->NumFramesDecoded();
    CompactLattice lat;
    decoder_ptr->GetLattice(true, &lat);
    std::string msg = LatticeToString(lat, *word_syms);

    // get time-span between endpoints,
    if (produce_time) {
      int32 t_beg = frame_offset - decoder_ptr->NumFramesDecoded();
      int32 t_end = frame_offset;
      msg = GetTimeString(t_beg, t_end, frame_shift * static_cast<BaseFloat>(frame_subsampling)) +
            " " + msg;
    }

    SPDLOG_INFO("Endpoint, sending message: {}", msg);
    output += msg;
  }

  // Finish up  decoding
  feature_pipeline_ptr->InputFinished();
  decoder_ptr->AdvanceDecoding();
  decoder_ptr->FinalizeDecoding();
  frame_offset += decoder_ptr->NumFramesDecoded();
  if (decoder_ptr->NumFramesDecoded() > 0) {
    CompactLattice lat;
    decoder_ptr->GetLattice(true, &lat);
    std::string msg = LatticeToString(lat, *word_syms);

    // get time-span from previous endpoint to end of audio,
    if (produce_time) {
      int32 t_beg = frame_offset - decoder_ptr->NumFramesDecoded();
      int32 t_end = frame_offset;
      msg = GetTimeString(t_beg, t_end, frame_shift * static_cast<BaseFloat>(frame_subsampling)) +
            " " + msg;
    }

    SPDLOG_INFO("EndOfAudio, sending message: {}", msg);
    output += msg;
  }
  return output;
}

Nnet3Data::Nnet3Data(int argc, char* argv[])
    : feature_opts(), decodable_opts(), decoder_opts(), endpoint_opts() {

  const char* usage = "Reads in audio from a network socket and performs online\n"
                      "decoding with neural nets (nnet3 setup), with iVector-based\n"
                      "speaker adaptation and endpointing.\n"
                      "Note: some configuration values and inputs are set via config\n"
                      "files whose filenames are passed as options\n"
                      "\n"
                      "Usage: online2-tcp-nnet3-decode-faster [options] <nnet3-in> "
                      "<fst-in> <word-symbol-table>\n";

  ParseOptions po(usage);

  chunk_length_secs = 0.18f;
  output_period = 1;
  samp_freq = 16000.0;
  port_num = 5050;
  read_timeout = 3;
  produce_time = false;

  po.Register("samp-freq", &samp_freq,
              "Sampling frequency of the input signal (coded as 16-bit slinear).");
  po.Register("chunk-length", &chunk_length_secs,
              "Length of chunk size in seconds, that we process.");
  po.Register("output-period", &output_period,
              "How often in seconds, do we check for changes in output.");
  po.Register("num-threads-startup", &g_num_threads,
              "Number of threads used when initializing iVector extractor.");
  po.Register("read-timeout", &read_timeout,
              "Number of seconds of timout for TCP audio data to appear on the stream. Use -1 "
              "for blocking.");
  po.Register("port-num", &port_num, "Port number the server will listen on.");
  po.Register(
      "produce-time", &produce_time,
      "Prepend begin/end times between endpoints (e.g. '5.46 6.81 <text_output>', in seconds)");

  feature_opts.Register(&po);
  decodable_opts.Register(&po);
  decoder_opts.Register(&po);
  endpoint_opts.Register(&po);

  po.Read(argc, argv);

  if (po.NumArgs() != 3) {
    po.PrintUsage();
  }

  const std::string nnet3_rxfilename = po.GetArg(1);
  const std::string fst_rxfilename = po.GetArg(2);
  const std::string word_syms_filename = po.GetArg(3);

  OnlineNnet2FeaturePipelineInfo feature_info(feature_opts);

  frame_shift = feature_info.FrameShiftInSeconds();
  frame_subsampling = decodable_opts.frame_subsampling_factor;

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

  // this object contains precomputed stuff that is used by all decodable
  // objects.  It takes a pointer to am_nnet because if it has iVectors it has
  // to modify the nnet to accept iVectors at intervals.
  nnet3::DecodableNnetSimpleLoopedInfo decodable_info(decodable_opts, &am_nnet);

  SPDLOG_INFO("Loading FST...");

  decode_fst = fst::ReadFstKaldiGeneric(fst_rxfilename);

  if (!word_syms_filename.empty()) {
    if (!(word_syms = fst::SymbolTable::ReadText(word_syms_filename))) {
      SPDLOG_ERROR("Could not read symbol table from file {}", word_syms_filename);
    }
  }

  samp_count = 0; // this is used for output refresh rate
  chunk_len = static_cast<size_t>(chunk_length_secs * samp_freq);
  check_period = static_cast<int32>(samp_freq * output_period);
  check_count = check_period;
  frame_offset = 0;

  feature_pipeline_ptr = std::make_unique<OnlineNnet2FeaturePipeline>(feature_info);
  decoder_ptr = std::make_unique<SingleUtteranceNnet3Decoder>(
      decoder_opts, trans_model, decodable_info, *decode_fst, feature_pipeline_ptr.get());
  /*
  OnlineNnet2FeaturePipeline feature_pipeline(feature_info);
  SingleUtteranceNnet3Decoder decoder(decoder_opts, trans_model, decodable_info, *decode_fst,
                                      &feature_pipeline);
  */

  decoder_ptr->InitDecoding(frame_offset);
  silence_weighting_ptr = std::make_unique<OnlineSilenceWeighting>(
      trans_model, feature_info.silence_weighting_config, decodable_opts.frame_subsampling_factor);
}

std::string LatticeToString(const Lattice& lat, const fst::SymbolTable& word_syms) {
  LatticeWeight weight;
  std::vector<int32> alignment;
  std::vector<int32> words;
  GetLinearSymbolSequence(lat, &alignment, &words, &weight);

  std::ostringstream msg;
  for (size_t i = 0; i < words.size(); i++) {
    std::string s = word_syms.Find(words[i]);
    if (s.empty()) {
      KALDI_WARN << "Word-id " << words[i] << " not in symbol table.";
      msg << "<#" << std::to_string(i) << "> ";
    } else
      msg << s << " ";
  }
  return msg.str();
}

std::string GetTimeString(int32 t_beg, int32 t_end, BaseFloat time_unit) {
  char buffer[100];
  double t_beg2 = static_cast<BaseFloat>(t_beg) * time_unit;
  double t_end2 = static_cast<BaseFloat>(t_end) * time_unit;
  snprintf(buffer, 100, "%.2f %.2f", t_beg2, t_end2);
  return std::string(buffer);
}

int32 GetLatticeTimeSpan(const Lattice& lat) {
  std::vector<int32> times;
  LatticeStateTimes(lat, &times);
  return times.back();
}

std::string LatticeToString(const CompactLattice& clat, const fst::SymbolTable& word_syms) {
  if (clat.NumStates() == 0) {
    KALDI_WARN << "Empty lattice.";
    return "";
  }
  CompactLattice best_path_clat;
  CompactLatticeShortestPath(clat, &best_path_clat);

  Lattice best_path_lat;
  ConvertLattice(best_path_clat, &best_path_lat);
  return LatticeToString(best_path_lat, word_syms);
}

} // namespace kaldi