#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <ctype.h>

#define PORT 8080

void convertToUpper(char *str) {
    for (int i = 0; str[i]; i++) {
        str[i] = toupper(str[i]);
    }
}

int main(int argc, char const *argv[]) {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};

    // Tạo file descriptor cho socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Gán socket tới port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR , &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind socket tới địa chỉ và port đã chỉ định
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Lắng nghe kết nối
    if (listen(server_fd, 5) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server đang lắng nghe trên port %d...\n", PORT);

    while(1) {
        // Chấp nhận kết nối từ client
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        // Tạo tiến trình con để xử lý client
        pid_t pid = fork();

        if (pid == 0) {
            close(server_fd);

            while(1) {
                memset(buffer, 0, 1024);
                int valread = read(new_socket, buffer, 1024);
                if (valread == 0) {
                    printf("Client đã ngắt kết nối\n");
                    break;
                }

                printf("Tin nhắn từ client: %s\n", buffer);

                // Chuyển chuỗi thành in hoa
                convertToUpper(buffer);

                // Gửi phản hồi tới client
                send(new_socket, buffer, strlen(buffer), 0);
                printf("Phản hồi đã được gửi: %s\n", buffer);


                if (strncmp(buffer, "EXIT", 4) == 0) {
                    printf("Ngắt kết nối với client\n");
                    break;
                }
            }
            close(new_socket);
            exit(0); 
        } else {
            close(new_socket);
        }
    }

    return 0;
}
