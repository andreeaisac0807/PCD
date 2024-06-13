#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>

extern int errno;

#define ADDR "127.0.0.1"
#define PORT 8080
#define BUFFER_SIZE 1024

int create_socket() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    return fd;
}

static void errAndExit(char * const log)
{
    perror(log);
    exit(errno);
}

void connect_socket(int sd, struct sockaddr_in addr, int poll_rate) {
    while (connect(sd, (const struct sockaddr *)&addr, (socklen_t)sizeof(addr)) < 0) {
        fprintf(stdout, "[INFO] Still connecting...\n");
        sleep(poll_rate);
    }
}

int create_client_socket() {
    int fd = create_socket();
    struct sockaddr_in addr;

    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ADDR);
    addr.sin_port = htons(PORT);

    connect_socket(fd, addr, 1);
    fprintf(stdout, "[INFO]\tSocket connected.\n");
    return fd;
}

void send_message(int sd, const char *message) {
    if (send(sd, message, strlen(message), 0) < 0) {
        errAndExit("sending message on socket");
    }
}

char *get_message(int sd) {
    char *recval = (char *)malloc(BUFFER_SIZE);
    int bytes_received = recv(sd, recval, BUFFER_SIZE, 0);
    if (bytes_received < 0) {
        errAndExit("reading message from socket");
    }
    recval[bytes_received] = '\0';
    return recval;
}

void send_file(int sd, const char *file_path) {
    FILE *file = fopen(file_path, "rb");
    if (!file) {
        errAndExit("opening file");
    }

    // Calculate file size
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    printf("Sending file of size: %zu bytes\n", file_size);

    char buffer[BUFFER_SIZE];
    size_t n;

    while ((n = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        if (send(sd, buffer, n, 0) < 0) {
            errAndExit("sending file");
        }
    }

    fclose(file);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <file_path> <name>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *file_path = argv[1];
    char *name = argv[2];

    int fd_client = create_client_socket();

    // Send the client's name
    char name_msg[BUFFER_SIZE];
    snprintf(name_msg, sizeof(name_msg), "name:%s", name);
    send_message(fd_client, name_msg);

    // Wait for the approval response from the admin
    char *response = get_message(fd_client);
    if (strcmp(response, "y") == 0) {
        printf("[INFO] Connection approved by admin.\n");
        // Send the file after approval
        send_file(fd_client, file_path);
        printf("[INFO] Client sent the file.\n");
    } else {
        printf("[INFO] Connection denied by admin.\n");
    }

    free(response);
    close(fd_client);
    return 0;
}
