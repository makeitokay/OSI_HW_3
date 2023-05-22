#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/random.h>
#include <sys/socket.h>
#include <netinet/in.h>

int getRandomInt(int exclusive_max) {
    int rnd;
    getrandom(&rnd, sizeof(int), GRND_NONBLOCK);
    return abs(rnd) % exclusive_max;
}

int main(int argc, char* argv[]) {
    int port = atoi(argv[1]);
    int count = atoi(argv[2]);

    if (argc != count + 4) {
        printf("Некорректное число аргументов");
        return 1;
    }

    int server_socket, client_socket;
    struct sockaddr_in server_address, client_address;
    socklen_t client_address_length;
    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(port);

    bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address));
    listen(server_socket, count);

    int sum = atoi(argv[3]);
    float parts[count];
    for (int i = 0; i < count; ++i) {
        parts[i] = atof(argv[i + 4]);
    }

    int* clients = (int*)malloc(count * sizeof(int));
    for (int i = 0; i < count; ++i) {
        client_address_length = sizeof(client_address);
        client_socket = accept(server_socket, (struct sockaddr*)&client_address, &client_address_length);

        printf("Client connected: %s:%d\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

        clients[i] = client_socket;
    }

    int shm = shm_open("shm", O_CREAT | O_RDWR, 0666);
    ftruncate(shm, 4 * count);
    float* lawyer_distribution = mmap(NULL, 4 * count, PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);

    char* sem_names[count];
    for (int i = 0; i < count; ++i) {
        char* str = (char*)malloc(100 * sizeof(char));
        sprintf(str, "SEM_%d", i + 1);
        sem_names[i] = str;
    }

    sem_t* sem[count];
    for (int i = 0; i < count; ++i) {
        lawyer_distribution[i] = -1;
        sem[i] = sem_open(sem_names[i], O_CREAT, 0666, 1);
    }

    for (int i = 0; i < count; ++i) {
        pid_t pid = fork();
        if (pid == 0) {
            sem_wait(sem[i]);

            sleep(getRandomInt(2));

            int rnd = getRandomInt(10);
            if (rnd >= 8) {
                // адвокат решает поступить нечестно (с шансом 20%)
                int rnd2 = getRandomInt(2);
                if (rnd2 == 0) {
                    // адвокат часть денег себе
                    lawyer_distribution[i] = sum * parts[i] / 2;
                    printf("[info]: Адвокат забирает часть денег от %d наследника себе\n", i + 1);
                } else {
                    // адвокат отдает часть денег другому наследнику

                    int rnd_person = getRandomInt(count);
                    while (rnd_person == i) {
                        rnd_person = getRandomInt(count);
                    }

                    sem_wait(sem[rnd_person]);

                    // отдаем деньги другому наследнику только если ему уже распределено, если нет -
                    // то поступаем честно
                    if (lawyer_distribution[rnd_person] != -1) {
                        printf(
                            "[info]: Адвокат отсыпает часть денег от %d наследника %d наследнику\n",
                            i + 1, rnd_person + 1);
                        lawyer_distribution[rnd_person] += sum * parts[i] / 2;
                        lawyer_distribution[i] = sum * parts[i] / 2;
                    } else {
                        printf(
                            "[info]: Адвокат поступает честно и распределяет наследство %d "
                            "наследнику\n",
                            i + 1);
                        lawyer_distribution[i] = sum * parts[i];
                    }

                    sem_post(sem[rnd_person]);
                }
            } else {
                // адвокат поступает честно с шансом 80%
                printf("[info]: Адвокат поступает честно и распределяет наследство %d наследнику\n",
                       i + 1);
                lawyer_distribution[i] = sum * parts[i];
            }

            sem_post(sem[i]);

            return 0;
        }
    }

    for (int i = 0; i < count; ++i) {
        wait(NULL);
    }

    char buffer[1024];

    printf("\nНаследники проверяют распределение наследства в соответствии со своими долями!\n");
    for (int i = 0; i < count; ++i) {
        float expected = sum * parts[i];
        float actual = lawyer_distribution[i];

        memset(buffer, 0, 1024);
        memcpy(buffer, &expected, sizeof(float));
        memcpy(buffer + sizeof(float), &actual, sizeof(float));

        send(clients[i], buffer, sizeof(float) * 2, 0);
    }

    for (int i = 0; i < count; ++i) {
        memset(buffer, 0, 1024);
        recv(clients[i], buffer, 1024, 0);
        printf("%d-ый наследник: %s\n", i + 1, buffer);

        close(clients[i]);

    }

    close(server_socket);
    free(clients);

    munmap(lawyer_distribution, 4 * count);
    close(shm);
    shm_unlink("shm");
    for (int i = 0; i < count; ++i) {
        sem_close(sem[i]);
        sem_unlink(sem_names[i]);
    }

    return 0;
}