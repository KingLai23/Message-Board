#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <ctype.h>
#include <string.h>
#include <netinet/in.h>

int validPortNumber(char *str) {
    for (int i = 0; i < strlen(str); i++) {
        if (!isdigit(str[i])) return -1;
    }

    char *end;
    int port_number = strtol(str, &end, 10);

    if (port_number == 0 || port_number < 1 || port_number > 65535) {
        return -1;
    }

    return port_number;
}

int validInput(char *str) {
    int i = 0;
    while (str[i] != '\n') {
        i++;
        if (i > 75) {
            return -1;
        }
    }

    return i;
}

void myhandler(int sig) {}

int main(int argc, char *argv[]) {

    if (argc < 2) {
        exit(1);
    }

    int port_number = validPortNumber(argv[1]);

    if (port_number == -1) {
        exit(1);
    }

    sigaction(SIGPIPE, &(struct sigaction){{myhandler}}, NULL);

    int listen_soc = socket(AF_INET, SOCK_STREAM, 0);

    if (listen_soc == -1) {
        perror("socket");
        exit(1);
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_number);
    addr.sin_addr.s_addr = INADDR_ANY;
    memset(&(addr.sin_zero), 0, 8);

    if (bind(listen_soc, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1) {
        perror("bind");
        close(listen_soc);
        exit(1);
    }

    if (listen(listen_soc, 15) < 0) {
        perror("listen");
        exit(1);
    }

    fd_set current_socket, ready_sockets;

    FD_ZERO(&current_socket);
    FD_SET(listen_soc, &current_socket);

    char current_post[75];
    current_post[0] = '\n';
    char post_with_null[75];
    post_with_null[0] = '\0';

    while (1) {
        ready_sockets = current_socket;

        if (select(FD_SETSIZE, &ready_sockets, NULL, NULL, NULL) < 0) {
            perror("select");
            exit(1);
        }

        int result;
        int client_socket;

        for (int i = 0; i < FD_SETSIZE; i++) {
            if (FD_ISSET(i, &ready_sockets)) {
                if (i == listen_soc) {
                    struct sockaddr_in client_addr;
                    client_addr.sin_family = AF_INET;
                    unsigned int client_len = sizeof(struct sockaddr_in);

                    client_socket = accept(listen_soc, (struct sockaddr *)&client_addr, &client_len);

                    if (client_socket < 0) {
                        perror("accept");
                        exit(1);
                    }

                    FD_SET(client_socket, &current_socket);
                } else {
                    char buf[1024];

                    result = recv(i, buf, 1024, 0);

                    if (result > 0) {
                        int valid = validInput(buf);
                        if (valid != -1) {
                            if (buf[0] == '?' && valid == 1) {
                                write(i, current_post, strlen(post_with_null)+1);
                            } else if (buf[0] != '!') {
                                //printf("Invalid input.\n");
                            } else {
                                for (int i = 1; i < valid; i++) {
                                    current_post[i-1] = buf[i];
                                    post_with_null[i-1] = buf[i];
                                }
                                current_post[valid-1] = '\n';
                                post_with_null[valid-1] = '\0';
                                
                                //printf("Client changed the post to: %s\n", post_with_null);
                            }
                        } else {
                            //printf("Invalid input.\n");
                        }
                    } else {
                        FD_CLR(i, &current_socket);
                    }
                }
            }
        }
    }

    return 0;
}