#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdatomic.h>
#include "gNodeB.h"

int main() {

    /* Khởi tạo paging queue */
    paging_queue.head = 0;
    paging_queue.tail = 0;
    paging_queue.count = 0;
    pthread_mutex_init(&paging_queue.mutex, NULL);
    pthread_cond_init(&paging_queue.cond, NULL);

    ue_sockfd = initUserAddress(&ue_addr);
    if (ue_sockfd < 0) {
        printf("Khởi tạo địa chỉ UE thất bại\n");
        return -1;
    }

    paging_sockfd = initPagingServer(&paging_server_addr);
    if (paging_sockfd < 0) {
        printf("Khởi tạo paging server thất bại\n");
        close(ue_sockfd);
        return -1;
    }
    
    /* Khởi tạo paging server thread */
    pthread_t paging_server_thread;
    if (pthread_create(&paging_server_thread, NULL, pagingServerThread, &paging_sockfd) != 0) {
        printf("Khởi tạo paging server thread thất bại\n");
        close(ue_sockfd);
        close(paging_sockfd);
        return -1;
    }

    /* Khởi tạo worker thread pool */
    pthread_t worker_threads[NUM_WORKER_THREADS];
    for (int i = 0; i < NUM_WORKER_THREADS; i++) {
        if (pthread_create(&worker_threads[i], NULL, pagingWorkerThread, NULL) != 0) {
            printf("Khởi tạo worker thread %d thất bại\n", i);
            close(ue_sockfd);
            close(paging_sockfd);
            return -1;
        }
    }

    printf("[gNodeB] Bắt đầu phát sóng...\n");

    while (1) {
        if (atomic_load(&gNodeB_sfn) % MIB_CYCLE == 0) {
            sendMIB(ue_sockfd, &ue_addr, atomic_load(&gNodeB_sfn));
        }

        usleep(SFN_INCREASE_TIME_MS * 1000); // Đợi 10ms
        atomic_store(&gNodeB_sfn, (atomic_load(&gNodeB_sfn) + 1) % (MAX_SFN + 1));
        
        if (atomic_load(&gNodeB_sfn) % 100 == 0)
            printf("[gNodeB] SFN hiện tại: %d\n", atomic_load(&gNodeB_sfn));
    }

    pthread_join(paging_server_thread, NULL);
    for (int i = 0; i < NUM_WORKER_THREADS; i++) {
        pthread_join(worker_threads[i], NULL);
    }
    close(ue_sockfd);
    close(paging_sockfd);
    return 0;
}