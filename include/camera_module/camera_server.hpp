#pragma once

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/DataWriterListener.hpp>
#include <fastdds/dds/core/policy/QosPolicies.hpp>
#include "Image.h"
#include "ImagePubSubTypes.h"

#include <thread>
#include <atomic>
#include <vector>
#include <sys/socket.h>
#include <arpa/inet.h>

class CameraServer {
public:
    CameraServer(int port = 8485);
    ~CameraServer();

    bool start();
    void stop();

private:
    void setupUDP(int port);
    void setupDDS();
    void receiveLoop();

    // UDP related members
    int sock;
    struct sockaddr_in addr;

    // FastDDS related members
    eprosima::fastdds::dds::DomainParticipant* participant_;
    eprosima::fastdds::dds::Publisher* publisher_;
    eprosima::fastdds::dds::Topic* topic_;
    eprosima::fastdds::dds::DataWriter* writer_;
    eprosima::fastdds::dds::TypeSupport type_;
    sensor_msgs::msg::dds_::Image_ image_data_;

    std::atomic<bool> running_;
    std::thread receive_thread_;
};