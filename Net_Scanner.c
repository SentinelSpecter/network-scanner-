#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>

#define BUFFER_SIZE 1024
#define SCAN_RANGE 254
#define TIMEOUT 1 // timeout in seconds

void print_progress(int current, int total) {
    int i;
    float percentage = (float)current / total * 100;

    printf("\rScanning... [%3d%%] ", (int)percentage);
    for (i = 0; i < 50; i++) {
        if (i < (int)(percentage / 2)) {
            printf("#");
        } else {
            printf(" ");
        }
    }
    printf("]");

    fflush(stdout);
}

void sig_alrm(int signum) {
    printf("\nTimeout!\n");
    exit(1);
}

int main() {
    int sockfd, i;
    struct sockaddr_in sin;
    struct icmp *icmp;
    u_char buffer[BUFFER_SIZE];
    clock_t start_time, end_time;
    double scan_time;

    // Create a raw socket
    sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd < 0) {
        perror("socket");
        exit(1);
    }

    // Set up the ICMP Echo Request packet
    icmp = (struct icmp *)buffer;
    icmp->icmp_type = ICMP_ECHO;
    icmp->icmp_code = 0;
    icmp->icmp_id = htons(1234);
    icmp->icmp_seq = htons(1);

    // Set up the IP address
    sin.sin_addr.s_addr = inet_addr("192.168.1.1"); // change this to your network's IP range
    sin.sin_family = AF_INET;

    // Start the timer
    start_time = clock();

    // Set up the alarm signal
    signal(SIGALRM, sig_alrm);
    alarm(TIMEOUT);

    // Send the ICMP Echo Request packets
    for (i = 1; i <= SCAN_RANGE; i++) {
        sin.sin_addr.s_addr = inet_addr("192.168.1.1") + i;
        sendto(sockfd, buffer, sizeof(struct icmp), 0, (struct sockaddr *)&sin, sizeof(sin));

        // Print progress
        print_progress(i, SCAN_RANGE);

        // Wait for a short period to avoid flooding the network
        usleep(1000);
    }

    // Stop the alarm signal
    alarm(0);

    // Wait for responses
    while (1) {
        recvfrom(sockfd, buffer, BUFFER_SIZE, 0, NULL, NULL);
        icmp = (struct icmp *)buffer;

        if (icmp->icmp_type == ICMP_ECHOREPLY) {
            printf("\nIP address: %s\n", inet_ntoa(sin.sin_addr));
        }
    }

    // Stop the timer
    end_time = clock();
    scan_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;

    printf("\nScan completed in %.2f seconds.\n", scan_time);

    return 0;
}