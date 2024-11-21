#include <unistd.h>  // Thư viện hỗ trợ các hàm liên quan đến hệ thống như sleep().
#include <stdio.h>   // Thư viện nhập/xuất chuẩn (printf, scanf, v.v.).
#include <sys/socket.h> // Thư viện hỗ trợ giao tiếp qua socket.
#include <stdlib.h>  // Thư viện hỗ trợ các hàm như malloc, free, exit.
#include <netinet/in.h> // Thư viện chứa các định nghĩa liên quan đến giao thức mạng (IPv4).
#include <string.h>  // Thư viện hỗ trợ các hàm xử lý chuỗi (strlen, strcpy, v.v.).
#include <pthread.h> // Thư viện hỗ trợ các thao tác với thread (pthread).
#include <time.h>    // Thư viện hỗ trợ các hàm làm việc với thời gian.
#include <ctype.h>   // Thư viện hỗ trợ các hàm xử lý ký tự (chuyển sang chữ hoa, chữ thường, v.v.).

#define PORT 8080  // Định nghĩa cổng mà server sẽ lắng nghe kết nối.

int clients[100];  // Mảng chứa các client kết nối đến server.
int client_count = 0;  // Số lượng client đang kết nối.
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER; // Khóa mutex để đồng bộ hóa truy cập vào danh sách clients.

void log_to_file(int client_id, const char* message) {
    FILE* file = fopen("output.txt", "a"); // Mở file output.txt để ghi log.
    if (!file) { // Nếu không thể mở file.
        perror("Không thể mở file output.txt"); // In lỗi.
        return;
    }

    // Lấy thời gian hiện tại.
    time_t now = time(NULL);
    struct tm *local = localtime(&now);

    // Ghi log vào file với thông tin client, thời gian và nội dung nhận được.
    fprintf(file, "client id: %d | ngày giờ: %02d:%02d:%02d %02d/%02d/%d | nội dung nhận được: %s\n",
            client_id,
            local->tm_hour, local->tm_min, local->tm_sec,
            local->tm_mday, local->tm_mon + 1, local->tm_year + 1900,
            message);
    fclose(file);  // Đóng file sau khi ghi.
}

void* send_time_to_clients(void* arg) {
    while (1) {
        sleep(30);  // Chờ 30 giây.

        // Lấy thời gian hiện tại và format thành chuỗi.
        time_t now = time(NULL);
        struct tm *local = localtime(&now);
        char time_message[1024];
        snprintf(time_message, sizeof(time_message), "Server time: %02d:%02d:%02d %02d/%02d/%d\n",
                 local->tm_hour, local->tm_min, local->tm_sec,
                 local->tm_mday, local->tm_mon + 1, local->tm_year + 1900);

        // Khóa mutex để bảo vệ danh sách client trong quá trình gửi dữ liệu.
        pthread_mutex_lock(&clients_mutex);
        for (int i = 0; i < client_count; i++) {
            send(clients[i], time_message, strlen(time_message), 0);  // Gửi thời gian đến tất cả client.
        }
        pthread_mutex_unlock(&clients_mutex);  // Mở khóa mutex.
    }
    return NULL;  // Kết thúc thread.
}

void* handle_client(void* client_socket) {
    int sock = *(int*)client_socket;  // Lấy socket của client.
    char buffer[1024] = {0};  // Bộ đệm để lưu dữ liệu nhận từ client.

    while (1) {
        int valread = read(sock, buffer, 1024);  // Đọc dữ liệu từ client.
        if (valread <= 0) {  // Nếu không nhận được dữ liệu hoặc client ngắt kết nối.
            printf("Client đã ngắt kết nối\n");
            break;  // Kết thúc vòng lặp.
        }
        buffer[valread] = '\0';  // Đảm bảo chuỗi kết thúc đúng.
        printf("Tin nhắn từ client: %s\n", buffer);  // In tin nhắn nhận được.

        // Ghi log vào file.
        log_to_file(sock, buffer);

        // Chuyển chuỗi thành chữ hoa và gửi lại cho client.
        for (int i = 0; buffer[i]; i++) {
            buffer[i] = toupper(buffer[i]);
        }
        send(sock, buffer, strlen(buffer), 0);  // Gửi chuỗi đã thay đổi lại cho client.

        if (strncmp(buffer, "EXIT", 4) == 0) {  // Nếu nhận được thông điệp "EXIT", ngắt kết nối.
            printf("Ngắt kết nối với client\n");
            break;  // Kết thúc vòng lặp.
        }
    }

    // Xóa client khỏi danh sách.
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < client_count; i++) {
        if (clients[i] == sock) {
            clients[i] = clients[client_count - 1];  // Đổi chỗ client bị xóa với client cuối cùng.
            client_count--;  // Giảm số lượng client.
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);  // Mở khóa mutex.

    close(sock);  // Đóng kết nối với client.
    free(client_socket);  // Giải phóng bộ nhớ đã cấp phát cho client_socket.
    return NULL;  // Kết thúc thread.
}

int main(int argc, char const *argv[]) {
    int server_fd, new_socket;
    struct sockaddr_in address;  // Cấu trúc địa chỉ cho server.
    int opt = 1;
    int addrlen = sizeof(address);
    pthread_t time_thread;  // Biến để lưu thread gửi thời gian.

    // Tạo socket cho server.
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Cấu hình socket để tái sử dụng địa chỉ cũ.
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR , &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // Cấu hình địa chỉ server.
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Gắn kết socket với địa chỉ và cổng.
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Bắt đầu lắng nghe kết nối.
    if (listen(server_fd, 5) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server đang lắng nghe trên port %d...\n", PORT);

    // Tạo thread gửi thời gian cho các client.
    pthread_create(&time_thread, NULL, send_time_to_clients, NULL);

    // Vòng lặp chờ và xử lý các kết nối từ client.
    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        printf("Client đã kết nối\n");

        // Lưu client vào danh sách.
        pthread_mutex_lock(&clients_mutex);
        clients[client_count++] = new_socket;
        pthread_mutex_unlock(&clients_mutex);

        // Tạo thread để xử lý client mới kết nối.
        int* new_sock = malloc(1);
        *new_sock = new_socket;
        pthread_t client_thread;
        pthread_create(&client_thread, NULL, handle_client, new_sock);
    }

    close(server_fd);  // Đóng socket server khi kết thúc.
    return 0;
}
