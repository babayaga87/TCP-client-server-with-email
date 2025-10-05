// server.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <winsock2.h>     // ⚠️ PHẢI include đầu tiên
#include <ws2tcpip.h>     // thêm cái này kế tiếp
#include <windows.h>      // chỉ include sau winsock2.h
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define BUFFER_SIZE 1024

// Hàm gửi email bằng PowerShell (trả về true nếu exit code = 0)
bool send_email_powershell(const char* smtp, const char* username, const char* password,
    const char* from, const char* to, const char* subject, const char* body)
{
    // Build command (chú ý escape ký tự nếu body có dấu ' hoặc " lớn)
    char cmd[8192];
    // Sử dụng ConvertTo-SecureString để tạo PSCredential
    // Lưu ý: password sẽ hiển thị trong process list => chỉ dùng cho test
    snprintf(cmd, sizeof(cmd),
        "powershell -Command \""
        "$pass = ConvertTo-SecureString '%s' -AsPlainText -Force; "
        "$cred = New-Object System.Management.Automation.PSCredential('%s', $pass); "
        "Send-MailMessage -SmtpServer 'smtp.gmail.com' -Port 587 -UseSsl -Credential $cred "
        "-From '%s' -To '%s' -Subject '%s' -Body '%s'\"",
        password, username, from, to, subject, body);

    // ✅ In ra lệnh PowerShell để kiểm tra
    printf("\n[DEBUG] PowerShell command:\n%s\n\n", cmd);

    // system() sẽ trả về exit code của PowerShell. 0 thường là thành công.
    int rc = system(cmd);
    return rc == 0;
}

int main() {
    WSADATA wsaData;
    SOCKET server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    int client_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];
    char client_ip[INET_ADDRSTRLEN];
    char server_ip[INET_ADDRSTRLEN];

    // 1. Khởi tạo Winsock
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        printf("WSAStartup failed: %d\n", result);
        return 1;
    }

    printf("Winsock initialized successfully.\n");

    // 2. Tạo socket
    server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_socket == INVALID_SOCKET) {
        printf("Socket creation failed: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // 3. Thiết lập địa chỉ server
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // Lắng nghe trên tất cả interfaces
    server_addr.sin_port = htons(PORT);

    // 4. Bind socket với địa chỉ
    result = bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (result == SOCKET_ERROR) {
        printf("Bind failed: %d\n", WSAGetLastError());
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    // 5. Bắt đầu lắng nghe kết nối
    result = listen(server_socket, 5);
    if (result == SOCKET_ERROR) {
        printf("Listen failed: %d\n", WSAGetLastError());
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    // Lấy IP của server
    struct sockaddr_in local_addr;
    int local_len = sizeof(local_addr);
    if (getsockname(server_socket, (struct sockaddr*)&local_addr, &local_len) == 0) {
        inet_ntop(AF_INET, &local_addr.sin_addr, server_ip, INET_ADDRSTRLEN);
    }
    else {
        strcpy(server_ip, "0.0.0.0");
    }

    // In thông tin server
    printf("\n=== TCP SERVER STARTED ===\n");
    printf("Server IP: %s\n", server_ip);
    printf("Server Port: %d\n", PORT);
    printf("Waiting for connections...\n\n");

    while (1) {
        // 6. Chấp nhận kết nối từ client
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket == INVALID_SOCKET) {
            printf("Accept failed: %d\n", WSAGetLastError());
            continue;
        }

        // Chuyển IP client sang dạng string
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);

        // In thông tin client kết nối
        printf("=== NEW CLIENT CONNECTED ===\n");
        printf("Client IP: %s\n", client_ip);
        printf("Client Port: %d\n", ntohs(client_addr.sin_port));
        printf("----------------------------\n");

        char hostname[256];
        if (gethostname(hostname, sizeof(hostname)) == SOCKET_ERROR) {
            printf("gethostname failed: %d\n", WSAGetLastError());
        }
        else {
            struct addrinfo hints, * info, * p;
            memset(&hints, 0, sizeof(hints));
            hints.ai_family = AF_INET; // chỉ IPv4
            hints.ai_socktype = SOCK_STREAM;

            if (getaddrinfo(hostname, NULL, &hints, &info) == 0) {
                for (p = info; p != NULL; p = p->ai_next) {
                    struct sockaddr_in* addr = (struct sockaddr_in*)p->ai_addr;
                    char local_ip[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &addr->sin_addr, local_ip, sizeof(local_ip));
                    printf("Local IP: %s\n", local_ip);
                    break; // in ra IP đầu tiên thôi
                }
                freeaddrinfo(info);
            }
            else {
                printf("getaddrinfo failed.\n");
            }
        }

        printf("Waiting for connections...\n\n");

        // 7. Nhận và xử lý tin nhắn từ client
        while (1) {
            memset(buffer, 0, BUFFER_SIZE);
            int bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);

            if (bytes_received == SOCKET_ERROR) {
                printf("Receive error: %d\n", WSAGetLastError());
                break;
            }
            else if (bytes_received == 0) {
                printf("Client disconnected gracefully.\n\n");
                break;
            }

            buffer[bytes_received] = '\0'; // Đảm bảo null-terminated
            printf("Message from client [%s:%d]: %s",
                client_ip,
                ntohs(client_addr.sin_port),
                buffer);
            for (int i = 0; buffer[i]; i++) {
                if (buffer[i] == '\r' || buffer[i] == '\n') buffer[i] = '\0';
            }
            // sau khi có `buffer` chứa message:
            send_email_powershell("smtp.gmail.com", "huynhvykhang1234@gmail.com", "pass",
                "huynhvykhang1234@gmail.com", "rokkegiaumat@gmail.com",
                "TCP message received", buffer);
            // Echo tin nhắn về lại client
            int bytes_sent = send(client_socket, buffer, bytes_received, 0);
            if (bytes_sent == SOCKET_ERROR) {
                printf("Send error: %d\n", WSAGetLastError());
                break;
            }
        }

        closesocket(client_socket);
        printf("Waiting for new connections...\n\n");
    }

    closesocket(server_socket);
    WSACleanup();
    return 0;
}


// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
