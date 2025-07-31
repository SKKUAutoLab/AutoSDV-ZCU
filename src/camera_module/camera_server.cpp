#include "camera_module/camera_server.hpp"
#include <iostream>
#include <unistd.h>

using namespace eprosima::fastdds::dds;

class ImageListener : public DataWriterListener {
public:
    ImageListener() : matched_(0) {}

    void on_publication_matched(DataWriter*, const PublicationMatchedStatus& info) override {
        if (info.current_count_change > 0) {
            matched_++;
            std::cout << "Camera: Subscriber matched! Total matches: " << matched_ << std::endl;
        } else {
            matched_--;
            std::cout << "Camera: Subscriber unmatched! Total matches: " << matched_ << std::endl;
        }
    }

private:
    int matched_;
};

CameraServer::CameraServer(int port) : running_(false) {
    setupUDP(port);
    setupDDS();
}

CameraServer::~CameraServer() {
    stop();
}

void CameraServer::setupUDP(int port) {
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        throw std::runtime_error("Socket creation failed");
    }

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        throw std::runtime_error("Bind failed");
    }
}

void CameraServer::setupDDS() {
    participant_ = DomainParticipantFactory::get_instance()->create_participant(0, PARTICIPANT_QOS_DEFAULT);
    if (participant_ == nullptr) {
        throw std::runtime_error("Create participant failed");
    }

    type_ = TypeSupport(new sensor_msgs::msg::dds_::Image_PubSubType());
    type_.register_type(participant_);

    publisher_ = participant_->create_publisher(PUBLISHER_QOS_DEFAULT);
    if (publisher_ == nullptr) {
        throw std::runtime_error("Create publisher failed");
    }

    topic_ = participant_->create_topic(
        "rt/image_02_raw",
        type_.get_type_name(),
        TOPIC_QOS_DEFAULT);
    if (topic_ == nullptr) {
        throw std::runtime_error("Create topic failed");
    }

    writer_ = publisher_->create_datawriter(
        topic_,
        DATAWRITER_QOS_DEFAULT,
        new ImageListener());
    if (writer_ == nullptr) {
        throw std::runtime_error("Create datawriter failed");
    }
}

void CameraServer::receiveLoop() {
    std::vector<uint8_t> buffer(65507);
    uint32_t data_size;

    while (running_) {
        ssize_t size_received = recv(sock, &data_size, sizeof(data_size), 0);
        if (size_received != sizeof(data_size) || !running_) break;

        data_size = ntohl(data_size);
        std::vector<uint8_t> jpeg_data;
        jpeg_data.resize(data_size);

        ssize_t received = recv(sock, jpeg_data.data(), data_size, MSG_WAITALL);

        if (received == static_cast<ssize_t>(data_size) && running_) {
            std::vector<uint8_t>& data_ref = jpeg_data;
            image_data_.data(data_ref);
            image_data_.width(640);
            image_data_.height(480);
            image_data_.encoding("jpeg");
            
            writer_->write(&image_data_);
        }
    }
}

bool CameraServer::start() {
    if (!running_) {
        running_ = true;
        receive_thread_ = std::thread(&CameraServer::receiveLoop, this);
        std::cout << "Camera server started. Listening on port 8485..." << std::endl;
        return true;
    }
    return false;
}

void CameraServer::stop() {
    if (running_) {
        running_ = false;
        if (receive_thread_.joinable()) {
            receive_thread_.join();
        }
        close(sock);

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
