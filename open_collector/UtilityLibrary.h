#ifndef UTILITYLIBRARY_H
#define UTILITYLIBRARY_H


#include <boost/asio.hpp>
#include <memory>
#include <vector>

typedef std::shared_ptr<boost::asio::io_service> service_ptr;
typedef std::shared_ptr<std::vector<unsigned char>> frame_ptr;
typedef std::shared_ptr<boost::asio::ip::tcp::socket> socket_ptr;

class ProxyBase{
public:
    virtual ~ProxyBase(){}
};
typedef std::shared_ptr<ProxyBase> ProxyBasePtr;


#endif // UTILITYLIBRARY_H
