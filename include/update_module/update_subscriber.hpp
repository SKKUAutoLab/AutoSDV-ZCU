#ifndef UPDATE_SUBSCRIBER_HPP
#define UPDATE_SUBSCRIBER_HPP

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/dds/topic/Topic.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

#include "UpdateNotification.h"
#include "UpdateNotificationPubSubTypes.h"

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>

#include <string>
#include <thread>
#include <atomic>
#include <iostream>
#include <fstream>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;
using tcp = boost::asio::ip::tcp;

class UpdateSubListener : public eprosima::fastdds::dds::DataReaderListener
{
public:
    UpdateSubListener();
    ~UpdateSubListener();

    void on_subscription_matched(
        eprosima::fastdds::dds::DataReader* reader,
        const eprosima::fastdds::dds::SubscriptionMatchedStatus& info) override;

    void on_data_available(
        eprosima::fastdds::dds::DataReader* reader) override;

    bool is_matched() const { return matched_ > 0; }

private:
    std::atomic<int> matched_;
    
    void download_file(const std::string& url, 
                      const std::string& target,
                      const std::string& version);
    
    bool parse_url(const std::string& url, 
                   std::string& host, 
                   std::string& port,
                   std::string& path);
    
    std::string extract_filename(const std::string& url);
};

class UpdateSubscriber
{
public:
    UpdateSubscriber();
    ~UpdateSubscriber();

    bool start();
    void stop();

private:
    eprosima::fastdds::dds::DomainParticipant* participant_;
    eprosima::fastdds::dds::Subscriber* subscriber_;
    eprosima::fastdds::dds::Topic* topic_;
    eprosima::fastdds::dds::DataReader* reader_;
    eprosima::fastdds::dds::TypeSupport type_;

    UpdateSubListener listener_;
    std::atomic<bool> running_;
    std::thread worker_thread_;

    void setupDDS();
    void run();
};

#endif // UPDATE_SUBSCRIBER_HPP