#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

// Function to handle file upload
void upload_file(int sock);

// Function to handle file download
void download_file(int sock);

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    // Set up server address structure
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 address from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

    char choice;
    printf("Enter 'u' to upload file or 'd' to download file: ");
    scanf(" %c", &choice);

    // Handle user's choice for upload or download
    if (choice == 'u') {
        upload_file(sock);
    } else if (choice == 'd') {
        download_file(sock);
    } else {
        printf("Invalid choice\n");
    }

    // Close the socket
    close(sock);
    return 0;
}

// Function to upload file to the server
void upload_file(int sock) {
    char filename[BUFFER_SIZE];
    printf("Enter the filename to upload: ");
    scanf("%s", filename);

    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        perror("File open error");
        return;
    }

    // Send upload command to the server
    char command[BUFFER_SIZE];
    snprintf(command, sizeof(command), "UPLOAD %s", filename);
    send(sock, command, strlen(command), 0);

    char buffer[BUFFER_SIZE];
    int bytes_read;

    // Read file and send data to the server
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        send(sock, buffer, bytes_read, 0);
    }

    fclose(file);
    printf("File uploaded successfully\n");
}

// Function to download file from the server
void download_file(int sock) {
    char filename[BUFFER_SIZE];
    printf("Enter the filename to download: ");
    scanf("%s", filename);

    // Send download command to the server
    char command[BUFFER_SIZE];
    snprintf(command, sizeof(command), "DOWNLOAD %s", filename);
    send(sock, command, strlen(command), 0);

    FILE *file = fopen(filename, "wb");
    if (file == NULL) {
        perror("File open error");
        return;
    }

    char buffer[BUFFER_SIZE];
    int bytes_received;

    // Receive data from the server and write to file
    while ((bytes_received = recv(sock, buffer, BUFFER_SIZE, 0)) > 0) {
        fwrite(buffer, 1, bytes_received, file);
    }

    fclose(file);
    printf("File downloaded successfully\n");
}
