#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT 9095
#define BUFFER_SIZE 1024
void handle_request(int client_socket, char *file_path)
{
    FILE *file = fopen(file_path, "r");
    printf("%s", file_path);
    if (file == NULL)
    {
        printf("404 eror \n");
        char *message = "HTTP/1.1 404 Not Found\nContent-Type: text/plain\n\nFile not found";
        send(client_socket, message, strlen(message), 0);
    }
    else
    {
        char buffer[BUFFER_SIZE];
        fseek(file, 0L, SEEK_END);
        size_t file_size = ftell(file);
        rewind(file);

        char header[1024];
        snprintf(header, sizeof(header), "HTTP/1.1 200 OK\nContent-Length: %zu\nContent-Type: text/html\n\n", file_size);
        printf("HTTP/1.1 200 OK\nContent-Length:\nContent-Type: text/html\n\n");
        send(client_socket, header, strlen(header), 0);

        while (!feof(file))
        {
            size_t bytes_read = fread(buffer, 1, sizeof(buffer), file);
            send(client_socket, buffer, bytes_read, 0);
        }

        fclose(file);
    }
}

int main(int argc, char *argv[])
{
    int server_fd, client_socket;
    struct sockaddr_in server_addr, client_addr;
    int addrlen = sizeof(server_addr);
    // 创建 TCP 套接字
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // 绑定套接字到指定的端口
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // 监听端口，等待连接
    if (listen(server_fd, 3) < 0)
    {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    while (1)
    {
        // 接受连接
        if ((client_socket = accept(server_fd, (struct sockaddr *)&client_addr, (socklen_t *)&addrlen)) < 0)
        {
            perror("accept failed");
            exit(EXIT_FAILURE);
        }
        printf("read buf \n");
        printf("Connection accepted from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        printf("read buf \n");
        // 解析 HTTP 请求
        char buffer[BUFFER_SIZE];
        read(client_socket, buffer, BUFFER_SIZE);
        printf("read buf %s\n", buffer);
        // 从请求中提取文件路径
        char file_path[1024];
        sscanf(buffer, "GET %s HTTP", file_path);
        printf("file: %s\n", file_path);
        // 处理请求
        handle_request(client_socket, file_path + 1); // 去掉 URL 中的起始斜杠
        // 关闭客户端套接字
        close(client_socket);
    }

    return 0;
}
