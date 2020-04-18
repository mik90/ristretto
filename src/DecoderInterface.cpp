#include "DecoderInterface.hpp"

namespace mik {
    void DecoderInterface::connect() {
        using boost::asio::ip::tcp;
        boost::asio::io_context ioContext;
        tcp::socket sock(ioContext);
        tcp::resolver res(ioContext);
        boost::asio::connect(sock, resolver.resolve(host, port));
        
        // Write out data to socket
        //boost::asio::write(sock, boost::asio::buffer(outBuf, outLen));

        // Read in reply 
        //boost::asio::read(sock, boost::asio::buffer(inBuf, inLen));
    }
}