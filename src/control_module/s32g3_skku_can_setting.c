#include "s32g3_skku_can_setting.h"
#define CAN_INTERFACE "can0"

int can_socket, eth_socket;

void set_can_bitrate(const char *interface, int bitrate) {
    char command[100];
    snprintf(command, sizeof(command), "sudo ip link set %s type can bitrate %d", interface, bitrate);
    
    system("sudo ip link set can0 down");
    int result = system(command);
    if (result != 0) {
        fprintf(stderr, "CAN 비트레이트 설정 실패\n");
        exit(1);
    }
    
    snprintf(command, sizeof(command), "sudo ip link set %s up", interface);
    result = system(command);
    if (result != 0) {
        fprintf(stderr, "CAN 인터페이스 활성화 실패\n");
        exit(1);
    }

    printf("CAN 인터페이스 %s의 비트레이트를 %d bps로 설정하고 활성화했습니다.\n", interface, bitrate);
}

void setup_can_interface() {
    struct sockaddr_can addr;
    struct ifreq ifr;
    // CAN 비트레이트 설정 (500 kbps)
    set_can_bitrate(CAN_INTERFACE, 500000);

    can_socket = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (can_socket < 0) {
        perror("CAN 소켓 생성 실패");
        exit(1);
    }

    strcpy(ifr.ifr_name, CAN_INTERFACE);
    ioctl(can_socket, SIOCGIFINDEX, &ifr);

    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(can_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("CAN 소켓 바인드 실패");
        exit(1);
    }

    printf("CAN 인터페이스 %s 설정 완료\n", CAN_INTERFACE);
}

void send_can_msg(uint32_t can_id, const unsigned char* data, uint8_t dlc)
{
    struct can_frame frame;
    int nbytes;

    // CAN 프레임 초기화
    memset(&frame, 0, sizeof(struct can_frame));

    // CAN ID 설정
    frame.can_id = can_id;

    // 데이터 길이 설정 (최대 8바이트)
    frame.can_dlc = dlc > 8 ? 8 : dlc;

    // 데이터 복사
    memcpy(frame.data, data, frame.can_dlc);

    // CAN 메시지 전송
    nbytes = write(can_socket, &frame, sizeof(struct can_frame));
    if (nbytes != sizeof(struct can_frame))
    {
        perror("CAN 메시지 전송 실패");
        return;
    }

    //printf("CAN 메시지 전송 성공: ID= 0x%X 데이터 길이 = %d\n", frame.can_id, frame.can_dlc);
}

void can_setting()
{
   setup_can_interface();
}
