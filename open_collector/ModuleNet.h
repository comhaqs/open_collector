#ifndef MODULENET_H
#define MODULENET_H

#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/format.hpp>
#include <vector>
#include "ModuleLibrary.h"
#include "ModuleService.h"

namespace open_collector{

template<typename Tinfo>
class ModuleNet : public std::enable_shared_from_this<ModuleNet<Tinfo>>
{
public:
    ModuleNet(unsigned int port, std::function<void (frame_ptr, socket_ptr, std::shared_ptr<Tinfo>)> fun): m_fun(fun), m_port(port){
    }

    virtual ~ModuleNet(){}

    virtual bool start(){
        service_ptr p_service = ModuleService::get_instance().get_service_singleton();
        boost::asio::spawn(*p_service, std::bind(&ModuleNet::handle_net, this->shared_from_this(), std::placeholders::_1, p_service, m_port));
        return true;
    }
protected:


    virtual void handle_net(boost::asio::yield_context yield, service_ptr p_service, unsigned int port){
        try{
            boost::asio::ip::tcp::acceptor acceptor(*p_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), static_cast<unsigned short>(port)));
            while(true) {
                boost::system::error_code ec;
                auto p_socket = socket_ptr(new socket_ptr::element_type(*p_service));
                acceptor.async_accept(*p_socket, yield[ec]);
                if(ec){
                   break;
                }
                boost::asio::spawn(*p_service, std::bind(&ModuleNet::handle_read, this->shared_from_this()
                                                               , std::placeholders::_1, p_socket));
            }
        }catch(std::exception& e){
            log_error((boost::format("accept failed:%s") % e.what()).str());
        }
    }
    virtual void handle_read(boost::asio::yield_context yield, socket_ptr p_socket){
        frame_ptr p_frame(new frame_ptr::element_type(m_buffer_max));
        frame_ptr p_frame_new;
        std::shared_ptr<Tinfo> p_info = std::make_shared<Tinfo>();
        while(true){
            boost::system::error_code ec;
            std::size_t n = p_socket->async_read_some(boost::asio::buffer(p_frame->data(), m_buffer_max), yield[ec]);
            if(ec){
                log_error((boost::format("read data error:%s") % ec.message()).str());
                break;
            }else if(0 >= n){
                log_error((boost::format("read data count error:%d") % n).str());
                break;
            }
            p_frame_new = std::make_shared<frame_ptr::element_type>(p_frame->begin(), p_frame->begin() + static_cast<long long>(n));
            read_frame(p_frame_new, p_socket, p_info);
        }
    }

    virtual void log_error(const std::string& msg){
        s_log_error(msg);
    }

    virtual void read_frame(frame_ptr p_frame, socket_ptr p_socket, std::shared_ptr<Tinfo> p_info){
        m_fun(p_frame, p_socket, p_info);
    }

    unsigned int m_buffer_max = 256;
    std::function<void (frame_ptr, socket_ptr, std::shared_ptr<Tinfo>)> m_fun;
    unsigned int m_port = 0;
};

}

#endif // MODULENET_H
