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

int create_socket()
{ /* Create the socket descriptor */
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    return fd;
}

static void errAndExit(char * const log)
{
    perror(log);
    exit(errno);
}

void connect_socket(int sd, struct sockaddr_in addr, int poll_rate)
{
    /* Connect to the desired address */
    while (connect(sd, (const struct sockaddr *) & addr, (socklen_t) sizeof (addr)) < 0)
    { /* if connect fails come here, print a message and sleep for poll_rate seconds */
        fprintf(stdout, "[INFO] Still connecting...\n");
        sleep(poll_rate);
    }
}

int create_client_socket()
{
    int fd = create_socket();

    struct sockaddr_in addr;

    memset(&addr, 0, sizeof(struct sockaddr_in));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ADDR);
    addr.sin_port = htons(PORT);

    connect_socket(fd, addr, 1); // try to connect every 1 sec until server becomes available

    fprintf(stdout, "[INFO]\tSocket connected.\n");

    return fd;
}

char * get_message(int sd)
{
    char * recval = (char *) malloc(1024);
    if (recval == NULL) {
        errAndExit("allocating memory for message");
    }

    int bytes_received = recv(sd, (void *) recval, 1024, 0);
    if (bytes_received < 0)
    {
        free(recval);
        errAndExit("reading message from socket");
    }

    recval[bytes_received] = '\0'; // ensure null-terminated string
    return recval;
}

void send_message(int sd, char * bf)
{
    if (send(sd, (const void *) bf, strlen(bf), 0) < 0)
    {
        errAndExit("sending message on socket");
    }
}

int main(int argc, char * argv[])
{
    int fd_client;

    fd_client = create_client_socket();

    while (1)
    {
        char * message = get_message(fd_client);
        fprintf(stdout, "[ADMIN] Server message: %s\n", message);

        char response[2];
        printf("[ADMIN] Accept connection (y/n): ");
        scanf("%s", response);

        send_message(fd_client, response);
    }

    if (close(fd_client) < 0)
    {
        errAndExit("closing client socket");
    }

    exit(0);
}
