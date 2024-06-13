#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <errno.h>
#include <signal.h>
#include "image_handler1.h" // include the header for image processing
#include<stdbool.h>

extern int errno;
static int clientNo;
static int admin_fd = -1;
#define MAX_CLIENTS FD_SETSIZE
#define BUFFER_SIZE 1024
#define ADDR "127.0.0.1"
#define PORT 8080

#define ERR_AND_EXIT(msg) \
    perror(msg); \
    exit(errno);

void pipe_handler(int sig) {
    if (sig == SIGPIPE) {
        ; /* do nothing. */
    }
}

void receive_file(int client_socket, const char* name) {
    char buffer[BUFFER_SIZE];
    int n;
    unsigned char* img_data = NULL;
    size_t img_size = 0;
    ssize_t total_received = 0;

    // Set a timeout for the socket
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof timeout);

    while ((n = recv(client_socket, buffer, BUFFER_SIZE, 0)) > 0) {
        img_data = (unsigned char*)realloc(img_data, img_size + n);
        if (img_data == NULL) {
            ERR_AND_EXIT("Error in reallocating memory.");
        }
        memcpy(img_data + img_size, buffer, n);
        img_size += n;
        total_received += n;
        memset(buffer, 0, BUFFER_SIZE);
    }

    if (n < 0 && errno != EWOULDBLOCK) {
        perror("Error receiving file data");
        free(img_data);
        close(client_socket);
        return;
    }

    printf("Total bytes received: %zu\n", total_received);

    // Save the received image
    //printf("Client disconnected {%s}\n", name);
    handle_image_processing(img_data, img_size, name, client_socket);

    free(img_data);
}

int main() {
    int server_fd, client_fds[MAX_CLIENTS], max_fd, activity, i, valread;
    int opt = 1;
    struct sockaddr_in address;
    fd_set readfds, active_fd_set;
    char buffer[BUFFER_SIZE] = {0};

    if (signal(SIGPIPE, pipe_handler)) {
        ERR_AND_EXIT("Ignoring SIGPIPE")
    }

    /* master socket */
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        ERR_AND_EXIT("Creating server socket")
    }

    /* make the socket reusable */
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        ERR_AND_EXIT("Making socket reusable")
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    /* Assign localhost:8080 to the server socket and bind it to the endpoint. */
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        ERR_AND_EXIT("Binding the server socket to localhost:8080")
    }

    /* Allow a maximum of 1024 clients connected at once (that is, FD_SETSIZE) */
    if (listen(server_fd, FD_SETSIZE) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port 8080\n");

    /* For safety reasons, initialise properly the active clients array. */
    for (i = 0; i < MAX_CLIENTS; i++) {
        client_fds[i] = 0;
    }

    /* Initialise the active fd set. */
    FD_ZERO(&active_fd_set);
    /* Add the server master socket to it. */
    FD_SET(server_fd, &active_fd_set);

    while (true) {
        readfds = active_fd_set; /* At each time, we clean up this */
        /* Thus, we need to work with an actual copy of it. */
        if (select(FD_SETSIZE, &readfds, NULL, NULL, NULL) < 0) {
            ERR_AND_EXIT("Monitoring server activity")
        }

        /* If master socket is active, ie. 
           it received a connection request from a client, accept the client
           and add it to the active fd set! */
        if (FD_ISSET(server_fd, &readfds)) {
            int new_socket;
            socklen_t addr_len = sizeof(address);
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, &addr_len)) < 0) {
                ERR_AND_EXIT("Accepting a new client")
            }

            if (admin_fd == -1) {
                admin_fd = new_socket;
                printf("Admin connected {fd=%d; endpoint=%s:%d}\n", new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));
            } else {
                printf("Client %d {fd=%d; endpoint=%s:%d}\n", clientNo, new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));
                send(admin_fd, "New client wants to connect. Accept? (y/n): ", strlen("New client wants to connect. Accept? (y/n): "), 0);
                
                char response[2];
                if (recv(admin_fd, response, 2, 0) <= 0) {
                    printf("Error receiving response from admin.\n");
                    close(new_socket);
                    continue;
                }

                response[1] = '\0'; // ensure null-terminated string
                printf("Admin response: %s\n", response);

                if (response[0] != 'y') {
                    printf("Admin rejected the connection.\n");
                    close(new_socket);
                    send(new_socket, "n", 1, 0);
                    continue;
                }

                client_fds[clientNo] = new_socket; /* store the new socket */
                clientNo++; /* new client was connected. */

                /* Client is connected. Add it to the set! */
                FD_SET(new_socket, &active_fd_set);
                send(new_socket, "y", 1, 0);
            }
        }

        for (i = 0; i < MAX_CLIENTS; i++) {
            int client_fd = client_fds[i];
            if (client_fd > 0 && FD_ISSET(client_fd, &readfds)) {
                if ((valread = read(client_fd, buffer, BUFFER_SIZE)) == 0) {
                    struct sockaddr_in client_address;
                    socklen_t addr_len = sizeof(client_address);
                    if (getpeername(client_fd, (struct sockaddr*)&client_address, &addr_len) < 0) {
                        ERR_AND_EXIT("Fetching client information")
                    }
                    printf("Client disconnected {%s:%d}\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
                    close(client_fd);
                    FD_CLR(client_fd, &active_fd_set);
                    client_fds[i] = 0;
                    clientNo--;
                } else {
                    buffer[valread] = '\0';
                    printf("Received message from client %d: %s\n", i, buffer);

                    if (strstr(buffer, "name:")) {
                        char name[100];
                        sscanf(buffer, "name:%s", name);
                        receive_file(client_fd, name);
                    } else {
                        if (send(client_fd, buffer, strlen(buffer), 0) < 0) {
                            ERR_AND_EXIT("Sending message to the client")
                        }
                    }
                }
            }
        }
    }

    return 0;
}
