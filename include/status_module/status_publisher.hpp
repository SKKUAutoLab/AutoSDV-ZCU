#pragma once

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/DataWriterListener.hpp>
#include <fastdds/dds/core/policy/QosPolicies.hpp>
#include "Status.h"
#include "StatusPubSubTypes.h"

#include <thread>
#include <atomic>

class StatusPublisher {
public:
    StatusPublisher();
    ~StatusPublisher();

    bool start();
    void stop();
    void sendShutdownMessages();

private:
    void setupDDS();
    void receiveLoop();

    // FastDDS related members
    eprosima::fastdds::dds::DomainParticipant* participant_;
    eprosima::fastdds::dds::Publisher* publisher_;
    eprosima::fastdds::dds::Topic* topic_;
    eprosima::fastdds::dds::DataWriter* writer_;
    eprosima::fastdds::dds::TypeSupport type_;

    std::atomic<bool> running_;
    std::thread receive_thread_;
};