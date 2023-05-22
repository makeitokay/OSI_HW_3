#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Некорректное число аргументов");
        return 1;
    }

    char* server_ip = argv[1];
    int port = atoi(argv[2]);

    int monitor_socket;
    struct sockaddr_in server_address;

    if ((monitor_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);

    if (inet_pton(AF_INET, server_ip, &server_address.sin_addr) <= 0) {
        perror("inet_pton");
        exit(EXIT_FAILURE);
    }

    if (connect(monitor_socket, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    printf("Connected to server: %s:%d\n", server_ip, port);

    char* buffer = malloc(2048);
    while (1) {
        int bytes_received = recv(monitor_socket, buffer, sizeof(char) * 2048, 0);
        if (bytes_received < 0) {
            perror("recv");
            exit(EXIT_FAILURE);
        }

        char *stop_msg = malloc(5);
        memcpy(stop_msg, buffer, 5);
        stop_msg[4] = '\0';
        if (strcmp("stop", stop_msg) == 0) {
            free(stop_msg);
            break;
        }

        printf("%s", buffer);
    }

    free(buffer);
    close(monitor_socket);

    return 0;
}