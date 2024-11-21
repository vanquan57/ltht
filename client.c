#include <stdio.h>                 // Thư viện chuẩn để sử dụng các hàm nhập xuất, như printf, fgets, v.v.
#include <sys/socket.h>            // Thư viện để làm việc với socket (giao thức mạng).
#include <arpa/inet.h>             // Thư viện cung cấp các hàm liên quan đến mạng, như inet_pton để chuyển đổi địa chỉ IP.
#include <unistd.h>                // Thư viện hỗ trợ các hàm như read, write, sleep.
#include <string.h>                // Thư viện cung cấp các hàm xử lý chuỗi, như strlen, strcmp, v.v.
#include <pthread.h>               // Thư viện cho phép sử dụng đa luồng (multithreading).
#include <stdlib.h>                // Thư viện chuẩn để sử dụng các hàm như malloc, free, exit.


// Hàm xử lý phản hồi từ server, chạy trong một luồng riêng
void* receive_from_server(void* sock_ptr) {
    int sock = *(int*)sock_ptr;   // Chuyển con trỏ socket từ đối số của hàm.
    char buffer[1024];            // Mảng buffer để lưu trữ dữ liệu nhận từ server.
    
    while (1) {                   
        int valread = read(sock, buffer, sizeof(buffer)); // Đọc dữ liệu từ server vào buffer.
        
        if (valread > 0) {         // Nếu có dữ liệu được nhận.
            buffer[valread] = '\0'; // Đảm bảo chuỗi kết thúc đúng.
            printf("Phản hồi từ server: %s\n", buffer); // In phản hồi từ server.
        }
    }
    return NULL; // Kết thúc hàm, trả về NULL
}

// Hàm kiểm tra xem file có tồn tại hay không
int file_exists(const char *filename) {
    FILE *file = fopen(filename, "r"); // Mở file ở chế độ đọc.
    if (file) {                      
        fclose(file);                // Nếu file mở thành công, đóng nó và trả về 1 (tồn tại).
        return 1;
    }
    return 0;  // Nếu không mở được file, trả về 0 (không tồn tại).
}

// Hàm đọc nội dung từ file và gửi đến server
void read_from_file_and_send(int sock, const char *filename) {
    FILE *file = fopen(filename, "r");  // Mở file để đọc.
    if (!file) {  // Nếu không thể mở file, in thông báo lỗi.
        printf("Không thể mở file %s\n", filename);
        return;
    }

    char line[1024];  // Mảng để chứa mỗi dòng đọc từ file.
    while (fgets(line, sizeof(line), file)) {  // Đọc từng dòng trong file.
        send(sock, line, strlen(line), 0);  // Gửi mỗi dòng đến server.
        printf("Đã gửi dòng: %s", line);  // In ra dòng đã gửi.
    }
    fclose(file);  // Đóng file sau khi gửi xong.
}

int main(int argc, char const *argv[]) {
    int sock = 0;                    // Biến socket.
    struct sockaddr_in serv_addr;    // Cấu trúc địa chỉ của server.
    char message[1024];               // Mảng để chứa tin nhắn người dùng nhập.
    pthread_t recv_thread;            // Biến luồng cho việc nhận dữ liệu từ server.

    // Tạo socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n"); // Thông báo lỗi nếu không tạo được socket.
        return -1;
    }

    serv_addr.sin_family = AF_INET;   // Chỉ định giao thức Internet (IPv4).
    serv_addr.sin_port = htons(8080); // Cổng server (8080).
    
    // Chuyển địa chỉ IP từ chuỗi sang dạng nhị phân.
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");  // Thông báo nếu địa chỉ không hợp lệ.
        return -1;
    }

    // Kết nối đến server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n"); // Thông báo nếu kết nối không thành công.
        return -1;
    }

    // Tạo một luồng để nhận dữ liệu từ server.
    pthread_create(&recv_thread, NULL, receive_from_server, &sock);

    while (1) {
        int choice;
        printf("\nChọn phương thức gửi tin nhắn:\n");
        printf("1. Nhập dữ liệu từ terminal\n");
        printf("2. Nhập đường dẫn file .txt\n");
        printf("Lựa chọn của bạn: ");
        scanf("%d", &choice); // Nhập lựa chọn của người dùng.
        getchar(); // Đọc ký tự xuống dòng còn lại trong bộ đệm.

        if (choice == 1) {  // Nếu người dùng chọn nhập tin nhắn từ terminal.
            printf("Nhập tin nhắn gửi tới server: ");
            fgets(message, 1024, stdin);  // Nhập tin nhắn từ bàn phím.

            send(sock, message, strlen(message), 0);  // Gửi tin nhắn tới server.
            printf("Tin nhắn đã được gửi\n");

            // Nếu tin nhắn là "exit", ngắt kết nối và thoát.
            if (strncmp(message, "exit", 4) == 0) {
                printf("Đã ngắt kết nối\n");
                break;
            }
        } else if (choice == 2) {  // Nếu người dùng chọn gửi file.
            char filepath[1024];
            printf("Nhập đường dẫn file: ");
            fgets(filepath, sizeof(filepath), stdin);  // Nhập đường dẫn file.

            filepath[strcspn(filepath, "\n")] = 0;  // Loại bỏ ký tự xuống dòng nếu có.

            // Kiểm tra nếu file tồn tại.
            if (file_exists(filepath)) {
                read_from_file_and_send(sock, filepath);  // Đọc từ file và gửi.
            } else {
                printf("File không tồn tại!\n");  // Thông báo nếu file không tồn tại.
            }
        } else {
            printf("Lựa chọn không hợp lệ!\n");  // Thông báo nếu lựa chọn không hợp lệ.
        }
    }

    pthread_join(recv_thread, NULL);  // Chờ luồng nhận dữ liệu từ server kết thúc.
    close(sock);  // Đóng kết nối socket.
    return 0;
}
