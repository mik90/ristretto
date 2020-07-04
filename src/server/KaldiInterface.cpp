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

#include <spdlog/spdlog.h>
#include <string>

#include "KaldiInterface.hpp"

namespace kaldi {

// --------------------------------------------------------------------------------------
// convertStringToInt16
// --------------------------------------------------------------------------------------
std::vector<int16_t> convertStringToInt16(const std::string& str) {
  // Expect that two chars will be the same size of an int16_t
  // Since this function relies upon combining two chars into an int16
  static_assert((2 * sizeof(char)) == sizeof(int16_t));

  std::vector<int16_t> converted_buffer;
  size_t values_to_convert = str.length();
  bool add_extra_val_at_end = false;

  // Length is odd
  if (values_to_convert % 2 != 0) {
    // TODO Make this neater, just ensure that it's an even value
    SPDLOG_DEBUG("DEBUG: values_to_convert is odd:{}", values_to_convert);
    values_to_convert--;
    add_extra_val_at_end = true;
  }
  for (size_t i = 0; i < values_to_convert; i += 2) {
    // Combine 2 chars into a 16 bit value
    const auto lower = static_cast<uint32_t>(str[i]);
    const auto upper = static_cast<uint32_t>(str[i + 1]);
    constexpr uint32_t bits_to_shift_by = 8;
    const auto combined = static_cast<int16_t>((upper << bits_to_shift_by) | lower);
    converted_buffer.push_back(combined);
  }

  if (add_extra_val_at_end) {
    converted_buffer.push_back(static_cast<int16_t>(str.back()));
  }

  return converted_buffer;
}

// --------------------------------------------------------------------------------------
// convertBytesToFloatVec
// --------------------------------------------------------------------------------------
Vector<BaseFloat> convertBytesToFloatVec(std::unique_ptr<std::string> audioDataPtr) {

  if (!audioDataPtr) {
    SPDLOG_ERROR("audioDataPtr was null!");
    return {};
  } else if (audioDataPtr->empty()) {
    SPDLOG_ERROR("audioData was empty!");
    return {};
  }
  SPDLOG_DEBUG("audioDataPtr string as int32 (first 20 values)");
  for (size_t i = 0; i < 20; i += 2) {
    SPDLOG_DEBUG("EVEN audioDataPtr->at({}):{}", i, static_cast<int32_t>(audioDataPtr->at(i)));
    SPDLOG_DEBUG("ODD audioDataPtr->at({}):{}", i + 1,
                 static_cast<int32_t>(audioDataPtr->at(i + 1)));
  }
  const auto buffer_int16 = convertStringToInt16(*audioDataPtr);
  SPDLOG_DEBUG("buffer_int16 (first 20 values)");
  for (size_t i = 0; i < 20; ++i) {
    SPDLOG_DEBUG("buffer_int16({}):{}", i, buffer_int16[i]);
  }
  const size_t buffer_size = buffer_int16.size();
  SPDLOG_DEBUG("buffer_int16.size():{}", buffer_size);

  Vector<BaseFloat> audio_data_float;
  // Ensure that there's enough space in the output Vector
  audio_data_float.Resize(static_cast<MatrixIndexT>(buffer_size), kUndefined);

  for (size_t i = 0; i < buffer_size; ++i) {
    // Convert each char in audioDataPtr into a BaseFloat for audio_data_float
    // operator () is used for indexing Kaldi vectors
    audio_data_float(static_cast<MatrixIndexT>(i)) = static_cast<BaseFloat>(buffer_int16[i]);
  }
  SPDLOG_DEBUG("audio_data_float.SizeInBytes():{}", audio_data_float.SizeInBytes());

  SPDLOG_DEBUG("audio_data_float (first 20 values)");
  for (int i = 0; i < 20; ++i) {
    SPDLOG_DEBUG("audio_data_float({}):{}", i, audio_data_float(i));
  }
  return audio_data_float;
}

// --------------------------------------------------------------------------------------
// Nnet3Data::decodeAudio
// --------------------------------------------------------------------------------------
std::string Nnet3Data::decodeAudio(std::unique_ptr<std::string> audioDataPtr) {

  SPDLOG_DEBUG("decodeAudioChunk entry");
  const Vector<BaseFloat> complete_audio_data = convertBytesToFloatVec(std::move(audioDataPtr));
  SPDLOG_DEBUG("complete_audio_data size in bytes:{}", complete_audio_data.SizeInBytes());
  SPDLOG_DEBUG("complete_audio_data Dim (elements):{}", complete_audio_data.Dim());

  SPDLOG_DEBUG("Getting lock on mutex...");
  // No idea how thread-safe Kaldi is so naively lock at the beginning of this method
  std::lock_guard<std::mutex> lock(decoder_mutex);
  SPDLOG_DEBUG("Got lock");

  decoder_ptr->InitDecoding(frame_offset);
  SPDLOG_INFO("Initialized decoding");

  silence_weighting_ptr = std::make_unique<OnlineSilenceWeighting>(
      trans_model, feature_info_ptr->silence_weighting_config,
      decodable_opts.frame_subsampling_factor);
  SPDLOG_DEBUG("Constructed OnlineSilenceWeighting");

  // For debugging purposes
  if (!feature_pipeline_ptr) {
    SPDLOG_ERROR("feature_pipeline_ptr was null!");
    return {};
  } else if (!silence_weighting_ptr) {
    SPDLOG_ERROR("silence_weighting_ptr was null!");
    return {};
  } else if (!decoder_ptr) {
    SPDLOG_ERROR("decoder_ptr was null!");
    return {};
  }
  try {
    std::string output;
    size_t iteration_count = 0;
    while (true) {
      SPDLOG_DEBUG("iteration_count {} ", iteration_count);

      // Get a usable chunk out of the audio data, this range will keep moving over the entire audio
      // data range
      auto samples_to_read = complete_audio_data.Dim() - samp_count;
      // Don't read more than a chunk
      samples_to_read = std::min(samples_to_read, static_cast<int32>(chunk_len));
      // Don't try to read a negative number
      samples_to_read = std::max(0, samples_to_read);
      SPDLOG_DEBUG("samples_to_read {} vs chunk_len {}", samples_to_read, chunk_len);

      if (samples_to_read == 0) {
        SPDLOG_INFO("End of stream, no more samples left. samp_count {}", samp_count);
        break;
      }

      // Equivalent to GetChunk
      SPDLOG_DEBUG("creating SubVector with Range({},{})", samp_count, samples_to_read);
      const auto sub_vec = complete_audio_data.Range(samp_count, samples_to_read);
      SPDLOG_DEBUG("created SubVector dim: {}, size in bytes:{}", sub_vec.Dim(),
                   sub_vec.SizeInBytes());
      SPDLOG_DEBUG("creating audio_chunk copy of SubVector");
      Vector<BaseFloat> audio_chunk(sub_vec);
      SPDLOG_DEBUG("created audio_chunk copy of SubVector dim: {}, size in bytes:{}",
                   audio_chunk.Dim(), audio_chunk.SizeInBytes());

      SPDLOG_DEBUG("GetChunk as BaseFloat (first 20 values)");
      for (int i = 0; i < 20; ++i) {
        SPDLOG_DEBUG("audio_chunk({}):{}", i, audio_chunk(i));
      }

      feature_pipeline_ptr->AcceptWaveform(samp_freq, audio_chunk);
      samp_count += static_cast<int32>(chunk_len);
      SPDLOG_INFO("Chunk length:{}, Total sample count:{}", chunk_len, samp_count);

      if (silence_weighting_ptr->Active() && feature_pipeline_ptr->IvectorFeature() != nullptr) {
        silence_weighting_ptr->ComputeCurrentTraceback(decoder_ptr->Decoder());
        silence_weighting_ptr->GetDeltaWeights(
            feature_pipeline_ptr->NumFramesReady(),
            frame_offset * decodable_opts.frame_subsampling_factor, &delta_weights);
        feature_pipeline_ptr->UpdateFrameWeights(delta_weights);
        SPDLOG_DEBUG("Adjusted silence weighting");
      }

      SPDLOG_DEBUG("Advancing decoding...");
      decoder_ptr->AdvanceDecoding();
      SPDLOG_DEBUG("Decoding advanced");

      // SPDLOG_DEBUG("samp_count:{}, check_count:{}", samp_count, check_count);
      if (samp_count > check_count) {
        SPDLOG_DEBUG("samp_count:{} > check_count:{}", samp_count, check_count);
        const auto num_frames_decoded = decoder_ptr->NumFramesDecoded();
        if (num_frames_decoded > 0) {
          SPDLOG_DEBUG("decoded {} frames", num_frames_decoded);
          Lattice lat;
          decoder_ptr->GetBestPath(/* end of utt */ false, &lat);
          TopSort(&lat); // for LatticeStateTimes(),
          std::string msg = LatticeToString(lat, *word_syms);

          // get time-span after previous endpoint,
          if (produce_time) {
            int32 t_beg = frame_offset;
            int32 t_end = frame_offset + GetLatticeTimeSpan(lat);
            msg = GetTimeString(t_beg, t_end,
                                frame_shift * static_cast<BaseFloat>(frame_subsampling)) +
                  " " + msg;
          }

          SPDLOG_INFO("Temporary transcript: {}", msg);
          output += msg;
        }
        check_count += check_period;
      }

      if (decoder_ptr->EndpointDetected(endpoint_opts)) {
        SPDLOG_INFO("Endpoint detected");
        decoder_ptr->FinalizeDecoding();
        frame_offset += decoder_ptr->NumFramesDecoded();
        CompactLattice lat;
        decoder_ptr->GetLattice(true, &lat);
        std::string msg = LatticeToString(lat, *word_syms);

        // get time-span between endpoints,
        if (produce_time) {
          int32 t_beg = frame_offset - decoder_ptr->NumFramesDecoded();
          int32 t_end = frame_offset;
          msg =
              GetTimeString(t_beg, t_end, frame_shift * static_cast<BaseFloat>(frame_subsampling)) +
              " " + msg;
        }

        SPDLOG_INFO("Endpoint, sending message: {}", msg);
        output += msg;
        break;
      }
      SPDLOG_DEBUG("end of iteration {} ", iteration_count++);
    } // end of while(true) loop

    SPDLOG_INFO("Input finished");
    feature_pipeline_ptr->InputFinished();
    if (silence_weighting_ptr->Active() && feature_pipeline_ptr->IvectorFeature() != nullptr) {
      silence_weighting_ptr->ComputeCurrentTraceback(decoder_ptr->Decoder());
      silence_weighting_ptr->GetDeltaWeights(feature_pipeline_ptr->NumFramesReady(),
                                             frame_offset * decodable_opts.frame_subsampling_factor,
                                             &delta_weights);
      feature_pipeline_ptr->UpdateFrameWeights(delta_weights);
      SPDLOG_DEBUG("Adjusted silence weighting");
    }

    SPDLOG_DEBUG("Advancing decoding again...");
    decoder_ptr->AdvanceDecoding();
    SPDLOG_DEBUG("Decoding advanced");
    decoder_ptr->FinalizeDecoding();
    SPDLOG_DEBUG("Decoding finalized");
    frame_offset += decoder_ptr->NumFramesDecoded();
    SPDLOG_DEBUG("frame_offset:{}, NumFramesDecoded:{}", frame_offset,
                 decoder_ptr->NumFramesDecoded());
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
    : feature_opts(), decodable_opts(), decoder_opts(), endpoint_opts() {

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

  feature_info_ptr = std::make_unique<OnlineNnet2FeaturePipelineInfo>(feature_opts);
  SPDLOG_INFO("Constructed OnlineNnet2FeaturePipelineInfo");

  frame_shift = feature_info_ptr->FrameShiftInSeconds();
  frame_subsampling = decodable_opts.frame_subsampling_factor;

  {
    SPDLOG_INFO("Loading acoustic model...");
    bool binary;
    Input ki(nnet3_rxfilename, &binary);
    trans_model.Read(ki.Stream(), binary);
    am_nnet.Read(ki.Stream(), binary);
    SetBatchnormTestMode(true, &(am_nnet.GetNnet()));
    SetDropoutTestMode(true, &(am_nnet.GetNnet()));
    nnet3::CollapseModel(nnet3::CollapseModelConfig(), &(am_nnet.GetNnet()));
    SPDLOG_INFO("Loaded acoustic model");
  }

  decodable_info_ptr =
      std::make_unique<nnet3::DecodableNnetSimpleLoopedInfo>(decodable_opts, &am_nnet);

  SPDLOG_INFO("Loading FST...");
  decode_fst = fst::ReadFstKaldiGeneric(fst_rxfilename);
  if (!word_syms_filename.empty()) {
    if (!(word_syms = fst::SymbolTable::ReadText(word_syms_filename))) {
      SPDLOG_ERROR("Could not read symbol table from file {}", word_syms_filename);
    }
  }
  SPDLOG_INFO("Loaded FST");

  samp_count = 0; // this is used for output refresh rate
  chunk_len = static_cast<size_t>(chunk_length_secs * samp_freq);
  check_period = static_cast<int32>(samp_freq * output_period);
  check_count = check_period;
  frame_offset = 0;

  SPDLOG_INFO("Config options:");
  SPDLOG_INFO("       samp_freq: {} Hz", samp_freq);
  SPDLOG_INFO("    chunk_length: {} seconds", chunk_length_secs);
  SPDLOG_INFO("    chunk_length: {} samples", chunk_len);
  SPDLOG_INFO("     check_count: {} samples", check_period);

  feature_pipeline_ptr = std::make_unique<OnlineNnet2FeaturePipeline>(*feature_info_ptr);
  SPDLOG_DEBUG("Constructed OnlineNnet2FeaturePipeline");

  decoder_ptr = std::make_unique<SingleUtteranceNnet3Decoder>(
      decoder_opts, trans_model, *decodable_info_ptr, *decode_fst, feature_pipeline_ptr.get());
  SPDLOG_DEBUG("Constructed SingleUtteranceNnet3Decoder");

  SPDLOG_INFO("Constructed Nnet3Data");
}

// --------------------------------------------------------------------------------------
// LatticeTostring
// --------------------------------------------------------------------------------------
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

// --------------------------------------------------------------------------------------
// LatticeToString
// --------------------------------------------------------------------------------------
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