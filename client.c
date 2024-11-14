#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

void* receive_from_server(void* sock_ptr) {
    int sock = *(int*)sock_ptr;
    char buffer[1024];
    while (1) {
        int valread = read(sock, buffer, sizeof(buffer));
        if (valread > 0) {
            buffer[valread] = '\0';
            printf("Phản hồi từ server: %s\n", buffer);
        }
    }
    return NULL;
}

int main(int argc, char const *argv[]) {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char message[1024];
    pthread_t recv_thread;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8080);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

    pthread_create(&recv_thread, NULL, receive_from_server, &sock);

    while (1) {
        printf("Nhập tin nhắn gửi tới server: ");
        fgets(message, 1024, stdin);

        send(sock, message, strlen(message), 0);
        printf("Tin nhắn đã được gửi\n");

        if (strncmp(message, "exit", 4) == 0) {
            printf("Đã ngắt kết nối\n");
            break;
        }
    }

    pthread_join(recv_thread, NULL);
    return 0;
}
