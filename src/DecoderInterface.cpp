#include <boost/asio.hpp>
#include <spdlog/spdlog.h> 
#include <spdlog/sinks/basic_file_sink.h>
#include <fmt/core.h>

#include <iostream>

#include "DecoderInterface.hpp"

namespace mik {
    DecoderInterface::DecoderInterface() : ioContext_(), socket_(ioContext_) {
        try {
            logger_ = spdlog::basic_logger_mt("DecodeInterfaceLogger", "logs/client.log");
        }
        catch(const spdlog::spdlog_ex& e) {
            std::cerr << "Log init failed with spdlog_ex: " << e.what() << std::endl;
        }
        
        logger_->info("DecoderInterface created.");
        logger_->set_level(spdlog::level::level_enum::debug);
        logger_->debug("Debug-level logging enabled");
    }

    void DecoderInterface::connect(std::string_view host, std::string_view port) {

        boost::asio::ip::tcp::resolver res(ioContext_);

        boost::asio::ip::tcp::resolver::results_type endpoints;
        try {
            endpoints = res.resolve(host, port);

        } catch(const boost::system::system_error& e) {
            logger_->error("Error {}", e.what());
            return;
        }

        logger_->info("Retrieved endpoints");

        try {
            boost::asio::connect(socket_, endpoints);
            logger_->info("Connected to {}:{}", host, port);

        } catch (const boost::system::system_error& e) {
            logger_->info("Caught exception on connect(): {}", e.what());
            return;
        }
        
    }

    size_t DecoderInterface::sendAudio(const std::unique_ptr<char>& buffer, size_t nBytes) {
        // Write out data to socket
        const auto buf = boost::asio::const_buffer(buffer.get(), nBytes);
        const auto bytesWritten = boost::asio::write(socket_, buf);
        logger_->debug("Wrote out {} bytes of audio", bytesWritten);
        return bytesWritten;
    }

    std::string DecoderInterface::readResults() {
        // Read in reply 
        boost::asio::streambuf streamBuf;
        const auto bytesRead = boost::asio::read_until(socket_, streamBuf, "\n");
        logger_->debug("Read in {} bytes of results");
        const auto bufIter = streamBuf.data();
        const std::string result(boost::asio::buffers_begin(bufIter),
                                 boost::asio::buffers_begin(bufIter) + static_cast<long>(bytesRead));
        return result;
    }
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {
    mik::DecoderInterface iface;
    iface.connect("127.0.0.1", "5050");


    return 0;
}