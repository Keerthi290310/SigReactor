#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define SERVER_IP "127.0.0.1"
#define PORT 8080
#define BUFFER_SIZE 1024

int main(void)
{
    int sock_fd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    /* Create TCP socket */
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (sock_fd == -1)
    {
        perror("socket");
        return 1;
    }

    /* Configure server address */
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0)
    {
        perror("inet_pton");
        close(sock_fd);
        return 1;
    }

    /* Connect to SigReactor server */
    if (connect(sock_fd,
                (struct sockaddr *)&server_addr,
                sizeof(server_addr)) == -1)
    {
        perror("connect");
        close(sock_fd);
        return 1;
    }

    printf("====================================\n");
    printf("       SigReactor Client\n");
    printf("====================================\n");
    printf("Connected to server: %s:%d\n", SERVER_IP, PORT);

    /* Send message */
    const char *message = "Hello from client\n";

    ssize_t bytes_sent = write(sock_fd, message, strlen(message));

    if (bytes_sent == -1)
    {
        perror("write");
        close(sock_fd);
        return 1;
    }

    printf("Message sent: %s", message);

    /* Receive server response */
    ssize_t bytes_read = read(sock_fd, buffer, BUFFER_SIZE - 1);

    if (bytes_read == -1)
    {
        perror("read");
        close(sock_fd);
        return 1;
    }

    if (bytes_read == 0)
    {
        printf("Server closed the connection.\n");
    }
    else
    {
        buffer[bytes_read] = '\0';

        printf("Server response: %s", buffer);
    }

    /* Close connection */
    close(sock_fd);

    printf("Connection closed.\n");

    return 0;
}
