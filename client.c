#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char const *argv[]) {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};
    char message[1024];

    // Tạo socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8080);


    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    // Kết nối tới server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

    // Gửi và nhận nhiều thông điệp
    while(1) {
        printf("Nhập tin nhắn gửi tới server: ");
        fgets(message, 1024, stdin);

        // Gửi tin nhắn tới server
        send(sock, message, strlen(message), 0);
        printf("Tin nhắn đã được gửi\n");

        // Nhận phản hồi từ server
        read(sock, buffer, 1024);
        printf("Phản hồi từ server: %s\n", buffer);

        // Thoát nếu người dùng nhập "exit"
        if (strncmp(message, "exit", 4) == 0) {
            printf("Đã ngắt kết nối\n");
            break;
        }
    }

    return 0;
}
