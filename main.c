//
//  main.c
//  reactor
//
//  Created by Mehmet İNAÇ on 03/05/2017.
//  Copyright © 2017 Mehmet İNAÇ. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>

#define TRUE  1
#define FALSE 0
#define MESSAGE_SIZE 2000
#define LISTEN_PORT 8888
#define GET "GET"
#define POST "POST"
#define NOT_ALLOWED "HTTP/1.1 405\n"


#define TRUE  1
#define FALSE 0
#define MESSAGE_SIZE 2000
#define LISTEN_PORT 9003
#define MAX_NUM_OF_CLIENTS 3000
#define NEW_LINE "\r\n\r\n"

fd_set socks;
int high_sock;
int listen_fd;

void signal_handler(int sig) {
    puts("Handle sig");
    close(listen_fd);
    exit(0);
}

void exit_with_message(char message[]) {
    puts(message);
    exit(-1);
}

// TODO: puts log into file
void put_log(char log_message[]) {
    puts(log_message);
}

void time_consuming_job() {
    int i;
    usleep(10000); // sleep 1 ms
    for(i = 0; i < 10000000; i++) {}
}

int read_request(int client_fd) {
    char client_message[MESSAGE_SIZE];
    char content_length_char[] = "Content-Length: ";
    int keep_reading = TRUE, content_length = 0, tmp = 0, numbers[10], read_line = 0;
    char *pos, *p, *new_line;
    long i;
    
    while (keep_reading) {
        read_line = recv(client_fd, client_message , MESSAGE_SIZE - 1 , 0);
        
        if (read_line == 0) {
            return 0;
        }
        
        new_line  = strstr(client_message, NEW_LINE);
        
        if (new_line != NULL) {
            keep_reading = FALSE;
        }
        
        if (pos) {
            i = (pos - client_message);
            p = client_message + i;
            
            while (*p) {
                if (isdigit(*p)) {
                    int val = (int)strtol(p, &p, 10);
                    numbers[tmp] = val;
                    tmp++;
                } else {
                    p++;
                }
            }
            
            for (i = 0; i < tmp; i++) {
                content_length += pow(10, (tmp - i - 1)) * numbers[i];
            }
        }
    }
    
    if (content_length > 0) {
        printf("Content length is %d", content_length);
        //        int length = 0;
        //
        //        while (content_length > length) {
        //            length += recv(client_fd, client_message , MESSAGE_SIZE - 1 , 0);
        //            printf("I've read %d bytes", length);
        //        }
    }
    
    return 1;
}


int *mock_endpoint(int *client_fd_ptr) {
    int client_fd = *(int *) client_fd_ptr;

    read_request(client_fd);

    // Mock time consuming job
    time_consuming_job();

    char message[] = "HTTP/1.1 200\r\n\r\nOK\r\n";

    write(client_fd, message, strlen(message));

    close(client_fd);

    return 0;
}

int main(int argc, const char * argv[]) {
    fd_set read_fds;
    int client_fd, temp_fd, c, i, client_sockets[MAX_NUM_OF_CLIENTS], max_fd, activity;
    struct sockaddr_in server, client;
    pthread_t thread;

    // Register signal handler for graceful shutdown
    signal(SIGINT, signal_handler);

    // Clear the client_socket list
    for (i = 0; i < MAX_NUM_OF_CLIENTS; i++) {
        client_sockets[i] = 0;
    }

    listen_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (listen_fd == -1) {
        exit_with_message("Could not create socket!");
    }
    put_log("Socket has been created.");

    // Prepare server
    server.sin_family      = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port        = htons(LISTEN_PORT);

    if (bind(listen_fd, (struct sockaddr*)&server, sizeof(server)) < 0) {
        exit_with_message("Binding failure!");
    }
    put_log("Binding done.");

    listen(listen_fd, 100);
    put_log("Listening incoming request with backlog option set to 100.");

    while (TRUE) {
        // Clear the set
        FD_ZERO(&read_fds);

        // Add liste file descriptor into set to listen incoming requests
        FD_SET(listen_fd, &read_fds);
        max_fd = listen_fd;

        for (i = 0; i < MAX_NUM_OF_CLIENTS; i++) {
            temp_fd = client_sockets[i];

            if (temp_fd != 0) {
                FD_SET(client_sockets[i], &read_fds);
            }

            if (temp_fd > listen_fd) {
                max_fd = client_sockets[i];
            }
        }
        
        activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);

        if (activity == -1 && errno != EINTR) {
            put_log("Could not select from socket list");
        }

        // We got a new connection request
        if (FD_ISSET(listen_fd, &read_fds)) {
            client_fd = accept(listen_fd, (struct sockaddr *)&client, (socklen_t *)&c);

            if(client_fd < 0) {
                put_log("Could not accept connection!");
            } else {
                put_log("We got a new connection");

                for (i = 0; i < MAX_NUM_OF_CLIENTS; i++) {
                    if (client_sockets[i] == 0) {
                        client_sockets[i] = client_fd;
                        break;
                    }
                }
            }
        }

        // Now we can check other sockets
        for (i = 0; i < MAX_NUM_OF_CLIENTS; i++) {
            temp_fd = client_sockets[i];

            // We have data coming from this socket
            if (FD_ISSET(temp_fd , &read_fds)) {
                if (mock_endpoint(&temp_fd) == 0) {
                    client_sockets[i] = 0;
                }
            }
        }
    }

    return 0;
}
