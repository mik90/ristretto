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

#include "KaldiInterface.hpp"
#include "Utils.hpp"

#include <fmt/core.h>
#include <fmt/locale.h>
#include <spdlog/spdlog.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

namespace mik {}

int main(int argc, char* argv[]) {
  mik::createLogger();
  try {
    using namespace kaldi;
    using namespace fst;

    const char* usage = "Reads in audio from a network socket and performs online\n"
                        "decoding with neural nets (nnet3 setup), with iVector-based\n"
                        "speaker adaptation and endpointing.\n"
                        "Note: some configuration values and inputs are set via config\n"
                        "files whose filenames are passed as options\n"
                        "\n"
                        "Usage: online2-tcp-nnet3-decode-faster [options] <nnet3-in> "
                        "<fst-in> <word-symbol-table>\n";

    ParseOptions po(usage);

    // feature_opts includes configuration for the iVector adaptation,
    // as well as the basic features.
    OnlineNnet2FeaturePipelineConfig feature_opts;
    nnet3::NnetSimpleLoopedComputationOptions decodable_opts;
    LatticeFasterDecoderConfig decoder_opts;
    OnlineEndpointConfig endpoint_opts;

    BaseFloat chunk_length_secs = 0.18;
    BaseFloat output_period = 1;
    BaseFloat samp_freq = 16000.0;
    int port_num = 5050;
    int read_timeout = 3;
    bool produce_time = false;

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
      return 1;
    }

    const std::string nnet3_rxfilename = po.GetArg(1);
    const std::string fst_rxfilename = po.GetArg(2);
    const std::string word_syms_filename = po.GetArg(3);

    OnlineNnet2FeaturePipelineInfo feature_info(feature_opts);

    BaseFloat frame_shift = feature_info.FrameShiftInSeconds();
    int32 frame_subsampling = decodable_opts.frame_subsampling_factor;

    SPDLOG_INFO("Loading AM...");

    TransitionModel trans_model;
    nnet3::AmNnetSimple am_nnet;
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

    fst::Fst<fst::StdArc>* decode_fst = ReadFstKaldiGeneric(fst_rxfilename);

    fst::SymbolTable* word_syms = NULL;
    if (!word_syms_filename.empty()) {
      if (!(word_syms = fst::SymbolTable::ReadText(word_syms_filename))) {
        SPDLOG_ERROR("Could not read symbol table from file {}", word_syms_filename);
      }
    }

    signal(SIGPIPE,
           SIG_IGN); // ignore SIGPIPE to avoid crashing when socket forcefully disconnected

    // TOOD Create server here
    // mik::RistrettoServer server;
    // server.run();
    static_cast<void>(read_timeout);
    static_cast<void>(port_num);

    while (true) {

      // TODO Accept connection

      int32 samp_count = 0; // this is used for output refresh rate
      size_t chunk_len = static_cast<size_t>(chunk_length_secs * samp_freq);
      int32 check_period = static_cast<int32>(samp_freq * output_period);
      int32 check_count = check_period;

      int32 frame_offset = 0;

      bool eos = false;

      OnlineNnet2FeaturePipeline feature_pipeline(feature_info);
      SingleUtteranceNnet3Decoder decoder(decoder_opts, trans_model, decodable_info, *decode_fst,
                                          &feature_pipeline);

      while (!eos) {

        decoder.InitDecoding(frame_offset);
        OnlineSilenceWeighting silence_weighting(trans_model, feature_info.silence_weighting_config,
                                                 decodable_opts.frame_subsampling_factor);
        std::vector<std::pair<int32, BaseFloat>> delta_weights;

        while (true) {
          // TODO Read chunk of data
          // eos = !server.ReadChunk(chunk_len);

          if (eos) {
            feature_pipeline.InputFinished();
            decoder.AdvanceDecoding();
            decoder.FinalizeDecoding();
            frame_offset += decoder.NumFramesDecoded();
            if (decoder.NumFramesDecoded() > 0) {
              CompactLattice lat;
              decoder.GetLattice(true, &lat);
              std::string msg = LatticeToString(lat, *word_syms);

              // get time-span from previous endpoint to end of audio,
              if (produce_time) {
                int32 t_beg = frame_offset - decoder.NumFramesDecoded();
                int32 t_end = frame_offset;
                msg = GetTimeString(t_beg, t_end, frame_shift * frame_subsampling) + " " + msg;
              }

              KALDI_VLOG(1) << "EndOfAudio, sending message: " << msg;
              // TODO Respond with text
              // server.WriteLn(msg);
            } else
              // TODO Respond with text
              // server.Write("\n");
              // TODO Disconnect
              // server.Disconnect();
              break;
          }

          // TODO Get chunk
          Vector<BaseFloat> wave_part /*= server.GetChunk()*/;
          feature_pipeline.AcceptWaveform(samp_freq, wave_part);
          samp_count += chunk_len;

          if (silence_weighting.Active() && feature_pipeline.IvectorFeature() != NULL) {
            silence_weighting.ComputeCurrentTraceback(decoder.Decoder());
            silence_weighting.GetDeltaWeights(
                feature_pipeline.NumFramesReady(),
                frame_offset * decodable_opts.frame_subsampling_factor, &delta_weights);
            feature_pipeline.UpdateFrameWeights(delta_weights);
          }

          decoder.AdvanceDecoding();

          if (samp_count > check_count) {
            if (decoder.NumFramesDecoded() > 0) {
              Lattice lat;
              decoder.GetBestPath(false, &lat);
              TopSort(&lat); // for LatticeStateTimes(),
              std::string msg = LatticeToString(lat, *word_syms);

              // get time-span after previous endpoint,
              if (produce_time) {
                int32 t_beg = frame_offset;
                int32 t_end = frame_offset + GetLatticeTimeSpan(lat);
                msg = GetTimeString(t_beg, t_end, frame_shift * frame_subsampling) + " " + msg;
              }

              KALDI_VLOG(1) << "Temporary transcript: " << msg;
              // TODO
              // server.WriteLn(msg, "\r");
            }
            check_count += check_period;
          }

          if (decoder.EndpointDetected(endpoint_opts)) {
            decoder.FinalizeDecoding();
            frame_offset += decoder.NumFramesDecoded();
            CompactLattice lat;
            decoder.GetLattice(true, &lat);
            std::string msg = LatticeToString(lat, *word_syms);

            // get time-span between endpoints,
            if (produce_time) {
              int32 t_beg = frame_offset - decoder.NumFramesDecoded();
              int32 t_end = frame_offset;
              msg = GetTimeString(t_beg, t_end, frame_shift * frame_subsampling) + " " + msg;
            }

            KALDI_VLOG(1) << "Endpoint, sending message: " << msg;
            // TODO
            // server.WriteLn(msg);
            break; // while (true)
          }
        }
      }
    }
  } catch (const std::exception& e) {
    std::cerr << e.what();
    return -1;
  }
} // main()