#include "update_module/update_subscriber.hpp"
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <filesystem>
#include <sys/stat.h>

namespace fs = std::filesystem;

using namespace eprosima::fastdds::dds;

UpdateSubListener::UpdateSubListener()
    : matched_(0)
{
}

UpdateSubListener::~UpdateSubListener()
{
}

void UpdateSubListener::on_subscription_matched(
    DataReader* reader,
    const SubscriptionMatchedStatus& info)
{
    if (info.current_count_change == 1)
    {
        matched_++;
        std::cout << "[UpdateSubscriber] Matched with publisher. Total matches: " << matched_ << std::endl;
    }
    else if (info.current_count_change == -1)
    {
        matched_--;
        std::cout << "[UpdateSubscriber] Unmatched with publisher. Total matches: " << matched_ << std::endl;
    }
}

void UpdateSubListener::on_data_available(DataReader* reader)
{
    SampleInfo info;
    ota_update_interfaces::msg::dds_::UpdateNotification_ update_msg;  // 네임스페이스 변경

    std::cout << "data available " << matched_ << std::endl;
    if (reader->take_next_sample(&update_msg, &info) == ReturnCode_t::RETCODE_OK)
    {
        if (info.valid_data)
        {
            std::cout << "\n========================================" << std::endl;
            std::cout << "[UpdateSubscriber] Received update notification:" << std::endl;
            std::cout << "  Target: " << update_msg.target() << std::endl;
            std::cout << "  Version: " << update_msg.version() << std::endl;
            std::cout << "  File Path: " << update_msg.file_path() << std::endl;
            std::cout << "  Size: " << update_msg.file_size() << " bytes" << std::endl;
            std::cout << "========================================\n" << std::endl;

            // 별도 스레드에서 파일 다운로드
            std::thread download_thread([this, update_msg]() {
                download_file(update_msg.file_path(), 
                            update_msg.target(), 
                            update_msg.version());
            });
            download_thread.detach();
        }
    }
}

bool UpdateSubListener::parse_url(const std::string& url,
                                   std::string& host,
                                   std::string& port,
                                   std::string& path)
{
    size_t protocol_end = url.find("://");
    if (protocol_end == std::string::npos)
    {
        std::cerr << "[UpdateSubscriber] Invalid URL format" << std::endl;
        return false;
    }

    std::string protocol = url.substr(0, protocol_end);
    size_t host_start = protocol_end + 3;
    
    if (protocol == "https")
        port = "443";
    else if (protocol == "http")
        port = "80";
    else
    {
        std::cerr << "[UpdateSubscriber] Unsupported protocol: " << protocol << std::endl;
        return false;
    }

    size_t path_start = url.find('/', host_start);
    if (path_start == std::string::npos)
    {
        host = url.substr(host_start);
        path = "/";
    }
    else
    {
        host = url.substr(host_start, path_start - host_start);
        path = url.substr(path_start);
    }

    size_t port_start = host.find(':');
    if (port_start != std::string::npos)
    {
        port = host.substr(port_start + 1);
        host = host.substr(0, port_start);
    }

    return true;
}

std::string UpdateSubListener::extract_filename(const std::string& url)
{
    size_t last_slash = url.find_last_of('/');
    if (last_slash != std::string::npos && last_slash < url.length() - 1)
    {
        return url.substr(last_slash + 1);
    }
    return "update_file.bin";
}

void UpdateSubListener::download_file(const std::string& url,
                                       const std::string& target,
                                       const std::string& version)
{
    try
    {
        std::cout << "[UpdateSubscriber] Starting download from: " << url << std::endl;

        std::string download_dir = "./Updates";
        if (!fs::exists(download_dir))
        {
            fs::create_directories(download_dir);
            std::cout << "[UpdateSubscriber] Created directory: " << download_dir << std::endl;
        }

        std::string host, port, path;
        if (!parse_url(url, host, port, path))
        {
            std::cerr << "[UpdateSubscriber] Failed to parse URL" << std::endl;
            return;
        }

        std::cout << "[UpdateSubscriber] Host: " << host << std::endl;
        std::cout << "[UpdateSubscriber] Port: " << port << std::endl;
        std::cout << "[UpdateSubscriber] Path: " << path << std::endl;

        // URL에서 프로토콜 확인
        bool is_https = (url.find("https://") == 0);
        std::cout << "[UpdateSubscriber] Protocol: " << (is_https ? "HTTPS" : "HTTP") << std::endl;

        net::io_context ioc;
        tcp::resolver resolver(ioc);
        auto const results = resolver.resolve(host, port);

        if (is_https)
        {
            // HTTPS 다운로드
            ssl::context ctx(ssl::context::tlsv12_client);
            ctx.set_default_verify_paths();
            ctx.set_verify_mode(ssl::verify_none);

            beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);

            if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str()))
            {
                beast::error_code ec{static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()};
                throw beast::system_error{ec};
            }

            beast::get_lowest_layer(stream).connect(results);
            stream.handshake(ssl::stream_base::client);

            http::request<http::empty_body> req{http::verb::get, path, 11};
            req.set(http::field::host, host);
            req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

            http::write(stream, req);

            beast::flat_buffer buffer;
            http::response<http::string_body> res;
            http::read(stream, buffer, res);

            std::cout << "[UpdateSubscriber] HTTP Status: " << res.result_int() << std::endl;

            if (res.result() != http::status::ok)
            {
                std::cerr << "[UpdateSubscriber] HTTP request failed: " << res.result_int() << std::endl;
                return;
            }

            std::string filename = extract_filename(url);
            std::string filepath = download_dir + "/" + filename;

            std::cout << "[UpdateSubscriber] Saving to: " << filepath << std::endl;

            std::ofstream file(filepath, std::ios::binary);
            if (!file.is_open())
            {
                std::cerr << "[UpdateSubscriber] Failed to open file: " << filepath << std::endl;
                return;
            }

            file.write(res.body().data(), res.body().size());
            file.close();

            std::cout << "[UpdateSubscriber] Download completed: " << filepath << std::endl;
            std::cout << "[UpdateSubscriber] File size: " << res.body().size() << " bytes" << std::endl;

            beast::error_code ec;
            stream.shutdown(ec);
            if (ec == net::error::eof)
            {
                ec = {};
            }
            if (ec)
            {
                std::cerr << "[UpdateSubscriber] Shutdown error: " << ec.message() << std::endl;
            }
        }
        else
        {
            // HTTP 다운로드 (SSL 없음)
            beast::tcp_stream stream(ioc);
            stream.connect(results);

            http::request<http::empty_body> req{http::verb::get, path, 11};
            req.set(http::field::host, host);
            req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

            http::write(stream, req);

            beast::flat_buffer buffer;
            http::response<http::string_body> res;
            http::read(stream, buffer, res);

            std::cout << "[UpdateSubscriber] HTTP Status: " << res.result_int() << std::endl;

            if (res.result() != http::status::ok)
            {
                std::cerr << "[UpdateSubscriber] HTTP request failed: " << res.result_int() << std::endl;
                return;
            }

            std::string filename = extract_filename(url);
            std::string filepath = download_dir + "/" + filename;

            std::cout << "[UpdateSubscriber] Saving to: " << filepath << std::endl;

            std::ofstream file(filepath, std::ios::binary);
            if (!file.is_open())
            {
                std::cerr << "[UpdateSubscriber] Failed to open file: " << filepath << std::endl;
                return;
            }

            file.write(res.body().data(), res.body().size());
            file.close();

            std::cout << "[UpdateSubscriber] Download completed: " << filepath << std::endl;
            std::cout << "[UpdateSubscriber] File size: " << res.body().size() << " bytes" << std::endl;

            beast::error_code ec;
            stream.socket().shutdown(tcp::socket::shutdown_both, ec);
            if (ec && ec != beast::errc::not_connected)
            {
                std::cerr << "[UpdateSubscriber] Shutdown error: " << ec.message() << std::endl;
            }
        }
    }
    catch (std::exception const& e)
    {
        std::cerr << "[UpdateSubscriber] Download error: " << e.what() << std::endl;
    }
}

UpdateSubscriber::UpdateSubscriber()
    : participant_(nullptr)
    , subscriber_(nullptr)
    , topic_(nullptr)
    , reader_(nullptr)
    , running_(false)
{
    setupDDS();
}

UpdateSubscriber::~UpdateSubscriber()
{
    stop();
}

void UpdateSubscriber::setupDDS()
{
    // Create participant with default QoS
    DomainParticipantQos participant_qos = PARTICIPANT_QOS_DEFAULT;
    
    // Set ROS2 compatible QoS
    participant_qos.wire_protocol().builtin.discovery_config.discoveryProtocol = 
        eprosima::fastrtps::rtps::DiscoveryProtocol_t::SIMPLE;
    participant_qos.wire_protocol().builtin.discovery_config.use_SIMPLE_EndpointDiscoveryProtocol = true;
    participant_qos.wire_protocol().builtin.discovery_config.m_simpleEDP.use_PublicationReaderANDSubscriptionWriter = true;
    participant_qos.wire_protocol().builtin.discovery_config.m_simpleEDP.use_PublicationWriterANDSubscriptionReader = true;

    participant_ = DomainParticipantFactory::get_instance()->create_participant(0, participant_qos);
    if (participant_ == nullptr)
    {
        throw std::runtime_error("[UpdateSubscriber] Failed to create DomainParticipant");
    }

    // Register type - 네임스페이스 변경
    type_ = TypeSupport(new ota_update_interfaces::msg::dds_::UpdateNotification_PubSubType());
    type_.register_type(participant_);

    // Create subscriber
    subscriber_ = participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT);
    if (subscriber_ == nullptr)
    {
        throw std::runtime_error("[UpdateSubscriber] Failed to create Subscriber");
    }

    // Create topic
    topic_ = participant_->create_topic(
        "rt/zcu_update_ready",
        type_.get_type_name(),
        TOPIC_QOS_DEFAULT);
    if (topic_ == nullptr)
    {
        throw std::runtime_error("[UpdateSubscriber] Failed to create Topic");
    }

    // Set DataReader QoS
    DataReaderQos reader_qos = DATAREADER_QOS_DEFAULT;
    reader_qos.reliability().kind = RELIABLE_RELIABILITY_QOS;

    // Create DataReader
    reader_ = subscriber_->create_datareader(
        topic_,
        reader_qos,
        &listener_);
    if (reader_ == nullptr)
    {
        throw std::runtime_error("[UpdateSubscriber] Failed to create DataReader");
    }

    std::cout << "[UpdateSubscriber] DDS setup completed successfully" << std::endl;
    std::cout << "[UpdateSubscriber] Waiting for update notifications on topic: rt/zcu_update_ready" << std::endl;
}

bool UpdateSubscriber::start()
{
    if (!running_)
    {
        running_ = true;
        worker_thread_ = std::thread(&UpdateSubscriber::run, this);
        std::cout << "[UpdateSubscriber] Started successfully" << std::endl;
        return true;
    }
    return false;
}

void UpdateSubscriber::stop()
{
    if (running_)
    {
        running_ = false;
        
        if (worker_thread_.joinable())
        {
            worker_thread_.join();
        }

        if (reader_ != nullptr)
        {
            subscriber_->delete_datareader(reader_);
            reader_ = nullptr;
        }
        if (subscriber_ != nullptr)
        {
            participant_->delete_subscriber(subscriber_);
            subscriber_ = nullptr;
        }
        if (topic_ != nullptr)
        {
            participant_->delete_topic(topic_);
            topic_ = nullptr;
        }
        if (participant_ != nullptr)
        {
            DomainParticipantFactory::get_instance()->delete_participant(participant_);
            participant_ = nullptr;
        }

        std::cout << "[UpdateSubscriber] Stopped" << std::endl;
    }
}

void UpdateSubscriber::run()
{
    while (running_)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}