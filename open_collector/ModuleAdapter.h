#ifndef MODULEADAPTER_H
#define MODULEADAPTER_H

#include <memory>
#include <functional>
#include <map>
#include <mutex>
#include <list>
#include <boost/asio.hpp>
#include <boost/format.hpp>
#include "ModuleLibrary.h"

namespace open_collector{

class ModuleAdapter
{
public:
    static ModuleAdapter& get_instance(){
        static ModuleAdapter s_instance;
        return s_instance;
    }

    template<typename ...T>
    void add_listen(const std::string& tag, std::function<void (T...)> fun){
        add_listen(tag, fun, service_ptr());
    }

    template<typename ...T>
    void add_listen(const std::string& tag, std::function<void (T...)> fun, service_ptr p_service){
        typedef std::shared_ptr<InfoModuleAdapter<T...>> info_ptr;
        std::lock_guard<std::mutex> lock(m_mutex);

        auto iter = m_adapters.find(tag);
        if(m_adapters.end() == iter){
            iter = m_adapters.insert(std::make_pair(tag, std::make_shared<std::list<ProxyBasePtr>>())).first;
        }
        info_ptr p_info = std::make_shared<InfoModuleAdapter<T...>>();
        p_info->p_service = p_service;
        p_info->fun = fun;
        iter->second->emplace_back(p_info);
    }

    template<typename ...T>
    void notify(const std::string& tag, T... args){
        return notify_call(std::move(tag), translate_type(std::move(args))...);
    }

protected:
    template<typename ...T>
    class InfoModuleAdapter : public ProxyBase{
    public:
        std::function<void (T...)> fun;
        std::shared_ptr<boost::asio::io_service> p_service;
    };

    template<typename T>
    T&& translate_type(T&& t){
        return std::move(t);
    }

    std::string&& translate_type(const char*&& t){
        return std::move(std::string(t));
    }

    template<typename ...T>
    void notify_call(const std::string& tag, T... args){
        std::list<ProxyBasePtr> list;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto iter = m_adapters.find(tag);
            if(m_adapters.end() == iter){
                log_error((boost::format("can not find tag:%s") % tag).str());
                return;
            }
            list = *(iter->second);
        }
        for(auto& p : list){
            auto p_info = std::dynamic_pointer_cast<InfoModuleAdapter<T...>>(p);
            if(!p_info){
                log_error((boost::format("dynamic_pointer_cast failed, type:%s") % typeid(InfoModuleAdapter<T...>)).str());
                return;
            }
            if(p_info->p_service){
                p_info->p_service->post(std::bind(ModuleAdapter::run<T...>, p_info->fun, std::move(args)...));
            }else{
                p_info->fun(std::move(args)...);
            }
        }
    }

    template<typename ...T>
    static void run(std::function<void (T...)> fun, T...args){
        fun(args...);
    }

    virtual void log_error(const std::string& msg){
        s_log_error(msg);
    }

    std::map<std::string, std::shared_ptr<std::list<ProxyBasePtr>>> m_adapters;
    std::mutex m_mutex;
};

}

#endif // MODULEADAPTER_H
