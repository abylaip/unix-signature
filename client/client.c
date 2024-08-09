#include <stdio.h>          // Include standard I/O library for input and output operations
#include <stdlib.h>         // Include standard library for memory management, random number generation, etc.
#include <string.h>         // Include string library for handling string operations
#include <unistd.h>         // Include POSIX operating system API for miscellaneous functions like close()
#include <arpa/inet.h>      // Include definitions for internet operations, like converting IP addresses

#define PORT 8080           // Define the port number where the server will listen
#define BUFFER_SIZE 1024    // Define the size of the buffer for data transfer

// Function declaration for uploading a file to the server
void upload_file(int sock);

// Function declaration for downloading a file from the server
void download_file(int sock);

int main() {
    int sock = 0;                          // Initialize socket descriptor
    struct sockaddr_in serv_addr;          // Structure to hold server address information

    // Create a socket using IPv4 and TCP (SOCK_STREAM) protocol
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");  // Print error message if socket creation fails
        return -1;                               // Return with error code -1
    }

    // Set up server address structure
    serv_addr.sin_family = AF_INET;              // Set address family to IPv4
    serv_addr.sin_port = htons(PORT);            // Set the port number, converting to network byte order

    // Convert IPv4 address from text to binary form and store in server address structure
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");  // Print error if address conversion fails
        return -1;                                              // Return with error code -1
    }

    // Connect the socket to the server using the server address structure
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");   // Print error message if connection fails
        return -1;                          // Return with error code -1
    }

    char choice;                            // Variable to store user's choice
    printf("Enter 'u' to upload file or 'd' to download file: ");  // Prompt user for action
    scanf(" %c", &choice);                  // Read user's choice

    // Handle user's choice to either upload or download a file
    if (choice == 'u') {
        upload_file(sock);                  // Call the function to upload a file
    } else if (choice == 'd') {
        download_file(sock);                // Call the function to download a file
    } else {
        printf("Invalid choice\n");         // Print error message if the choice is invalid
    }

    // Close the socket after operation is complete
    close(sock);
    return 0;                               // Return 0 to indicate successful execution
}

// Function to upload a file to the server
void upload_file(int sock) {
    char filename[BUFFER_SIZE];             // Buffer to store the filename
    printf("Enter the filename to upload: ");  // Prompt user to enter the filename
    scanf("%s", filename);                  // Read the filename from the user

    FILE *file = fopen(filename, "rb");     // Open the file in binary read mode
    if (file == NULL) {                     // Check if the file could not be opened
        perror("File open error");          // Print error message if file opening fails
        return;                             // Return without proceeding further
    }

    // Prepare the command string to be sent to the server
    char command[BUFFER_SIZE];
    snprintf(command, sizeof(command), "UPLOAD %s", filename);  // Format the command as "UPLOAD filename"
    send(sock, command, strlen(command), 0);                    // Send the command to the server

    char buffer[BUFFER_SIZE];             // Buffer to hold file data
    int bytes_read;                       // Variable to hold the number of bytes read from the file

    // Read the file data and send it to the server
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        send(sock, buffer, bytes_read, 0);  // Send the data to the server
    }

    fclose(file);                          // Close the file after sending is complete
    printf("File uploaded successfully\n"); // Print success message
}

// Function to download a file from the server
void download_file(int sock) {
    char filename[BUFFER_SIZE];              // Buffer to store the filename
    printf("Enter the filename to download: ");  // Prompt user to enter the filename
    scanf("%s", filename);                   // Read the filename from the user

    // Prepare the command string to be sent to the server
    char command[BUFFER_SIZE];
    snprintf(command, sizeof(command), "DOWNLOAD %s", filename);  // Format the command as "DOWNLOAD filename"
    send(sock, command, strlen(command), 0);                      // Send the command to the server

    FILE *file = fopen(filename, "wb");       // Open the file in binary write mode
    if (file == NULL) {                       // Check if the file could not be opened
        perror("File open error");            // Print error message if file opening fails
        return;                               // Return without proceeding further
    }

    char buffer[BUFFER_SIZE];               // Buffer to hold the received file data
    int bytes_received;                     // Variable to hold the number of bytes received from the server

    // Receive the file data from the server and write it to the file
    while ((bytes_received = recv(sock, buffer, BUFFER_SIZE, 0)) > 0) {
        fwrite(buffer, 1, bytes_received, file);  // Write the data to the file
    }

    fclose(file);                            // Close the file after receiving is complete
    printf("File downloaded successfully\n"); // Print success message
}
