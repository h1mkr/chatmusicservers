#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <pthread.h>

#define BUFFER_SIZE 1024
#define MAX_CLIENTS 100
char* directory = NULL;

// Structure to hold client information
struct client_info {
    int socket;
    struct sockaddr_in address;
};

// Function to handle client requests
void *handle_client(void *arg) 
{
    // List of available MP3 files
    const char *file_names[10] = 
    {
        "0.mp3",
        "1.mp3",
        "2.mp3",
        "3.mp3",
        "4.mp3",
        "5.mp3",
        "6.mp3",
        "7.mp3",
        "8.mp3",
        "9.mp3"
    };
    
    // Casting the argument to client_info structure
    struct client_info *client = (struct client_info *)arg;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;
    
    // Receiving data from the client
    bytes_received = recv(client->socket, buffer, sizeof(buffer), 0);
    if (bytes_received <= 0) 
    {
        perror("Error receiving data");
        close(client->socket);
        free(client);
        pthread_exit(NULL);
    }
    printf("Client connected: %s:%d - Requested song: %s\n",inet_ntoa(client->address.sin_addr),ntohs(client->address.sin_port),buffer);

    // Extracting the index of the requested MP3 file
    int index = atoi(buffer);
    char* ret = malloc(strlen(directory) + strlen(file_names[index]) + 2);
    strcpy(ret, directory);
    strcat(ret, "/");
    strcat(ret, file_names[index]);
    
    // Opening the requested MP3 file
    FILE *mp3_file = fopen(ret, "rb");
    if (!mp3_file) 
    {
        perror("Error opening MP3 file");
        close(client->socket);
        free(client);
        pthread_exit(NULL);
    }

    // Sending the MP3 file to the client
    while ((bytes_received = fread(buffer, 1, sizeof(buffer), mp3_file)) > 0) 
    {
        if (send(client->socket, buffer, bytes_received, 0) != bytes_received) 
        {
            perror("Error sending data");
            fclose(mp3_file);
            close(client->socket);
            free(client);
            pthread_exit(NULL);
        }
    }
    fclose(mp3_file);
    close(client->socket);
    free(client);
    pthread_exit(NULL);
    free(ret);
}

// Main function
int main(int argc, char *argv[]) 
{
    // Check for correct number of command line arguments
    if (argc != 4) 
    {
        fprintf(stderr, "Usage: %s <port> <directory> <max_clients>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Extracting arguments from command line
    int port = atoi(argv[1]);
    directory = argv[2];
    int max_clients = atoi(argv[3]);

    // Creating the server socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) 
    {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // Binding the socket to the specified port
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) 
    {
        perror("Error binding socket");
        exit(EXIT_FAILURE);
    }

    // Listening for incoming connections
    if (listen(server_socket, max_clients) == -1) 
    {
        perror("Error listening for connections");
        exit(EXIT_FAILURE);
    }

    // Server is now listening
    printf("Server listening on port %d...\n", port);

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    pthread_t threads[MAX_CLIENTS];
    int num_clients = 0;

    // Accepting incoming connections
    while (1) 
    {
        if(num_clients == max_clients) 
        {
            printf("Maximum client limit reached. No more connections other than the current one will be accepted.\n");
            break;
        }
        else 
        {
            int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
            if (client_socket == -1) 
            {
                perror("Error accepting connection");
                continue;
            }

            struct client_info *client = malloc(sizeof(struct client_info));
            if (!client) 
            {
                perror("Error allocating memory for client info");
                close(client_socket);
                continue;
            }

            client->socket = client_socket;
            client->address = client_addr;

            // Creating a thread to handle the client request
            if (pthread_create(&threads[num_clients], NULL, handle_client, (void *)client) != 0) 
            {
                perror("Error creating thread for client");
                close(client_socket);
                free(client);
                continue;
            }
            num_clients++;
        }
        
    }
    // Closing the server socket
    close(server_socket);
    
    // Waiting for all client threads to finish
    for (int i = 0; i < num_clients; i++)
    {
        pthread_join(threads[i], NULL);
    }
    
    return 0;
}
