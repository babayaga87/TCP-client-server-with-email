// TCP_client.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define BUFFER_SIZE 1024

int main(int argc, char* argv[]) {
    WSADATA wsaData;
    SOCKET client_socket;
    struct sockaddr_in server_addr, client_addr;
    int client_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];
    const char* server_ip;
    char client_ip[INET_ADDRSTRLEN];

    // Kiểm tra tham số dòng lệnh
    if (argc != 2) {
        printf("No server IP provided. Using default: 127.0.0.1\n");
        server_ip = "127.0.0.1";
    }
    else {
        server_ip = argv[1];
    }

    // 1. Khởi tạo Winsock
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        printf("WSAStartup failed: %d\n", result);
        return 1;
    }

    printf("Winsock initialized successfully.\n");

    // 2. Tạo socket
    client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client_socket == INVALID_SOCKET) {
        printf("Socket creation failed: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // 3. Thiết lập địa chỉ server
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    // Chuyển đổi IP từ string sang binary
    result = inet_pton(AF_INET, server_ip, &server_addr.sin_addr);
    if (result <= 0) {
        if (result == 0) {
            printf("Invalid IP address format: %s\n", server_ip);
        }
        else {
            printf("inet_pton failed: %d\n", WSAGetLastError());
        }
        closesocket(client_socket);
        WSACleanup();
        return 1;
    }

    // 4. Kết nối đến server
    printf("Attempting to connect to %s:%d...\n", server_ip, PORT);
    result = connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (result == SOCKET_ERROR) {
        printf("Connection failed: %d\n", WSAGetLastError());
        closesocket(client_socket);
        WSACleanup();
        return 1;
    }

    // Lấy thông tin local client
    if (getsockname(client_socket, (struct sockaddr*)&client_addr, &client_len) == 0) {
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);

        printf("\n=== CONNECTION ESTABLISHED ===\n");
        printf("Connected to server: %s:%d\n", server_ip, PORT);
        printf("Local client IP: %s\n", client_ip);
        printf("Local client Port: %d\n", ntohs(client_addr.sin_port));
        printf("===============================\n");
    }

    printf("You can now send messages to the server.\n");
    printf("Type 'quit' to exit.\n\n");

    // 5. Gửi và nhận tin nhắn
    while (1) {
        printf("Enter message: ");
        fflush(stdout);

        // Đọc input từ terminal
        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
            printf("\nInput error or EOF. Exiting...\n");
            break;
        }

        // Kiểm tra lệnh quit
        if (strncmp(buffer, "quit", 4) == 0) {
            printf("Disconnecting from server...\n");
            break;
        }

        // Gửi tin nhắn đến server
        int bytes_sent = send(client_socket, buffer, strlen(buffer), 0);
        if (bytes_sent == SOCKET_ERROR) {
            printf("Send failed: %d\n", WSAGetLastError());
            break;
        }

        // Nhận phản hồi từ server
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received == SOCKET_ERROR) {
            printf("Receive failed: %d\n", WSAGetLastError());
            break;
        }
        else if (bytes_received == 0) {
            printf("Server disconnected.\n");
            break;
        }

        buffer[bytes_received] = '\0';
        printf("Server echo: %s\n", buffer);
    }

    closesocket(client_socket);
    WSACleanup();
    printf("Client closed.\n");
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
