#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <sys/time.h>
#include "common.h"

 
atomic_int  UE_sfn = 0;
atomic_bool is_synced = false;
int8_t      mib_received_count = 0;

void* message_reception_thread(void* arg);

void* message_reception_thread(void* arg) {
    int sockfd = *(int*)arg;
    free(arg);
    struct sockaddr_in gnb_addr;
    socklen_t addr_len = sizeof(gnb_addr);

    printf("[UE] Message reception thread started, listening on port %d\n", GNODEB_UDP_PORT);

    while (1) {
        // Nhận message (có thể là MIB hoặc paging)
        char buffer[1024];
        int ret = recvfrom(sockfd, buffer, sizeof(buffer), 0,
                           (struct sockaddr*)&gnb_addr, &addr_len);
        if (ret < 0) {
            perror("recvfrom failed");
            continue;
        }

        if (ret == sizeof(MIB_t)) {
            // Xử lý MIB message
            MIB_t* mib = (MIB_t*)buffer;
            uint16_t received_sfn = ntohs(mib->sfnValue);
            
            if (false == atomic_load(&is_synced)) {
                atomic_store(&UE_sfn, received_sfn);
                atomic_store(&is_synced, true);
                printf("[UE] Đồng bộ lần đầu: SFN = %d\n", received_sfn);
            } else {
                mib_received_count++;
                if (mib_received_count >= MIB_SYNC_CYCLE) {
                    int16_t oldValue = atomic_load(&UE_sfn);
                    atomic_store(&UE_sfn, received_sfn);
                    mib_received_count = 0;
                    if (oldValue != received_sfn) {
                        printf("[UE] Hiệu chỉnh SFN từ %d -> %d\n", oldValue, received_sfn);
                    }
                }
            }
        } else if (ret == sizeof(Paging_t)) {
            // Xử lý paging message
            Paging_t* paging_msg = (Paging_t*)buffer;
            if (paging_msg->messageType == NGAP_PAGING_MESSAGE_TYPE) {
                int32_t ue_id = UE_ID_DEFAULT;
                int32_t current_sfn = atomic_load(&UE_sfn);
                
                // Kiểm tra xem có phải paging occasion không
                int32_t left_side = (current_sfn + PAGING_FRAME_OFFSET) % PAGING_CYCLE;
                int32_t right_side = (PAGING_CYCLE / PF_PER_CYCLE) * (ue_id % PF_PER_CYCLE);
                
                if (left_side == right_side) {
                    if (paging_msg->ueId == ue_id) {
                        printf("[UE] Nhận RRC Paging: UE ID=%u, TAC=%u, CN Domain=%u tại SFN=%d\n",
                               paging_msg->ueId, paging_msg->TAC, paging_msg->cn_domain, current_sfn);
                        // Xử lý paging: có thể thức dậy hoàn toàn, kết nối lại, etc.
                    } else {
                        printf("[UE] Nhận paging cho UE khác: ID=%u\n", paging_msg->ueId);
                    }
                } else {
                    printf("[UE] Nhận paging ngoài paging occasion: SFN=%d, bỏ qua\n", current_sfn);
                }
            }
        } else {
            printf("[UE] Nhận message không hợp lệ, size=%d\n", ret);
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

int main() {
    int sockfd;
    struct sockaddr_in my_addr;
    pthread_t message_thread_id;

    sockfd = initUserAddress(&my_addr);
    if (sockfd < 0) {
        fprintf(stderr, "Khởi tạo địa chỉ UE thất bại\n");
        return -1;
    }

    // Khởi tạo thread nhận message (MIB và paging)
    int* sockfd_ptr = malloc(sizeof(int));
    *sockfd_ptr = sockfd;
    pthread_create(&message_thread_id, NULL, message_reception_thread, sockfd_ptr);
    
    printf("[UE] Đang đợi message từ gNodeB...\n");

    while (1) {
        usleep(SFN_INCREASE_TIME_MS * 1000);
        
        if (atomic_load(&is_synced)) {
            int current = atomic_load(&UE_sfn);
            atomic_store(&UE_sfn, (current + 1) % (MAX_SFN + 1));
            
            int display_sfn = atomic_load(&UE_sfn);
            if (display_sfn % 100 == 0) {
                printf("[UE] Tick - SFN: %d\n", display_sfn);
            }
        }
    }
    
    close(sockfd);
    pthread_join(message_thread_id, NULL);
    return 0;
}
