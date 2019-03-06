#ifndef MODULESERVICE_H
#define MODULESERVICE_H

#include "ModuleLibrary.h"
#include <boost/thread.hpp>
#include <boost/format.hpp>
#include <vector>
#include <mutex>
#include <atomic>


class ModuleService{
public:
    static ModuleService& get_instance(){
        static ModuleService s_instance;
        return s_instance;
    }

    virtual ~ModuleService(){
        stop();
    }

    virtual void stop(){
        m_bstart = false;
        std::vector<service_ptr> services;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            services.insert(services.end(), m_services.begin(), m_services.end());
            m_services.clear();
            services.insert(services.end(), m_services_singleton.begin(), m_services_singleton.end());
            m_services_singleton.clear();
        }
        for(auto& p : services){
            p->stop();
        }
    }

    virtual service_ptr get_service(){
        if(!m_bstart){
            return service_ptr();
        }
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if(m_services.empty()){
                auto count = boost::thread::hardware_concurrency();
                for(unsigned int i = 0; i < count; ++i){
                    auto p_service = std::make_shared<service_ptr::element_type>();
                    m_services.emplace_back(p_service);
                    m_threads.emplace_back(boost::thread(std::bind(ModuleService::handle_thread, p_service)));
                }
            }
            return m_services[static_cast<std::size_t>(rand()) % m_services.size()];
        }
    }
    virtual service_ptr get_service_singleton(){
        if(!m_bstart){
            return service_ptr();
        }
        auto p_service = std::make_shared<service_ptr::element_type>();
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_services_singleton.emplace_back(p_service);
            m_threads.push_back(boost::thread(std::bind(ModuleService::handle_thread, p_service)));
        }
        return p_service;
    }

protected:
    static void log_error(const std::string&){}

    static void handle_thread(service_ptr p_service){
        while (true) {
            try{
                boost::asio::io_service::work work(*p_service);
                p_service->run();
            }catch(boost::thread_interrupted&){
                break;
            }catch(std::exception& e){
                log_error((boost::format("service error:%s") % e.what()).str());
            }
            boost::this_thread::sleep(boost::posix_time::seconds(60));
        }
    }

    std::vector<boost::thread> m_threads;
    std::vector<service_ptr> m_services;
    std::vector<service_ptr> m_services_singleton;
    std::mutex m_mutex;
    std::atomic_bool m_bstart{true};
};




#endif // MODULESERVICE_H
