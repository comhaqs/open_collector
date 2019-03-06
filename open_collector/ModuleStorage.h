#ifndef MODULESTORAGE_H
#define MODULESTORAGE_H


#include <memory>
#include <functional>
#include <list>
#include <mutex>
#include <condition_variable>
#include <boost/thread.hpp>
#include <boost/format.hpp>

template<typename TData, typename TConnect>
class ModuleStorage
{
public:
    using data_ptr = std::shared_ptr<TData>;
    using cnt_ptr = std::shared_ptr<TConnect>;

    ModuleStorage(std::function<void (data_ptr, cnt_ptr)> fun, std::function<cnt_ptr ()> fun_connect, unsigned int thread_count = 1)
        :m_fun(fun), m_fun_get_connect(fun_connect){
        if(0 >= thread_count){
            m_thread_count = 1;
        }else if(30 < thread_count){
            m_thread_count = 30;
        }

        start();
    }

    virtual void stop(){
        std::list<boost::thread> threads;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            threads.swap(m_threads);
        }
        for(auto& t : threads){
            t.interrupt();
        }
    }

    virtual void push_data(data_ptr p_data){
        {
            std::lock_guard<std::mutex> lock(m_mutex_condition);
            m_data.push_back(p_data);
        }
        m_condition.notify_one();
    }
protected:
    virtual bool start(){
        std::lock_guard<std::mutex> lock(m_mutex);
        for(unsigned int i = 0; i < m_thread_count; ++i){
            m_threads.emplace_back(boost::thread(std::bind(&ModuleStorage<TData,TConnect>::handle_thread, this->shared_from_this())));
        }
        return true;
    }

    virtual void log_error(const std::string& ){}
    virtual void get_connect(){return m_fun_get_connect();}
    virtual void handle_thread(){
        while(true){
            try{
                cnt_ptr p_connect = get_connect();
                std::list<data_ptr> list;
                while (p_connect) {
                    {
                        std::lock_guard<std::mutex> lock(m_mutex_condition);
                        while(m_data.empty()){
                            m_condition.wait(lock);
                        }
                        list = m_data;
                        m_data.clear();
                    }
                    for(auto& p : list){
                        m_fun(p, p_connect);
                    }
                    list.clear();
                }
            }catch(boost::thread_interrupted&){
                break;
            }catch(std::exception& e){
                log_error((boost::format("thread error: %s") % e.what()).str());
            }
            boost::this_thread::sleep(boost::posix_time::seconds(60));
        }
    }

    std::list<data_ptr> m_data;
    std::condition_variable_any m_condition;
    std::mutex m_mutex_condition;
    std::function<void (data_ptr, cnt_ptr)> m_fun;
    std::list<boost::thread> m_threads;
    unsigned int m_thread_count = 1;
    std::mutex m_mutex;
    std::function<cnt_ptr ()> m_fun_get_connect;
};

#endif // MODULESTORAGE_H
