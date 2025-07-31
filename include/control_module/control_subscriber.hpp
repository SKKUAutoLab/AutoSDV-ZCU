#pragma once

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/dds/core/policy/QosPolicies.hpp>
#include "ControlCommand.h"
#include "ControlCommandPubSubTypes.h"

#include <thread>
#include <atomic>

class ControlSubscriber {
public:
    ControlSubscriber();
    ~ControlSubscriber();

    bool start();
    void stop();
    void sendShutdownMessages();

private:
    void setupDDS();

    // FastDDS related members
    eprosima::fastdds::dds::DomainParticipant* participant_;
    eprosima::fastdds::dds::Subscriber* subscriber_;
    eprosima::fastdds::dds::Topic* topic_;
    eprosima::fastdds::dds::DataReader* reader_;
    eprosima::fastdds::dds::TypeSupport type_;
    eprosima::fastdds::dds::DataReaderListener* listener_;

    std::atomic<bool> running_;
};