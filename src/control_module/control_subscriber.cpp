#include "control_module/control_subscriber.hpp"
#include "s32g3_skku_can_setting.h"
#include <iostream>

using namespace eprosima::fastdds::dds;

int32_t steering_data, leftspeed_data, rightspeed_data;
unsigned char can_msg_steering[4];
unsigned char can_msg_speed[8];

class ControlListener : public DataReaderListener {
public:
    ControlListener() : matched_(0) {
        std::cout << "Control: ControlListener created" << std::endl;
    }

    void on_subscription_matched(DataReader*, const SubscriptionMatchedStatus& info) override {
        if (info.current_count_change > 0) {
            matched_++;
            std::cout << "Control: Publisher matched! Total matches: " << matched_ << std::endl;
        } else {
            matched_--;
            std::cout << "Control: Publisher unmatched! Total matches: " << matched_ << std::endl;
        }
    }

    void on_data_available(DataReader* reader) override {
        interfaces_pkg::msg::dds_::MotionCommand_ sample;
        SampleInfo info;
        
        while (reader->take_next_sample(&sample, &info) == ReturnCode_t::RETCODE_OK) {
            if (info.valid_data) {
                // Steering CAN message
                steering_data = sample.steering();
                can_msg_steering[0] = 0;
                can_msg_steering[1] = 0;
                can_msg_steering[2] = 0;
                can_msg_steering[3] = (steering_data + 7) & 0xFF;

                // Speed CAN message
                leftspeed_data = sample.left_speed();
                rightspeed_data = sample.right_speed();
                // Left speed data (upper 4 bytes)
                can_msg_speed[0] = (leftspeed_data < 0) ? 1 : 0;
                can_msg_speed[1] = 0;
                can_msg_speed[2] = 0;
                can_msg_speed[3] = (leftspeed_data < 0) ? -leftspeed_data : leftspeed_data;
                // Right speed data (lower 4 bytes)
                can_msg_speed[4] = (rightspeed_data < 0) ? 1 : 0;
                can_msg_speed[5] = 0;
                can_msg_speed[6] = 0;
                can_msg_speed[7] = (rightspeed_data < 0) ? -rightspeed_data : rightspeed_data;

                // Send CAN messages
                send_can_msg(0x111, can_msg_steering, 4);
                send_can_msg(0x123, can_msg_speed, 8);
            }
        }
    }

private:
    int matched_;
};

ControlSubscriber::ControlSubscriber() : running_(false) {
    can_setting();
    setupDDS();
}

ControlSubscriber::~ControlSubscriber() {
    stop();
}

void ControlSubscriber::setupDDS() {
    // Create participant with default QoS
    DomainParticipantQos participant_qos = PARTICIPANT_QOS_DEFAULT;
    
    // Set ROS2 compatible QoS
    participant_qos.wire_protocol().builtin.discovery_config.discoveryProtocol = 
        eprosima::fastrtps::rtps::DiscoveryProtocol_t::SIMPLE;
    participant_qos.wire_protocol().builtin.discovery_config.use_SIMPLE_EndpointDiscoveryProtocol = true;
    participant_qos.wire_protocol().builtin.discovery_config.m_simpleEDP.use_PublicationReaderANDSubscriptionWriter = true;
    participant_qos.wire_protocol().builtin.discovery_config.m_simpleEDP.use_PublicationWriterANDSubscriptionReader = true;

    participant_ = DomainParticipantFactory::get_instance()->create_participant(0, participant_qos);
    if (participant_ == nullptr) {
        throw std::runtime_error("Create participant failed");
    }

    // Register type
    type_ = TypeSupport(new interfaces_pkg::msg::dds_::MotionCommand_PubSubType());
    type_.register_type(participant_);

    // Create subscriber
    subscriber_ = participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT);
    if (subscriber_ == nullptr) {
        throw std::runtime_error("Create subscriber failed");
    }

    // Create topic
    topic_ = participant_->create_topic(
        "rt/topic_control_signal",
        type_.get_type_name(),
        TOPIC_QOS_DEFAULT);
    if (topic_ == nullptr) {
        throw std::runtime_error("Create topic failed");
    }

    // Set DataReader QoS
    DataReaderQos reader_qos = DATAREADER_QOS_DEFAULT;
    reader_qos.reliability().kind = RELIABLE_RELIABILITY_QOS;

    // Create DataReader
    listener_ = new ControlListener();
    reader_ = subscriber_->create_datareader(
        topic_,
        reader_qos,
        listener_);
    if (reader_ == nullptr) {
        throw std::runtime_error("Create datareader failed");
    }
}

bool ControlSubscriber::start() {
    if (!running_) {
        running_ = true;
        std::cout << "Control subscriber started on domain 0." << std::endl;
        return true;
    }
    return false;
}

void ControlSubscriber::sendShutdownMessages() {
    // Speed message (0x123) - all values to 0
    memset(can_msg_speed, 0, 8);
    send_can_msg(0x123, can_msg_speed, 8);

    // Steering message (0x111) - [0,0,0,7]
    can_msg_steering[0] = 0;
    can_msg_steering[1] = 0;
    can_msg_steering[2] = 0;
    can_msg_steering[3] = 7;
    send_can_msg(0x111, can_msg_steering, 4);
    
    std::cout << "Shutdown messages sent" << std::endl;
}

void ControlSubscriber::stop() {
    if (running_) {
        running_ = false;

        if (reader_ != nullptr) {
            subscriber_->delete_datareader(reader_);
        }
        if (subscriber_ != nullptr) {
            participant_->delete_subscriber(subscriber_);
        }
        if (topic_ != nullptr) {
            participant_->delete_topic(topic_);
        }
        if (participant_ != nullptr) {
            DomainParticipantFactory::get_instance()->delete_participant(participant_);
        }

        delete listener_;
    }
}