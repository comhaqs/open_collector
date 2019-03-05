#ifndef SERVICEADAPTER_H
#define SERVICEADAPTER_H

#include <memory>
#include <functional>
#include <map>
#include <mutex>
#include <list>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/ref.hpp>

class ProxyBase{
public:
    virtual ~ProxyBase(){}
};
typedef std::shared_ptr<ProxyBase> ProxyBasePtr;

template <typename T1>
class ProxyParam1 : public ProxyBase{
public:
    T1 param1;
};

template <typename T1, typename T2>
class ProxyParam2 : public ProxyBase{
public:
    T1 param1;
    T2 param2;
};

template <typename T1, typename T2, typename T3>
class ProxyParam3 : public ProxyBase{
public:
    T1 param1;
    T2 param2;
    T3 param3;
};

template<typename ...T>
class InfoServiceAdapter : public ProxyBase{
public:
    std::function<void (T...)> fun;
    std::shared_ptr<boost::asio::io_service> p_service;
};


class ServiceAdapter
{
public:
    typedef std::shared_ptr<boost::asio::io_service> service_ptr;


    template<typename ...T>
    void add_listen(const std::string& tag, std::function<void (T...)> fun){
        add_listen(tag, fun, get_service());
    }

    template<typename ...T>
    void add_listen(const std::string& tag, std::function<void (T...)> fun, service_ptr p_service){
        typedef std::shared_ptr<InfoServiceAdapter<T...>> info_ptr;
        std::lock_guard<std::mutex> lock(m_mutex);

        auto iter = m_adapters.find(tag);
        if(m_adapters.end() == iter){
            iter = m_adapters.insert(std::make_pair(tag, std::make_shared<std::list<ProxyBasePtr>>())).first;
        }
        info_ptr p_info = std::make_shared<InfoServiceAdapter<T...>>();
        p_info->p_service = p_service;
        p_info->fun = fun;
        iter->second->emplace_back(p_info);
    }

    template<typename ...T>
    void notify(const std::string& tag, T... args){
        std::list<ProxyBasePtr> list;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto iter = m_adapters.find(tag);
            if(m_adapters.end() == iter){
                return;
            }
            list = *(iter->second);
        }
        for(auto& p : list){
            auto p_info = std::dynamic_pointer_cast<InfoServiceAdapter<T...>>(p);
            if(!p_info){
                return;
            }
            if(p_info->p_service){
                p_info->p_service->post(std::bind(ServiceAdapter::run<T...>, p_info->fun, args...));
            }else{
                p_info->fun(args...);
            }
        }
    }

    template<typename ...T>
    static void run(std::function<void (T...)> fun, T...args){
        fun(args...);
    }

    static service_ptr get_service(){
        return service_ptr();
    }

    std::map<std::string, std::shared_ptr<std::list<ProxyBasePtr>>> m_adapters;
    std::mutex m_mutex;
};


#endif // SERVICEADAPTER_H
