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

    int client_socket;
    struct sockaddr_in server_address;

    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);

    if (inet_pton(AF_INET, server_ip, &server_address.sin_addr) <= 0) {
        perror("inet_pton");
        exit(EXIT_FAILURE);
    }

    if (connect(client_socket, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    printf("Connected to server: %s:%d\n", server_ip, port);

    float expected, actual;
    float *buffer = malloc(sizeof(float) * 2);

    recv(client_socket, buffer, sizeof(float) * 2, 0);
    expected = buffer[0];
    actual = buffer[1];

    char* str = (char*)malloc(2048);
    if (expected != actual) {
        sprintf(str, "НЕВЕРНОЕ распределение. Ожидалось %.3f, получил %.3f.\n", expected, actual);
    } else {
        sprintf(str, "ВЕРНОЕ распределение\n");
    }

    send(client_socket, str, strlen(str), 0);

    free(str);
    close(client_socket);
    printf("Client shutdown\n");

    return 0;
}