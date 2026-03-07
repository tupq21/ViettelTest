#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <sys/time.h>
#include <sys/epoll.h>
#include <errno.h>
#include <pthread.h>
#include "userEquipment.h"

 
atomic_int  UE_sfn = 0;
atomic_bool is_synced = false;
int8_t      mib_received_count = 0;

void* tick_thread(void* arg) {
    while (1) {
        usleep(10 * 1000); // 10ms theo tài liệu
        
        // Luôn tăng SFN nội bộ để giữ nhịp, 
        // nhưng chỉ bắt đầu chuẩn hóa khi đã nhận MIB đầu tiên
        int current = atomic_load(&UE_sfn);
        current = (current + 1) % 1024;
        atomic_store(&UE_sfn, current);
        
        if (current % 100 == 0 && atomic_load(&is_synced)) {
            printf("[UE] Tick - SFN: %d\n", current);
        }
    }
    return NULL;
}

int32_t initUserAddress(struct sockaddr_in* ue_addr) {
    int32_t sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }

    memset(ue_addr, 0, sizeof(*ue_addr));
    ue_addr->sin_family = AF_INET;
    ue_addr->sin_port = htons(GNODEB_UDP_PORT);
    ue_addr->sin_addr.s_addr = inet_addr(GNODEB_BROADCAST_ADDRESS);

    bind(sockfd, (struct sockaddr*)ue_addr, sizeof(*ue_addr));
    return sockfd;
}