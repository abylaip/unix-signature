#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/select.h>

#define PORT 8080
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

pthread_mutex_t file_mutex;

// Function to handle client connections
void *handle_client(void *client_socket);

int main() {
    int server_fd, new_socket; //
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    pthread_t tid[MAX_CLIENTS];
    int client_sockets[MAX_CLIENTS] = {0};
    fd_set readfds;
    int max_sd, sd, activity, i;

    // Create socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options to reuse address and port
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    // Set up server address structure
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind the socket to the address and port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Start listening for incoming connections
    if (listen(server_fd, 3) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    // Initialize the file mutex
    pthread_mutex_init(&file_mutex, NULL);

    printf("Server listening on port %d\n", PORT);

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        max_sd = server_fd;

        // Add active client sockets to the set
        for (i = 0; i < MAX_CLIENTS; i++) {
            sd = client_sockets[i];
            if (sd > 0)
                FD_SET(sd, &readfds);
            if (sd > max_sd)
                max_sd = sd;
        }

        // Wait for activity on one of the sockets
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        // Check if there's a new connection
        if (FD_ISSET(server_fd, &readfds)) {
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
                perror("accept failed");
                exit(EXIT_FAILURE);
            }

            printf("New connection: socket fd is %d, ip is: %s, port: %d\n", new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));

            // Add the new socket to the list of client sockets
            for (i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] == 0) {
                    client_sockets[i] = new_socket;
                    // Create a new thread for the client
                    pthread_create(&tid[i], NULL, handle_client, (void*)&client_sockets[i]);
                    break;
                }
            }
        }
    }

    // Destroy the file mutex
    pthread_mutex_destroy(&file_mutex);
    return 0;
}

// Function to handle individual client connections
void *handle_client(void *client_socket) {
    int sock = *(int*)client_socket;
    char buffer[BUFFER_SIZE];
    int bytes_read;

    // Read client command
    if ((bytes_read = read(sock, buffer, BUFFER_SIZE)) > 0) {
        buffer[bytes_read] = '\0';
        printf("Client command: %s\n", buffer);

        char command[BUFFER_SIZE], filename[BUFFER_SIZE];
        sscanf(buffer, "%s %s", command, filename);

        filename[strcspn(filename, "\r\n")] = 0;

        // Handle file upload
        if (strcmp(command, "UPLOAD") == 0) {
            FILE *file = fopen(filename, "wb");
            if (file == NULL) {
                perror("File open error");
                close(sock);
                return NULL;
            }

            while ((bytes_read = read(sock, buffer, BUFFER_SIZE)) > 0) {
                fwrite(buffer, 1, bytes_read, file);
            }

            fclose(file);
            printf("File uploaded successfully\n");
        }
        // Handle file download
        else if (strcmp(command, "DOWNLOAD") == 0) {
            FILE *file = fopen(filename, "rb");
            if (file == NULL) {
                perror("File open error");
                close(sock);
                return NULL;
            }

            while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
                send(sock, buffer, bytes_read, 0);
            }

            fclose(file);
            printf("File downloaded successfully\n");
        } else {
            printf("Invalid command\n");
        }
    }

    // Close the client socket
    close(sock);
    return NULL;
}
