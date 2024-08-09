#include <stdio.h>              // Include standard I/O library for input and output operations
#include <stdlib.h>             // Include standard library for memory management, random number generation, etc.
#include <string.h>             // Include string library for handling string operations
#include <unistd.h>             // Include POSIX operating system API for miscellaneous functions like close()
#include <arpa/inet.h>          // Include definitions for internet operations, like converting IP addresses
#include <pthread.h>            // Include library for creating and managing threads
#include <sys/select.h>         // Include library for monitoring multiple file descriptors

#define PORT 8080               // Define the port number where the server will listen
#define MAX_CLIENTS 10          // Define the maximum number of clients that can connect simultaneously
#define BUFFER_SIZE 1024        // Define the size of the buffer for data transfer

pthread_mutex_t file_mutex;     // Declare a mutex for synchronizing file access across multiple threads

// Function declaration for handling client connections
void *handle_client(void *client_socket);

int main() {
    int server_fd, new_socket;  // Declare variables for the server and new client socket file descriptors
    struct sockaddr_in address; // Structure to hold server address information
    int opt = 1;                // Option for setting socket options
    int addrlen = sizeof(address); // Length of the address structure
    pthread_t tid[MAX_CLIENTS]; // Array to hold thread identifiers for each client
    int client_sockets[MAX_CLIENTS] = {0}; // Array to keep track of active client sockets
    fd_set readfds;             // File descriptor set for select function
    int max_sd, sd, activity, i; // Variables for managing file descriptors and loop control

    // Create socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed"); // Print error message if socket creation fails
        exit(EXIT_FAILURE);      // Exit the program with failure status
    }

    // Set socket options to reuse address and port
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt failed"); // Print error message if setting socket options fails
        exit(EXIT_FAILURE);          // Exit the program with failure status
    }

    // Set up server address structure
    address.sin_family = AF_INET;           // Set address family to IPv4
    address.sin_addr.s_addr = INADDR_ANY;   // Allow the server to accept connections on any IP address
    address.sin_port = htons(PORT);         // Set the port number, converting to network byte order

    // Bind the socket to the address and port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");  // Print error message if binding fails
        exit(EXIT_FAILURE);     // Exit the program with failure status
    }

    // Start listening for incoming connections
    if (listen(server_fd, 3) < 0) {
        perror("listen failed"); // Print error message if listening fails
        exit(EXIT_FAILURE);      // Exit the program with failure status
    }

    // Initialize the file mutex
    pthread_mutex_init(&file_mutex, NULL); // Initialize the mutex to synchronize file access

    printf("Server listening on port %d\n", PORT); // Print a message indicating the server is listening

    while (1) { // Infinite loop to continuously accept and handle client connections
        FD_ZERO(&readfds);           // Clear the file descriptor set
        FD_SET(server_fd, &readfds); // Add the server socket to the file descriptor set
        max_sd = server_fd;          // Initialize max_sd with the server file descriptor

        // Add active client sockets to the set
        for (i = 0; i < MAX_CLIENTS; i++) {
            sd = client_sockets[i];  // Get the client socket descriptor
            if (sd > 0)              // If the client socket is active
                FD_SET(sd, &readfds); // Add it to the file descriptor set
            if (sd > max_sd)         // Update max_sd if necessary
                max_sd = sd;
        }

        // Wait for activity on one of the sockets
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL); // Monitor multiple file descriptors for events

        // Check if there's a new connection
        if (FD_ISSET(server_fd, &readfds)) { // If the server socket has activity
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
                perror("accept failed"); // Print error message if accepting a connection fails
                exit(EXIT_FAILURE);      // Exit the program with failure status
            }

            printf("New connection: socket fd is %d, ip is: %s, port: %d\n", new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port)); // Print connection details

            // Add the new socket to the list of client sockets
            for (i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] == 0) {        // Find an empty slot in the client sockets array
                    client_sockets[i] = new_socket;  // Add the new socket to the array
                    // Create a new thread for the client
                    pthread_create(&tid[i], NULL, handle_client, (void*)&client_sockets[i]); // Create a new thread to handle the client
                    break;                           // Exit the loop once the client is assigned
                }
            }
        }
    }

    // Destroy the file mutex
    pthread_mutex_destroy(&file_mutex); // Destroy the mutex after the server shuts down
    return 0;                           // Return 0 to indicate successful execution
}

// Function to handle individual client connections
void *handle_client(void *client_socket) {
    int sock = *(int*)client_socket;  // Dereference the client socket pointer to get the socket descriptor
    char buffer[BUFFER_SIZE];         // Buffer to hold the data received from the client
    int bytes_read;                   // Variable to hold the number of bytes read from the client

    // Read client command
    if ((bytes_read = read(sock, buffer, BUFFER_SIZE)) > 0) { // Read the client's command into the buffer
        buffer[bytes_read] = '\0';        // Null-terminate the string
        printf("Client command: %s\n", buffer); // Print the client's command

        char command[BUFFER_SIZE], filename[BUFFER_SIZE]; // Buffers to hold the command and filename
        sscanf(buffer, "%s %s", command, filename);       // Parse the command and filename from the buffer

        filename[strcspn(filename, "\r\n")] = 0; // Remove newline characters from the filename

        // Handle file upload
        if (strcmp(command, "UPLOAD") == 0) {  // If the command is "UPLOAD"
            FILE *file = fopen(filename, "wb"); // Open the file for writing in binary mode
            if (file == NULL) {                 // Check if the file could not be opened
                perror("File open error");      // Print error message if file opening fails
                close(sock);                    // Close the client socket
                return NULL;                    // Return NULL to exit the thread
            }

            while ((bytes_read = read(sock, buffer, BUFFER_SIZE)) > 0) { // Read data from the client
                fwrite(buffer, 1, bytes_read, file); // Write the data to the file
            }

            fclose(file);                       // Close the file after uploading is complete
            printf("File uploaded successfully\n"); // Print success message
        }
        // Handle file download
        else if (strcmp(command, "DOWNLOAD") == 0) { // If the command is "DOWNLOAD"
            FILE *file = fopen(filename, "rb"); // Open the file for reading in binary mode
            if (file == NULL) {                 // Check if the file could not be opened
                perror("File open error");      // Print error message if file opening fails
                close(sock);                    // Close the client socket
                return NULL;                    // Return NULL to exit the thread
            }

            while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) { // Read data from the file
                send(sock, buffer, bytes_read, 0); // Send the data to the client
            }

            fclose(file);                       // Close the file after downloading is complete
            printf("File downloaded successfully\n"); // Print success message
        } else {
            printf("Invalid command\n");        // Print error message if the command is invalid
        }
    }

    // Close the client socket
    close(sock);  // Close the socket after communication is complete
    return NULL;  // Return NULL to exit the thread
}
