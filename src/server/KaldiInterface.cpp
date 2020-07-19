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

#include <cstring>
#include <spdlog/spdlog.h>
#include <string>

#include "KaldiInterface.hpp"

using namespace kaldi;
namespace mik {

/**
 * stringToKaldiVector
 */
Vector<BaseFloat> stringToKaldiVector(std::unique_ptr<std::string> audioDataPtr) {

  if (!audioDataPtr) {
    SPDLOG_ERROR("audioDataPtr was null!");
    return {};
  } else if (audioDataPtr->empty()) {
    SPDLOG_ERROR("audioData was empty!");
    return {};
  }
  if (audioDataPtr->length() % 2 != 0) {
    // Length is odd, just add an empty value to the end so the bitshifting logic won't be broken
    audioDataPtr->push_back(0x0);
  }

  const size_t inputLength = audioDataPtr->length();
  const size_t outputLength = inputLength / 2;

  std::vector<int16_t> buffer_int16(outputLength);
  if (!std::memmove(buffer_int16.data(), audioDataPtr->data(), audioDataPtr->length())) {
    SPDLOG_ERROR("Could not memmove audio data into int16 buffer!");
    return {};
  }

  // These vectors have the same number of elements
  Vector<BaseFloat> audio_data_float(static_cast<MatrixIndexT>(buffer_int16.size()), kUndefined);

  for (int i = 0; i < audio_data_float.Dim(); ++i) {
    // Convert each int16 into a BaseFloat
    audio_data_float(i) = static_cast<BaseFloat>(buffer_int16[static_cast<size_t>(i)]);
  }

  return audio_data_float;
}

/**
 * Nnet3Data::decodeAudio
 */
std::string Nnet3Data::decodeAudio(const std::string& sessionToken, uint32_t audioId,
                                   std::unique_ptr<std::string> audioDataPtr) {

  SPDLOG_INFO("decodeAudio sessionToken:{}, audioId:{}", sessionToken, audioId);
  const Vector<BaseFloat> complete_audio_data = stringToKaldiVector(std::move(audioDataPtr));
  SPDLOG_DEBUG("complete_audio_data size in bytes:{}, number of elements:{}",
               complete_audio_data.SizeInBytes(), complete_audio_data.Dim());

  SPDLOG_DEBUG("Getting lock on mutex...");
  // No idea how thread-safe Kaldi is so naively lock at the beginning of this method
  std::lock_guard<std::mutex> lock(decoderMutex_);
  SPDLOG_DEBUG("Got lock");

  decoderPtr_->InitDecoding(frameOffset_);
  SPDLOG_INFO("Initialized decoding");

  silenceWeightingPtr_ = std::make_unique<OnlineSilenceWeighting>(
      transModel_, featureInfoPtr_->silence_weighting_config,
      decodableOpts_.frame_subsampling_factor);
  SPDLOG_DEBUG("Constructed OnlineSilenceWeighting");

  // For debugging purposes
  if (!featurePipelinePtr_) {
    SPDLOG_ERROR("feature_pipeline_ptr was null!");
    return {};
  } else if (!silenceWeightingPtr_) {
    SPDLOG_ERROR("silenceWeightingPtr_ was null!");
    return {};
  } else if (!decoderPtr_) {
    SPDLOG_ERROR("decoderPtr_ was null!");
    return {};
  }
  try {
    std::string output;
    while (true) {

      // Get a usable chunk out of the audio data, this range will keep moving over the entire audio
      // data range
      auto samplesToRead = complete_audio_data.Dim() - sampCount;
      // Don't read more than a chunk
      samplesToRead = std::min(samplesToRead, static_cast<int32>(chunkLen_));
      // Don't try to read a negative number
      samplesToRead = std::max(0, samplesToRead);
      SPDLOG_DEBUG("samplesToRead {} vs chunkLen_ {}", samplesToRead, chunkLen_);

      if (samplesToRead == 0) {
        SPDLOG_INFO("End of stream, no more samples left. sampCount {}", sampCount);
        break;
      }

      // Equivalent to GetChunk
      SPDLOG_DEBUG("creating SubVector with Range({},{})", sampCount, samplesToRead);
      const auto sub_vec = complete_audio_data.Range(sampCount, samplesToRead);
      SPDLOG_DEBUG("created SubVector dim: {}, size in bytes:{}", sub_vec.Dim(),
                   sub_vec.SizeInBytes());
      SPDLOG_DEBUG("creating audio_chunk copy of SubVector");
      Vector<BaseFloat> audio_chunk(sub_vec);
      SPDLOG_DEBUG("created audio_chunk copy of SubVector dim: {}, size in bytes:{}",
                   audio_chunk.Dim(), audio_chunk.SizeInBytes());

      featurePipelinePtr_->AcceptWaveform(sampFreq_, audio_chunk);
      sampCount += static_cast<int32>(chunkLen_);
      SPDLOG_INFO("Chunk length:{}, Total sample count:{}", chunkLen_, sampCount);

      if (silenceWeightingPtr_->Active() && featurePipelinePtr_->IvectorFeature() != nullptr) {
        silenceWeightingPtr_->ComputeCurrentTraceback(decoderPtr_->Decoder());
        silenceWeightingPtr_->GetDeltaWeights(
            featurePipelinePtr_->NumFramesReady(),
            frameOffset_ * decodableOpts_.frame_subsampling_factor, &deltaWeights_);
        featurePipelinePtr_->UpdateFrameWeights(deltaWeights_);
        SPDLOG_DEBUG("Adjusted silence weighting");
      }

      SPDLOG_DEBUG("Advancing decoding...");
      decoderPtr_->AdvanceDecoding();
      SPDLOG_DEBUG("Decoding advanced");

      // SPDLOG_DEBUG("sampCount:{}, checkCount_:{}", sampCount, checkCount_);
      if (sampCount > checkCount_) {
        SPDLOG_DEBUG("sampCount:{} > checkCount_:{}", sampCount, checkCount_);
        const auto num_frames_decoded = decoderPtr_->NumFramesDecoded();
        if (num_frames_decoded > 0) {
          SPDLOG_DEBUG("decoded {} frames", num_frames_decoded);
          Lattice lat;
          decoderPtr_->GetBestPath(/* end of utt */ false, &lat);
          TopSort(&lat); // for LatticeStateTimes(),
          std::string msg = LatticeToString(lat, *wordSyms_);

          // get time-span after previous endpoint,
          if (produceTime_) {
            int32 t_beg = frameOffset_;
            int32 t_end = frameOffset_ + GetLatticeTimeSpan(lat);
            // NOLINTNEXTLINE: Don't want to mess with Kaldi code
            msg = GetTimeString(t_beg, t_end,
                                frameShift_ * static_cast<BaseFloat>(frameSubsampling_)) +
                  " " + msg;
          }

          SPDLOG_INFO("Temporary transcript: {}", msg);
        }
        checkCount_ += checkPeriod_;
      }

      if (decoderPtr_->EndpointDetected(endpointOpts_)) {
        SPDLOG_INFO("Endpoint detected");
        decoderPtr_->FinalizeDecoding();
        frameOffset_ += decoderPtr_->NumFramesDecoded();
        CompactLattice lat;
        decoderPtr_->GetLattice(true, &lat);
        std::string msg = LatticeToString(lat, *wordSyms_);

        // get time-span between endpoints,
        if (produceTime_) {
          int32 t_beg = frameOffset_ - decoderPtr_->NumFramesDecoded();
          int32 t_end = frameOffset_;
          // NOLINTNEXTLINE: Don't want to mess with Kaldi code
          msg =
              GetTimeString(t_beg, t_end, frameShift_ * static_cast<BaseFloat>(frameSubsampling_)) +
              " " + msg;
        }

        SPDLOG_INFO("Endpoint, sending message: {}", msg);
        output += msg;
        break;
      }
    } // end of while(true) loop

    SPDLOG_INFO("Input finished");
    featurePipelinePtr_->InputFinished();
    if (silenceWeightingPtr_->Active() && featurePipelinePtr_->IvectorFeature() != nullptr) {
      silenceWeightingPtr_->ComputeCurrentTraceback(decoderPtr_->Decoder());
      silenceWeightingPtr_->GetDeltaWeights(featurePipelinePtr_->NumFramesReady(),
                                            frameOffset_ * decodableOpts_.frame_subsampling_factor,
                                            &deltaWeights_);
      featurePipelinePtr_->UpdateFrameWeights(deltaWeights_);
      SPDLOG_DEBUG("Adjusted silence weighting");
    }

    decoderPtr_->AdvanceDecoding();
    decoderPtr_->FinalizeDecoding();
    const auto numFramesDecoded = decoderPtr_->NumFramesDecoded();
    frameOffset_ += numFramesDecoded;
    SPDLOG_DEBUG("frameOffset:{}, NumFramesDecoded:{}", frameOffset_, numFramesDecoded);
    if (numFramesDecoded > 0) {
      CompactLattice lat;
      decoderPtr_->GetLattice(true, &lat);
      std::string msg = LatticeToString(lat, *wordSyms_);

      // get time-span from previous endpoint to end of audio,
      if (produceTime_) {
        int32 t_beg = frameOffset_ - decoderPtr_->NumFramesDecoded();
        int32 t_end = frameOffset_;
        msg = GetTimeString(t_beg, t_end, frameShift_ * static_cast<BaseFloat>(frameSubsampling_)) +
              " " + msg;
      }

      SPDLOG_INFO("EndOfAudio, sending message: {}", msg);
      output += msg;
    }
    return output;

  } catch (const std::exception& e) {
    SPDLOG_ERROR("Caught std::exception:{}", e.what());
  } catch (...) {
    SPDLOG_ERROR("Caught unknown exception");
  }

  return {};
}

/**
 * Nnet3Data::Nnet3Data
 * @brief Reads in config file and sets up Nnet3 for a session of online decoding
 */
Nnet3Data::Nnet3Data(int argc, const char** argv) // NOLINT: Easiest to use cmd line args with kaldi
    : featureOpts_(), decodableOpts_(), decoderOpts_(), endpointOpts_() {

  SPDLOG_INFO("Constructing Nnet3Data");

  chunkLengthSecs_ = 0.18f;
  outputPeriod = 1;
  sampFreq_ = 16000.0;
  readTimeout_ = 3;
  produceTime_ = false;
  const char* usage = "Reads in audio from a network socket and performs online\n"
                      "decoding with neural nets (nnet3 setup), with iVector-based\n"
                      "speaker adaptation and endpointing.\n"
                      "Note: some configuration values and inputs are set via config\n"
                      "files whose filenames are passed as options\n"
                      "\n"
                      "Usage: online2-tcp-nnet3-decode-faster [options] <nnet3-in> "
                      "<fst-in> <word-symbol-table>\n";
  ParseOptions po(usage);

  po.Register("samp-freq", &sampFreq_,
              "Sampling frequency of the input signal (coded as 16-bit slinear).");
  po.Register("chunk-length", &chunkLengthSecs_,
              "Length of chunk size in seconds, that we process.");
  po.Register("output-period", &outputPeriod,
              "How often in seconds, do we check for changes in output.");
  po.Register("num-threads-startup", &g_num_threads,
              "Number of threads used when initializing iVector extractor.");
  po.Register("read-timeout", &readTimeout_,
              "Number of seconds of timout for TCP audio data to appear on the stream. Use -1 "
              "for blocking.");
  po.Register(
      "produce-time", &produceTime_,
      "Prepend begin/end times between endpoints (e.g. '5.46 6.81 <text_output>', in seconds)");

  featureOpts_.Register(&po);
  decodableOpts_.Register(&po);
  decoderOpts_.Register(&po);
  endpointOpts_.Register(&po);

  po.Read(argc, argv);

  if (po.NumArgs() != 3) {
    po.PrintUsage();
  }

  const std::string nnet3_rxfilename = po.GetArg(1);
  const std::string fst_rxfilename = po.GetArg(2);
  const std::string word_syms_filename = po.GetArg(3);

  featureInfoPtr_ = std::make_unique<OnlineNnet2FeaturePipelineInfo>(featureOpts_);
  SPDLOG_INFO("Constructed OnlineNnet2FeaturePipelineInfo");

  frameShift_ = featureInfoPtr_->FrameShiftInSeconds();
  frameSubsampling_ = decodableOpts_.frame_subsampling_factor;

  {
    SPDLOG_INFO("Loading acoustic model...");
    bool binary;
    Input ki(nnet3_rxfilename, &binary);
    transModel_.Read(ki.Stream(), binary);
    amNnet_.Read(ki.Stream(), binary);
    SetBatchnormTestMode(true, &(amNnet_.GetNnet()));
    SetDropoutTestMode(true, &(amNnet_.GetNnet()));
    nnet3::CollapseModel(nnet3::CollapseModelConfig(), &(amNnet_.GetNnet()));
    SPDLOG_INFO("Loaded acoustic model");
  }

  decodableInfoPtr_ =
      std::make_unique<nnet3::DecodableNnetSimpleLoopedInfo>(decodableOpts_, &amNnet_);

  SPDLOG_INFO("Loading FST...");
  decodeFst_ = fst::ReadFstKaldiGeneric(fst_rxfilename);
  if (!word_syms_filename.empty()) {
    if (!(wordSyms_ = fst::SymbolTable::ReadText(word_syms_filename))) {
      SPDLOG_ERROR("Could not read symbol table from file {}", word_syms_filename);
    }
  }
  SPDLOG_INFO("Loaded FST");

  sampCount = 0; // this is used for output refresh rate
  chunkLen_ = static_cast<size_t>(chunkLengthSecs_ * sampFreq_);
  checkPeriod_ = static_cast<int32>(sampFreq_ * outputPeriod);
  checkCount_ = checkPeriod_;
  frameOffset_ = 0;

  SPDLOG_INFO("Config options:");
  SPDLOG_INFO("  sample frequency: {} Hz", sampFreq_);
  SPDLOG_INFO("  chunk length: {} seconds, {} samples", chunkLengthSecs_, chunkLen_);

  featurePipelinePtr_ = std::make_unique<OnlineNnet2FeaturePipeline>(*featureInfoPtr_);
  SPDLOG_DEBUG("Constructed OnlineNnet2FeaturePipeline");

  decoderPtr_ = std::make_unique<SingleUtteranceNnet3Decoder>(
      decoderOpts_, transModel_, *decodableInfoPtr_, *decodeFst_, featurePipelinePtr_.get());
  SPDLOG_DEBUG("Constructed SingleUtteranceNnet3Decoder");

  SPDLOG_INFO("Constructed Nnet3Data");
}

/**
 * LatticeToString
 */
std::string LatticeToString(const Lattice& lat, const fst::SymbolTable& wordSyms) {
  LatticeWeight weight;
  std::vector<int32> alignment;
  std::vector<int32> words;
  GetLinearSymbolSequence(lat, &alignment, &words, &weight);

  std::ostringstream msg;
  for (size_t i = 0; i < words.size(); i++) {
    std::string s = wordSyms.Find(words[i]);
    if (s.empty()) {
      KALDI_WARN << "Word-id " << words[i] << " not in symbol table.";
      msg << "<#" << std::to_string(i) << "> ";
    } else
      msg << s << " ";
  }
  return msg.str();
}

/**
 * LatticeToString
 */
std::string LatticeToString(const CompactLattice& clat, const fst::SymbolTable& wordSyms) {
  if (clat.NumStates() == 0) {
    KALDI_WARN << "Empty lattice.";
    return "";
  }
  CompactLattice best_path_clat;
  CompactLatticeShortestPath(clat, &best_path_clat);

  Lattice best_path_lat;
  ConvertLattice(best_path_clat, &best_path_lat);
  return LatticeToString(best_path_lat, wordSyms);
}

/**
 * GetTimeString
 */
std::string GetTimeString(int32 t_beg, int32 t_end, BaseFloat time_unit) {
  std::array<char, 100> buffer; //
  double t_beg2 =
      static_cast<BaseFloat>(t_beg) * time_unit; // NOLINT: Scared to fix Kaldi code without tests
  double t_end2 =
      static_cast<BaseFloat>(t_end) * time_unit; // NOLINT: Scared to fix Kaldi code without tests
  snprintf(buffer.data(), 100, "%.2f %.2f", t_beg2, t_end2);
  return std::string(buffer.data());
}

/**
 * GetLatticeTimeSpan
 */
int32 GetLatticeTimeSpan(const Lattice& lat) {
  std::vector<int32> times;
  LatticeStateTimes(lat, &times);
  return times.back();
}

} // namespace mik