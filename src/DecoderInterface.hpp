#pragma once

#include <boost/asio.hpp>

constexpr long MaxLength = 1024;

namespace mik {
    // Used for sending audio to the Kaldi online2-tcp-nnet3-decode-faster server

    // Given a stream of audio, send it over TCP
    class DecoderInterface {
        public:
            void connect();
        private:
            boost::asio::ip::tcp socket_;
    }

}