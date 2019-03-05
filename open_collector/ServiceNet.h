#ifndef SERVICENET_H
#define SERVICENET_H

#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <vector>
#include "UtilityLibrary.h"
#include "ServiceService.h"


class ServiceNet : public std::enable_shared_from_this<ServiceNet>
{
public:
    typedef std::shared_ptr<boost::asio::io_service> service_ptr;
    #define BUFFER_MAX 256

    virtual ~ServiceNet(){}

    virtual bool start(){
        if(!mp_serviceservice){
            mp_serviceservice = std::make_shared<ServiceServicePtr::element_type>();
        }
        service_ptr p_service = mp_service = mp_serviceservice->get_service_singleton();
        boost::asio::spawn(*p_service, std::bind(&ServiceNet::handle_net, shared_from_this(), std::placeholders::_1, p_service, 1234));
        return true;
    }

    virtual void stop(){
        mp_service.reset();
        auto p_serviceservice = mp_serviceservice;
        mp_serviceservice.reset();
        if(p_serviceservice){
            p_serviceservice->stop();
        }
    }

protected:
    virtual void handle_net(boost::asio::yield_context yield, service_ptr p_service, const int port){
        boost::asio::ip::tcp::acceptor acceptor(*p_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), static_cast<unsigned short>(port)));

        while(true) {
            boost::system::error_code ec;
            auto p_socket = socket_ptr(new socket_ptr::element_type(*p_service));
            acceptor.async_accept(*p_socket, yield[ec]);
            if(ec){
               break;
            }
            boost::asio::spawn(*p_service, std::bind(&ServiceNet::handle_read, shared_from_this()
                                                           , std::placeholders::_1, p_socket));
        }
    }
    virtual void handle_read(boost::asio::yield_context yield, socket_ptr p_socket){
        frame_ptr p_frame(new frame_ptr::element_type(BUFFER_MAX));
        frame_ptr p_frame_new;
        while(true){
            boost::system::error_code ec;
            std::size_t n = p_socket->async_read_some(boost::asio::buffer(p_frame->data(), BUFFER_MAX), yield[ec]);
            if(ec || 0 >= n){
                break;
            }
            p_frame_new = std::make_shared<frame_ptr::element_type>(p_frame->begin(), p_frame->begin() + static_cast<long long>(n));
            read_frame(p_frame_new, p_socket);
        }
    }

    virtual void log_error(const std::string& ){}
    virtual void read_frame(frame_ptr p_frame, socket_ptr p_socket){}

    service_ptr mp_service;
    ServiceServicePtr mp_serviceservice;
};



#endif // SERVICENET_H
