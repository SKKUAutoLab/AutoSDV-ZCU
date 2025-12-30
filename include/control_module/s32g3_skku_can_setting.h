#ifndef S32G3_SKKU_CAN_SETTING_H_
#define S32G3_SKKU_CAN_SETTING_H_

// C++에서 include할 때는 extern "C"로 감싸기
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <netpacket/packet.h>
#include <netinet/ether.h>

void setup_can_interface();
void set_can_bitrate(const char *interface, int bitrate);
void send_can_msg(uint32_t can_id, const unsigned char* data, uint8_t dlc);
void can_setting();

#ifdef __cplusplus
}
#endif

#endif