#ifndef MODULELIBRARY_H
#define MODULELIBRARY_H


#include <boost/asio.hpp>
#include <memory>
#include <vector>

namespace open_collector{

typedef std::shared_ptr<boost::asio::io_service> service_ptr;
typedef std::shared_ptr<std::vector<unsigned char>> frame_ptr;
typedef std::shared_ptr<boost::asio::ip::tcp::socket> socket_ptr;

static std::function<void (const std::string&)> s_log_error;

class ProxyBase{
public:
    virtual ~ProxyBase(){}
};
typedef std::shared_ptr<ProxyBase> ProxyBasePtr;

}

#endif // MODULELIBRARY_H
