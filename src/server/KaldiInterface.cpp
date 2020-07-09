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

namespace kaldi {

// --------------------------------------------------------------------------------------
// deserializeAudioData
// --------------------------------------------------------------------------------------
Vector<BaseFloat> deserializeAudioData(std::unique_ptr<std::string> audioDataPtr) {

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

// --------------------------------------------------------------------------------------
// Nnet3Data::decodeAudio
// --------------------------------------------------------------------------------------
std::string Nnet3Data::decodeAudio(std::unique_ptr<std::string> audioDataPtr) {

  SPDLOG_DEBUG("decodeAudioChunk entry");
  const Vector<BaseFloat> complete_audio_data = deserializeAudioData(std::move(audioDataPtr));
  SPDLOG_DEBUG("complete_audio_data size in bytes:{}, number of elements:{}",
               complete_audio_data.SizeInBytes(), complete_audio_data.Dim());

  SPDLOG_DEBUG("Getting lock on mutex...");
  // No idea how thread-safe Kaldi is so naively lock at the beginning of this method
  std::lock_guard<std::mutex> lock(decoderMutex);
  SPDLOG_DEBUG("Got lock");

  decoderPtr->InitDecoding(frameOffset);
  SPDLOG_INFO("Initialized decoding");

  silenceWeightingPtr = std::make_unique<OnlineSilenceWeighting>(
      transModel, featureInfoPtr->silence_weighting_config, decodableOpts.frame_subsampling_factor);
  SPDLOG_DEBUG("Constructed OnlineSilenceWeighting");

  // For debugging purposes
  if (!featurePipelinePtr) {
    SPDLOG_ERROR("feature_pipeline_ptr was null!");
    return {};
  } else if (!silenceWeightingPtr) {
    SPDLOG_ERROR("silenceWeightingPtr was null!");
    return {};
  } else if (!decoderPtr) {
    SPDLOG_ERROR("decoderPtr was null!");
    return {};
  }
  try {
    std::string output;
    while (true) {

      // Get a usable chunk out of the audio data, this range will keep moving over the entire audio
      // data range
      auto samplesToRead = complete_audio_data.Dim() - sampCount;
      // Don't read more than a chunk
      samplesToRead = std::min(samplesToRead, static_cast<int32>(chunkLen));
      // Don't try to read a negative number
      samplesToRead = std::max(0, samplesToRead);
      SPDLOG_DEBUG("samplesToRead {} vs chunkLen {}", samplesToRead, chunkLen);

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

      featurePipelinePtr->AcceptWaveform(sampFreq, audio_chunk);
      sampCount += static_cast<int32>(chunkLen);
      SPDLOG_INFO("Chunk length:{}, Total sample count:{}", chunkLen, sampCount);

      if (silenceWeightingPtr->Active() && featurePipelinePtr->IvectorFeature() != nullptr) {
        silenceWeightingPtr->ComputeCurrentTraceback(decoderPtr->Decoder());
        silenceWeightingPtr->GetDeltaWeights(featurePipelinePtr->NumFramesReady(),
                                             frameOffset * decodableOpts.frame_subsampling_factor,
                                             &deltaWeights);
        featurePipelinePtr->UpdateFrameWeights(deltaWeights);
        SPDLOG_DEBUG("Adjusted silence weighting");
      }

      SPDLOG_DEBUG("Advancing decoding...");
      decoderPtr->AdvanceDecoding();
      SPDLOG_DEBUG("Decoding advanced");

      // SPDLOG_DEBUG("sampCount:{}, checkCount:{}", sampCount, checkCount);
      if (sampCount > checkCount) {
        SPDLOG_DEBUG("sampCount:{} > checkCount:{}", sampCount, checkCount);
        const auto num_frames_decoded = decoderPtr->NumFramesDecoded();
        if (num_frames_decoded > 0) {
          SPDLOG_DEBUG("decoded {} frames", num_frames_decoded);
          Lattice lat;
          decoderPtr->GetBestPath(/* end of utt */ false, &lat);
          TopSort(&lat); // for LatticeStateTimes(),
          std::string msg = LatticeToString(lat, *wordSyms);

          // get time-span after previous endpoint,
          if (produceTime) {
            int32 t_beg = frameOffset;
            int32 t_end = frameOffset + GetLatticeTimeSpan(lat);
            msg =
                GetTimeString(t_beg, t_end, frameShift * static_cast<BaseFloat>(frameSubsampling)) +
                " " + msg;
          }

          SPDLOG_INFO("Temporary transcript: {}", msg);
        }
        checkCount += checkPeriod;
      }

      if (decoderPtr->EndpointDetected(endpointOpts)) {
        SPDLOG_INFO("Endpoint detected");
        decoderPtr->FinalizeDecoding();
        frameOffset += decoderPtr->NumFramesDecoded();
        CompactLattice lat;
        decoderPtr->GetLattice(true, &lat);
        std::string msg = LatticeToString(lat, *wordSyms);

        // get time-span between endpoints,
        if (produceTime) {
          int32 t_beg = frameOffset - decoderPtr->NumFramesDecoded();
          int32 t_end = frameOffset;
          msg = GetTimeString(t_beg, t_end, frameShift * static_cast<BaseFloat>(frameSubsampling)) +
                " " + msg;
        }

        SPDLOG_INFO("Endpoint, sending message: {}", msg);
        output += msg;
        break;
      }
    } // end of while(true) loop

    SPDLOG_INFO("Input finished");
    featurePipelinePtr->InputFinished();
    if (silenceWeightingPtr->Active() && featurePipelinePtr->IvectorFeature() != nullptr) {
      silenceWeightingPtr->ComputeCurrentTraceback(decoderPtr->Decoder());
      silenceWeightingPtr->GetDeltaWeights(featurePipelinePtr->NumFramesReady(),
                                           frameOffset * decodableOpts.frame_subsampling_factor,
                                           &deltaWeights);
      featurePipelinePtr->UpdateFrameWeights(deltaWeights);
      SPDLOG_DEBUG("Adjusted silence weighting");
    }

    decoderPtr->AdvanceDecoding();
    decoderPtr->FinalizeDecoding();
    const auto numFramesDecoded = decoderPtr->NumFramesDecoded();
    frameOffset += numFramesDecoded;
    SPDLOG_DEBUG("frameOffset:{}, NumFramesDecoded:{}", frameOffset, numFramesDecoded);
    if (numFramesDecoded > 0) {
      CompactLattice lat;
      decoderPtr->GetLattice(true, &lat);
      std::string msg = LatticeToString(lat, *wordSyms);

      // get time-span from previous endpoint to end of audio,
      if (produceTime) {
        int32 t_beg = frameOffset - decoderPtr->NumFramesDecoded();
        int32 t_end = frameOffset;
        msg = GetTimeString(t_beg, t_end, frameShift * static_cast<BaseFloat>(frameSubsampling)) +
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

// --------------------------------------------------------------------------------------
// Nnet3Data::Nnet3Data
// --------------------------------------------------------------------------------------
Nnet3Data::Nnet3Data(int argc, char* argv[])
    : featureOpts(), decodableOpts(), decoderOpts(), endpointOpts() {

  SPDLOG_INFO("Constructing Nnet3Data");
  const char* usage = "Reads in audio from a network socket and performs online\n"
                      "decoding with neural nets (nnet3 setup), with iVector-based\n"
                      "speaker adaptation and endpointing.\n"
                      "Note: some configuration values and inputs are set via config\n"
                      "files whose filenames are passed as options\n"
                      "\n"
                      "Usage: online2-tcp-nnet3-decode-faster [options] <nnet3-in> "
                      "<fst-in> <word-symbol-table>\n";

  ParseOptions po(usage);

  chunkLengthSecs = 0.18f;
  outputPeriod = 1;
  sampFreq = 16000.0;
  portNum = 5050;
  readTimeout = 3;
  produceTime = false;

  po.Register("samp-freq", &sampFreq,
              "Sampling frequency of the input signal (coded as 16-bit slinear).");
  po.Register("chunk-length", &chunkLengthSecs,
              "Length of chunk size in seconds, that we process.");
  po.Register("output-period", &outputPeriod,
              "How often in seconds, do we check for changes in output.");
  po.Register("num-threads-startup", &g_num_threads,
              "Number of threads used when initializing iVector extractor.");
  po.Register("read-timeout", &readTimeout,
              "Number of seconds of timout for TCP audio data to appear on the stream. Use -1 "
              "for blocking.");
  po.Register("port-num", &portNum, "Port number the server will listen on.");
  po.Register(
      "produce-time", &produceTime,
      "Prepend begin/end times between endpoints (e.g. '5.46 6.81 <text_output>', in seconds)");

  featureOpts.Register(&po);
  decodableOpts.Register(&po);
  decoderOpts.Register(&po);
  endpointOpts.Register(&po);

  po.Read(argc, argv);

  if (po.NumArgs() != 3) {
    po.PrintUsage();
  }

  const std::string nnet3_rxfilename = po.GetArg(1);
  const std::string fst_rxfilename = po.GetArg(2);
  const std::string word_syms_filename = po.GetArg(3);

  featureInfoPtr = std::make_unique<OnlineNnet2FeaturePipelineInfo>(featureOpts);
  SPDLOG_INFO("Constructed OnlineNnet2FeaturePipelineInfo");

  frameShift = featureInfoPtr->FrameShiftInSeconds();
  frameSubsampling = decodableOpts.frame_subsampling_factor;

  {
    SPDLOG_INFO("Loading acoustic model...");
    bool binary;
    Input ki(nnet3_rxfilename, &binary);
    transModel.Read(ki.Stream(), binary);
    amNnet.Read(ki.Stream(), binary);
    SetBatchnormTestMode(true, &(amNnet.GetNnet()));
    SetDropoutTestMode(true, &(amNnet.GetNnet()));
    nnet3::CollapseModel(nnet3::CollapseModelConfig(), &(amNnet.GetNnet()));
    SPDLOG_INFO("Loaded acoustic model");
  }

  decodableInfoPtr = std::make_unique<nnet3::DecodableNnetSimpleLoopedInfo>(decodableOpts, &amNnet);

  SPDLOG_INFO("Loading FST...");
  decodeFst = fst::ReadFstKaldiGeneric(fst_rxfilename);
  if (!word_syms_filename.empty()) {
    if (!(wordSyms = fst::SymbolTable::ReadText(word_syms_filename))) {
      SPDLOG_ERROR("Could not read symbol table from file {}", word_syms_filename);
    }
  }
  SPDLOG_INFO("Loaded FST");

  sampCount = 0; // this is used for output refresh rate
  chunkLen = static_cast<size_t>(chunkLengthSecs * sampFreq);
  checkPeriod = static_cast<int32>(sampFreq * outputPeriod);
  checkCount = checkPeriod;
  frameOffset = 0;

  SPDLOG_INFO("Config options:");
  SPDLOG_INFO("       sampFreq: {} Hz", sampFreq);
  SPDLOG_INFO("    chunk_length: {} seconds", chunkLengthSecs);
  SPDLOG_INFO("    chunk_length: {} samples", chunkLen);
  SPDLOG_INFO("     checkCount: {} samples", checkPeriod);

  featurePipelinePtr = std::make_unique<OnlineNnet2FeaturePipeline>(*featureInfoPtr);
  SPDLOG_DEBUG("Constructed OnlineNnet2FeaturePipeline");

  decoderPtr = std::make_unique<SingleUtteranceNnet3Decoder>(
      decoderOpts, transModel, *decodableInfoPtr, *decodeFst, featurePipelinePtr.get());
  SPDLOG_DEBUG("Constructed SingleUtteranceNnet3Decoder");

  SPDLOG_INFO("Constructed Nnet3Data");
}

// --------------------------------------------------------------------------------------
// LatticeTostring
// --------------------------------------------------------------------------------------
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

// --------------------------------------------------------------------------------------
// LatticeToString
// --------------------------------------------------------------------------------------
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

// --------------------------------------------------------------------------------------
// GetTimeString
// --------------------------------------------------------------------------------------
std::string GetTimeString(int32 t_beg, int32 t_end, BaseFloat time_unit) {
  char buffer[100];
  double t_beg2 = static_cast<BaseFloat>(t_beg) * time_unit;
  double t_end2 = static_cast<BaseFloat>(t_end) * time_unit;
  snprintf(buffer, 100, "%.2f %.2f", t_beg2, t_end2);
  return std::string(buffer);
}

// --------------------------------------------------------------------------------------
// GetLatticeTimeSpan
// --------------------------------------------------------------------------------------
int32 GetLatticeTimeSpan(const Lattice& lat) {
  std::vector<int32> times;
  LatticeStateTimes(lat, &times);
  return times.back();
}

} // namespace kaldi