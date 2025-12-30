#include "status_module/status_publisher.hpp"
#include "s32g3_skku_can_setting.h"
#include <iostream>

#define CAN_STATUS_ID 0x113

using namespace eprosima::fastdds::dds;

extern int can_socket;

class StatusListener : public DataWriterListener {
    public:
        StatusListener() : matched_(0) {}
    
        void on_publication_matched(DataWriter*, const PublicationMatchedStatus& info) override {
            if (info.current_count_change > 0) {
                matched_++;
                std::cout << "Status: Subscriber matched! Total matches: " << matched_ << std::endl;
            } else {
                matched_--;
                std::cout << "Status: Subscriber unmatched! Total matches: " << matched_ << std::endl;
            }
        }
    
    private:
        int matched_;
    };
    
    StatusPublisher::StatusPublisher() : running_(false) {
        setupDDS();
    }
    
    StatusPublisher::~StatusPublisher() {
        stop();
    }
    
    void StatusPublisher::setupDDS() {
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
        type_ = TypeSupport(new interfaces_pkg::msg::dds_::SteeringStatus_PubSubType());
        type_.register_type(participant_);
    
        // Create publisher
        publisher_ = participant_->create_publisher(PUBLISHER_QOS_DEFAULT);
        if (publisher_ == nullptr) {
            throw std::runtime_error("Create publisher failed");
        }
    
        // Create topic
        topic_ = participant_->create_topic(
            "rt/topic_status_signal",
            type_.get_type_name(),
            TOPIC_QOS_DEFAULT);
        if (topic_ == nullptr) {
            throw std::runtime_error("Create topic failed");
        }
    
        // Set DataWriter QoS
        DataWriterQos writer_qos = DATAWRITER_QOS_DEFAULT;
        writer_qos.reliability().kind = RELIABLE_RELIABILITY_QOS;
    
        // Create DataWriter
        writer_ = publisher_->create_datawriter(
            topic_,
            writer_qos,
            new StatusListener());
        if (writer_ == nullptr) {
            throw std::runtime_error("Create datawriter failed");
        }
    }
    
    void StatusPublisher::receiveLoop() {
        struct can_frame frame;
        interfaces_pkg::msg::dds_::SteeringStatus_ status_data;
    
        while (running_) {
            ssize_t nbytes = read(can_socket, &frame, sizeof(struct can_frame));
            
            if (nbytes < 0) {
                perror("CAN read error");
                continue;
            }
    
            if (frame.can_id == CAN_STATUS_ID) {
                // Process the CAN frame data and fill in the status_data
                // This depends on your specific Status message structure
                // For example:
                status_data.steering((frame.data[0]) & 0xFF);
                // Publish the status data
                writer_->write(&status_data);
            }
        }
    }
    
    bool StatusPublisher::start() {
        if (!running_) {
            running_ = true;
            
            // Setup CAN socket for specific ID filter
            struct can_filter rfilter[1];
            rfilter[0].can_id = CAN_STATUS_ID;
            rfilter[0].can_mask = CAN_SFF_MASK;
            setsockopt(can_socket, SOL_CAN_RAW, CAN_RAW_FILTER, &rfilter, sizeof(rfilter));
            
            receive_thread_ = std::thread(&StatusPublisher::receiveLoop, this);
            std::cout << "Status publisher started. Listening for CAN ID 0x113..." << std::endl;
            return true;
        }
        return false;
    }
    
    void StatusPublisher::sendShutdownMessages() {
        std::cout << "Status publisher shutting down..." << std::endl;
    }
    
    void StatusPublisher::stop() {
        if (running_) {
            running_ = false;
            
            if (receive_thread_.joinable()) {
                receive_thread_.join();
            }
    
            if (writer_ != nullptr) {
                publisher_->delete_datawriter(writer_);
            }
            if (publisher_ != nullptr) {
                participant_->delete_publisher(publisher_);
            }
            if (topic_ != nullptr) {
                participant_->delete_topic(topic_);
            }
            if (participant_ != nullptr) {
                DomainParticipantFactory::get_instance()->delete_participant(participant_);
            }
        }
    }
