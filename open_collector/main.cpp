#include <iostream>
#include <boost/thread.hpp>
#include "ServiceAdapter.h"
#include "ServiceNet.h"


using namespace std;




int main()
{
    ServiceNet net;

    ServiceAdapter service;

    std::function<void ()> fun0 = [](){
        std::cout<<boost::this_thread::get_id()<<" -> param null"<<std::endl;
    };
    service.add_listen("0", fun0);

    auto p_service1 = std::shared_ptr<boost::asio::io_service>(new boost::asio::io_service());
    std::function<void (int)> fun1 = [](int d){
        std::cout<<boost::this_thread::get_id()<<" -> param "<<d<<std::endl;
    };
    service.add_listen("1", fun1, p_service1);
    boost::thread thread1([p_service1](){
        boost::asio::io_service::work work(*p_service1);
        p_service1->run();
    });

    auto p_service2 = std::shared_ptr<boost::asio::io_service>(new boost::asio::io_service());
    std::function<void (int, std::string)> fun2 = [](int d, std::string c){
        std::cout<<boost::this_thread::get_id()<<" -> param "<<d<<","<<c<<std::endl;
    };
    service.add_listen("2", fun2, p_service2);
    boost::thread thread2([p_service2](){
        boost::asio::io_service::work work(*p_service2);
        p_service2->run();
    });

    std::function<void (int, int, std::string)> fun31 = [](int p1, int p2, std::string p3){
        std::cout<<boost::this_thread::get_id()<<" -> param "<<p1<<","<<p2<<","<<p3<<std::endl;
    };
    service.add_listen("31", fun31, p_service2);
    std::function<void (int, std::string, std::string)> fun32 = [](int p1, std::string p2, std::string p3){
        std::cout<<boost::this_thread::get_id()<<" -> param "<<p1<<","<<p2<<","<<p3<<std::endl;
    };
    service.add_listen("32", fun32, p_service2);
    std::function<void (std::string, std::string, std::string)> fun33 = [](std::string p1, std::string p2, std::string p3){
        std::cout<<boost::this_thread::get_id()<<" -> param "<<p1<<","<<p2<<","<<p3<<std::endl;
    };
    service.add_listen("33", fun33, p_service2);

    cout <<boost::this_thread::get_id()<< " -> Hello World!" << endl;
    service.notify("0");
    service.notify("1", 10);
    service.notify("1", 11);
    service.notify("2", 20, "200");
    service.notify("2", "abc", "210");

    service.notify("31", 1, 2,"210");
    service.notify("32", 1, "32", "210");
    service.notify("33", "33", std::string("33"), "210");

    boost::this_thread::sleep(boost::posix_time::seconds(2));
    p_service1->stop();
    p_service2->stop();
    thread1.join();
    thread2.join();
    return 0;
}
