#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "AMF.h"
#include "../../include/common.h"

int32_t initGNodeBAddress(struct sockaddr_in* gnb_addr) {
    int32_t sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }

    memset(gnb_addr, 0, sizeof(*gnb_addr));
    gnb_addr->sin_family = AF_INET;
    gnb_addr->sin_port = htons(GNODEB_TCP_PORT);
    gnb_addr->sin_addr.s_addr = inet_addr(GNODEB_SERVER_IP);

    if (connect(sockfd, (struct sockaddr*)gnb_addr, sizeof(*gnb_addr)) < 0) {
        perror("connect");
        close(sockfd);
        return -1;
    }
    return sockfd;
}