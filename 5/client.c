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

    client_socket = socket(AF_INET, SOCK_STREAM, 0);

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);

    connect(client_socket, (struct sockaddr*)&server_address, sizeof(server_address));

    printf("Connected to server: %s:%d\n", server_ip, port);

    float expected, actual;
    char buffer[1024];

    recv(client_socket, buffer, sizeof(float) * 2, 0);
    memcpy(&expected, buffer, sizeof(float));
    memcpy(&actual, buffer + sizeof(float), sizeof(float));

    if (expected != actual) {
        sprintf(buffer, "НЕВЕРНОЕ распределение. Ожидалось %.3f, получил %.3f.\n", expected, actual);
    } else {
        sprintf(buffer, "ВЕРНОЕ распределение\n");
    }

    send(client_socket, buffer, strlen(buffer), 0);

    close(client_socket);
    printf("Client shutdown\n");

    return 0;
}