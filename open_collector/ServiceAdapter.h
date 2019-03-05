#ifndef SERVICEADAPTER_H
#define SERVICEADAPTER_H

#include <memory>
#include <functional>
#include <map>
#include <mutex>
#include <list>
#include <boost/asio.hpp>


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
        return notify_call(std::move(tag), translate_type(std::move(args))...);
    }

protected:
    class ProxyBase{
    public:
        virtual ~ProxyBase(){}
    };
    typedef std::shared_ptr<ProxyBase> ProxyBasePtr;


    template<typename ...T>
    class InfoServiceAdapter : public ProxyBase{
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
                p_info->p_service->post(std::bind(ServiceAdapter::run<T...>, p_info->fun, std::move(args)...));
            }else{
                p_info->fun(std::move(args)...);
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
