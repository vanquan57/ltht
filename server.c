#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <ctype.h>

#define PORT 8080

int clients[100];
int client_count = 0;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void log_to_file(int client_id, const char* message) {
    FILE* file = fopen("output.txt", "a");
    if (!file) {
        perror("Không thể mở file output.txt");
        return;
    }

    // Lấy thời gian hiện tại
    time_t now = time(NULL);
    struct tm *local = localtime(&now);

    // Ghi nội dung vào file
    fprintf(file, "client id: %d | ngày giờ: %02d:%02d:%02d %02d/%02d/%d | nội dung nhận được: %s\n",
            client_id,
            local->tm_hour, local->tm_min, local->tm_sec,
            local->tm_mday, local->tm_mon + 1, local->tm_year + 1900,
            message);
    fclose(file);
}

void* send_time_to_clients(void* arg) {
    while (1) {
        sleep(30);

        time_t now = time(NULL);
        struct tm *local = localtime(&now);
        char time_message[1024];
        snprintf(time_message, sizeof(time_message), "Server time: %02d:%02d:%02d %02d/%02d/%d\n",
                 local->tm_hour, local->tm_min, local->tm_sec,
                 local->tm_mday, local->tm_mon + 1, local->tm_year + 1900);

        pthread_mutex_lock(&clients_mutex);
        for (int i = 0; i < client_count; i++) {
            send(clients[i], time_message, strlen(time_message), 0);
        }
        pthread_mutex_unlock(&clients_mutex);
    }
    return NULL;
}

void* handle_client(void* client_socket) {
    int sock = *(int*)client_socket;
    char buffer[1024] = {0};

    while (1) {
        int valread = read(sock, buffer, 1024);
        if (valread <= 0) {
            printf("Client đã ngắt kết nối\n");
            break;
        }
        buffer[valread] = '\0';  // Đảm bảo chuỗi kết thúc
        printf("Tin nhắn từ client: %s\n", buffer);

        // Ghi log vào file
        log_to_file(sock, buffer);

        // Chuyển chuỗi thành in hoa và gửi lại client
        for (int i = 0; buffer[i]; i++) {
            buffer[i] = toupper(buffer[i]);
        }
        send(sock, buffer, strlen(buffer), 0);

        if (strncmp(buffer, "EXIT", 4) == 0) {
            printf("Ngắt kết nối với client\n");
            break;
        }
    }

    // Xóa client ra khỏi danh sách
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < client_count; i++) {
        if (clients[i] == sock) {
            clients[i] = clients[client_count - 1];
            client_count--;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);

    close(sock);
    free(client_socket);
    return NULL;
}

int main(int argc, char const *argv[]) {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    pthread_t time_thread;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR , &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 5) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server đang lắng nghe trên port %d...\n", PORT);

    // Tạo thread để gửi thời gian
    pthread_create(&time_thread, NULL, send_time_to_clients, NULL);

    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        printf("Client đã kết nối\n");

        // Lưu client vào danh sách
        pthread_mutex_lock(&clients_mutex);
        clients[client_count++] = new_socket;
        pthread_mutex_unlock(&clients_mutex);

        int* new_sock = malloc(1);
        *new_sock = new_socket;
        pthread_t client_thread;
        pthread_create(&client_thread, NULL, handle_client, new_sock);
    }

    close(server_fd);
    return 0;
}
