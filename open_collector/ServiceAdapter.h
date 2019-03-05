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

namespace open_collector {


class InfoServiceAdapter{
public:
    std::function<void (ProxyBasePtr)> fun;
    std::shared_ptr<boost::asio::io_service> p_service;
};
typedef std::shared_ptr<InfoServiceAdapter> InfoServiceAdapterPtr;

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
        std::lock_guard<std::mutex> lock(m_mutex);
        InfoServiceAdapterPtr p_info;
        auto iter = m_adapters.find(tag);
        if(m_adapters.end() == iter){
            iter = m_adapters.insert(std::make_pair(tag, std::list<InfoServiceAdapterPtr>())).first;
        }
        p_info = InfoServiceAdapterPtr(new InfoServiceAdapterPtr::element_type());
        p_info->p_service = p_service;
        p_info->fun = std::bind(ServiceAdapter::decode_proxy<T...>, std::placeholders::_1, fun);
        iter->second.push_back(p_info);
    }

    void add_listen(const std::string& tag, std::function<void ()> fun, service_ptr p_service){
        std::lock_guard<std::mutex> lock(m_mutex);
        InfoServiceAdapterPtr p_info;
        auto iter = m_adapters.find(tag);
        if(m_adapters.end() == iter){
            iter = m_adapters.insert(std::make_pair(tag, std::list<InfoServiceAdapterPtr>())).first;
        }
        p_info = InfoServiceAdapterPtr(new InfoServiceAdapterPtr::element_type());
        p_info->p_service = p_service;
        p_info->fun = std::bind(ServiceAdapter::decode_proxy_null, std::placeholders::_1, fun);
        iter->second.push_back(p_info);
    }

    template<typename ...T>
    void notify(const std::string& tag, T... args){
        std::list<InfoServiceAdapterPtr> list;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto iter = m_adapters.find(tag);
            if(m_adapters.end() == iter){
                return;
            }
            list = iter->second;
        }
        auto p_info = encode_proxy<T...>(args...);
        for(auto& p : list){
            if(p->p_service){
                p->p_service->post(std::bind(ServiceAdapter::run, p_info, p->fun));
            }else{
                p->fun(p_info);
            }
        }
    }

    void notify(const std::string& tag){
        std::list<InfoServiceAdapterPtr> list;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto iter = m_adapters.find(tag);
            if(m_adapters.end() == iter){
                return;
            }
            list = iter->second;
        }
        auto p_info = encode_proxy();
        for(auto& p : list){
            if(p->p_service){
                p->p_service->post(std::bind(ServiceAdapter::run, p_info, p->fun));
            }else{
                p->fun(p_info);
            }
        }
    }


    static void run(ProxyBasePtr p_proxy, std::function<void (ProxyBasePtr)> fun){
        fun(p_proxy);
    }

    static service_ptr get_service(){
        return service_ptr();
    }

    static void decode_proxy_null(ProxyBasePtr, std::function<void ()> fun){
        fun();
    }

    template<typename T1>
    static void decode_proxy(ProxyBasePtr p_proxy, std::function<void (T1)> fun){
        auto p_param = std::dynamic_pointer_cast<ProxyParam1<T1>>(p_proxy);
        if(!p_param){
            return;
        }
        fun(p_param->param1);
    }

    template<typename T1, typename T2>
    static void decode_proxy(ProxyBasePtr p_proxy, std::function<void (T1, T2)> fun){
        auto p_param = std::dynamic_pointer_cast<ProxyParam2<T1,T2>>(p_proxy);
        if(!p_param){
            return;
        }
        fun(p_param->param1, p_param->param2);
    }

    template<typename T1, typename T2, typename T3>
    static void decode_proxy(ProxyBasePtr p_proxy, std::function<void (T1,T2,T3)> fun){
        auto p_param = std::dynamic_pointer_cast<ProxyParam3<T1,T2,T3>>(p_proxy);
        if(!p_param){
            return;
        }
        fun(p_param->param1, p_param->param2, p_param->param3);
    }


    static ProxyBasePtr encode_proxy(){
        return ProxyBasePtr();
    }

    template<typename T1, typename T2 = void, typename T3 = void>
    class identity {
        typedef T1 type1;
        typedef T2 type2;
        typedef T3 type3;
    };

    template<typename T1>
    ProxyBasePtr encode_proxy(T1 param1){
        return encode_proxy_call(identity<T1>(), param1);
    }

    template<typename T1>
    ProxyBasePtr encode_proxy_call(identity<T1>, T1 param1){
        auto p_info = std::make_shared<ProxyParam1<T1>>();
        p_info->param1 = param1;
        return p_info;
    }


    ProxyBasePtr encode_proxy_call(identity<const char*>, const char* param1){
        auto p_info = std::make_shared<ProxyParam1<std::string>>();
        p_info->param1 = param1;
        return p_info;
    }


    template<typename T1, typename T2>
    static ProxyBasePtr encode_proxy(T1 param1, T2 param2){
        return encode_proxy_call(identity<T1,T2>(), param1, param2);
    }

    template<typename T1, typename T2>
    static ProxyBasePtr encode_proxy_call(identity<T1, T2>, T1 param1, T2 param2){
        auto p_info = std::make_shared<ProxyParam2<T1,T2>>();
        p_info->param1 = param1;
        p_info->param2 = param2;
        return p_info;
    }

    template<typename T2>
    static ProxyBasePtr encode_proxy_call(identity<const char*, T2>, const char* param1, T2 param2){
        return encode_proxy_call(identity<std::string,T2>(), boost::ref(std::string(param1)), param2);
    }

    template<typename T1>
    static ProxyBasePtr encode_proxy_call(identity<T1, const char*>, T1 param1, const char* param2){
        return encode_proxy_call<T1, std::string>(identity<T1,std::string>(), param1, std::string(param2));
    }

    static ProxyBasePtr encode_proxy_call(identity<const char*, const char*>, const char* param1, const char* param2){
        return encode_proxy_call<std::string, std::string>(identity<std::string,std::string>(), std::string(param1), std::string(param2));
    }

    template<typename T1, typename T2, typename T3>
    static ProxyBasePtr encode_proxy(T1 param1, T2 param2, T3 param3){
        auto p_info = std::make_shared<ProxyParam3<T1,T2,T3>>();
        p_info->param1 = param1;
        p_info->param2 = param2;
        p_info->param3 = param3;
        return p_info;
    }

    template<typename T1, typename T2, typename T3>
    static ProxyBasePtr encode_proxy_call(identity<T1, T2, T3>, T1 param1, T2 param2, T3 param3){
        auto p_info = std::make_shared<ProxyParam3<T1,T2,T3>>();
        p_info->param1 = param1;
        p_info->param2 = param2;
        p_info->param3 = param3;
        return p_info;
    }

    template<typename T2, typename T3>
    static ProxyBasePtr encode_proxy_call(identity<const char*, T2, T3>, const char* param1, T2 param2, T3 param3){
        return encode_proxy_call(identity<std::string,T2,T3>(), std::string(param1), param2, param3);
    }

    template<typename T1, typename T3>
    static ProxyBasePtr encode_proxy_call(identity<T1, const char*, T3>, T1 param1, const char* param2, T3 param3){
        return encode_proxy_call(identity<T1,std::string,T3>(), param1, std::string(param2), param3);
    }

    template<typename T1, typename T2>
    static ProxyBasePtr encode_proxy_call(identity<T1, T2, const char*>, T1 param1, T2 param2, const char* param3){
        return encode_proxy_call(identity<T1,T2,std::string>(), param1, param2, std::string(param3));
    }

    static ProxyBasePtr encode_proxy_call(identity<const char*, const char*, const char*>, const char* param1, const char* param2, const char* param3){
        return encode_proxy_call(identity<std::string,std::string,std::string>(), std::string(param1), std::string(param2), std::string(param3));
    }

    std::map<std::string, std::list<InfoServiceAdapterPtr>> m_adapters;
    std::mutex m_mutex;
};





}

#endif // SERVICEADAPTER_H
