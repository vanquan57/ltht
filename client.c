#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>

// Hàm xử lý phản hồi từ server
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

// Hàm kiểm tra file tồn tại
int file_exists(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file) {
        fclose(file);
        return 1;
    }
    return 0;
}

// Hàm đọc nội dung từ file
void read_from_file_and_send(int sock, const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("Không thể mở file %s\n", filename);
        return;
    }

    char line[1024];
    while (fgets(line, sizeof(line), file)) {
        send(sock, line, strlen(line), 0);
        printf("Đã gửi dòng: %s", line);
    }
    fclose(file);
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
        int choice;
        printf("\nChọn phương thức gửi tin nhắn:\n");
        printf("1. Nhập dữ liệu từ terminal\n");
        printf("2. Nhập đường dẫn file .txt\n");
        printf("Lựa chọn của bạn: ");
        scanf("%d", &choice);
        getchar(); // Đọc ký tự xuống dòng còn lại trong bộ đệm

        if (choice == 1) {
            printf("Nhập tin nhắn gửi tới server: ");
            fgets(message, 1024, stdin);

            send(sock, message, strlen(message), 0);
            printf("Tin nhắn đã được gửi\n");

            if (strncmp(message, "exit", 4) == 0) {
                printf("Đã ngắt kết nối\n");
                break;
            }
        } else if (choice == 2) {
            char filepath[1024];
            printf("Nhập đường dẫn file: ");
            fgets(filepath, sizeof(filepath), stdin);
            filepath[strcspn(filepath, "\n")] = 0; // Xóa ký tự xuống dòng

            if (file_exists(filepath)) {
                read_from_file_and_send(sock, filepath);
            } else {
                printf("File không tồn tại!\n");
            }
        } else {
            printf("Lựa chọn không hợp lệ!\n");
        }
    }

    pthread_join(recv_thread, NULL);
    close(sock);
    return 0;
}
