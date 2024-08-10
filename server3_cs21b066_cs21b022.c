#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <pthread.h>

#define PORT 8080
#define MAX_CLIENTS 1000000
#define BUFFER_SIZE 1024
#define BUFFER_SIZE1 1024
#define BUFFER_SIZE2 2048

// Structure to hold client information
struct client_info 
{
    int socket;
    char username[BUFFER_SIZE];
};

// Global variables
int max_clients;
int timeout;
bool chat_active = true;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
struct client_info clients[MAX_CLIENTS];
int num_clients = 0;

// Function to create a server socket
int create_server_socket(int port);

// Thread function to handle each client
void *handle_client(void *arg);

// Function to send a message to all clients
void send_message_all_clients(const char *message, int sender_index);

// Function to add a new client
void add_client(int client_socket, const char *username);

// Function to remove a client
void remove_client(int client_index);

// Function to handle client commands
void handle_client_commands(const char *message, int client_index);

int main(int argc, char *argv[]) 
{
    // Check command-line arguments
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <port> <max_clients> <timeout>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Parse command-line arguments
    int port = atoi(argv[1]);
    max_clients = atoi(argv[2]);
    timeout = atoi(argv[3]);
    
    // Create server socket
    int server_socket = create_server_socket(port);

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    // Accept client connections
    while (chat_active) 
    {
        int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket == -1) 
        {
            perror("Error accepting connection");
            continue;
        }

        pthread_t thread;
        // Create a thread to handle each client
        if (pthread_create(&thread, NULL, handle_client, &client_socket) != 0) 
        {
            perror("Error creating thread for client");
            close(client_socket);
            continue;
        }
        pthread_detach(thread); 
    }
    close(server_socket);
    return EXIT_SUCCESS;
}

// Function to create a server socket
int create_server_socket(int port) 
{
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error binding socket");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, MAX_CLIENTS) == -1) {
        perror("Error listening for connections");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", port);

    return server_socket;
}

// Thread function to handle each client
void *handle_client(void *arg) 
{
    int client_socket = *(int *)arg;

    // Check if the maximum number of clients has been reached
    if(num_clients == max_clients)
    {
        char full_response[BUFFER_SIZE2];
        snprintf(full_response, sizeof(full_response), "[Max capacity reached. Please press enter to leave :)]");
        send(client_socket, full_response, strlen(full_response), 0);
        close(client_socket);
        pthread_exit(NULL);
    }
    else
    {
        // Request username from the client
        char request_username[BUFFER_SIZE2];
        snprintf(request_username, sizeof(request_username), "[Enter your username:] ");
        send(client_socket, request_username, strlen(request_username), 0);
        char username[BUFFER_SIZE1];

        memset(username, 0, BUFFER_SIZE);
        ssize_t bytes_received;

        bool flag = true;

        // Receive and validate username
        while(flag) 
        {
            bytes_received = recv(client_socket, username, sizeof(username), 0);
            if (bytes_received <= 0) 
            {
                perror("Error receiving username");
                close(client_socket);
                pthread_exit(NULL);
            }

            bool flag2 = false;
            for(int ii = 0; ii < num_clients; ii++) 
            {
                if(strcmp(clients[ii].username, username) == 0) 
                {
                    char request_username[BUFFER_SIZE2];
                    snprintf(request_username, sizeof(request_username), "[Username %s is already taken. Please choose another one]\n[Enter your username:] ", username);
                    send(client_socket, request_username, strlen(request_username), 0);
                    flag2 = true;
                }
            }
            if(flag2)
                continue;
            flag = false;
        }

        // Add client to the list
        int client_id = 0;
        pthread_mutex_lock(&mutex);
        clients[num_clients].socket = client_socket;
        strcpy(clients[num_clients].username, username);
        client_id = num_clients;
        num_clients++;
        pthread_mutex_unlock(&mutex);

        int client_sock = clients[client_id].socket;


        char welcome_message[BUFFER_SIZE2];
        snprintf(welcome_message, sizeof(welcome_message), "Welcome, %s!", username);
        send(client_socket, welcome_message, strlen(welcome_message), 0);
        memset(welcome_message, 0, BUFFER_SIZE2);

        char new_join_announcement[BUFFER_SIZE2];
        strcat(new_join_announcement, clients[client_id].username);
        strcat(new_join_announcement, " joined the chat");
        pthread_mutex_lock(&mutex);
        for (int i = 0; i < num_clients; i++) 
        {
            if (i != client_id) 
            {
                send(clients[i].socket, new_join_announcement, strlen(new_join_announcement), 0);
            }
        }
        memset(new_join_announcement, 0, BUFFER_SIZE2);
        pthread_mutex_unlock(&mutex);

        char buffer[BUFFER_SIZE];

        struct timeval to;  
        to.tv_sec=timeout;
        to.tv_usec=0;
        if(setsockopt(client_socket ,SOL_SOCKET,SO_RCVTIMEO,(const char*)&to,sizeof(to))<0)
        { 
            perror("Error setting socket timeout");
        }
        bool chk =false;
        while (1) 
        {
            if((bytes_received = recv(client_socket, buffer, sizeof(buffer), 0)) <= 0)
            {
                if(bytes_received<0)chk=true;
                break;
            }
            buffer[bytes_received] = '\0';
            handle_client_commands(buffer, client_sock); 
            memset(buffer, 0, BUFFER_SIZE);
        }
        if(chk) 
        {
            char timeout_message[BUFFER_SIZE2];
            snprintf(timeout_message, sizeof(timeout_message), "[Exiting due to timeout.]");
            send(client_socket, timeout_message, strlen(timeout_message), 0);
            memset(timeout_message, 0, BUFFER_SIZE2);
            close(client_socket);
        }
        pthread_exit(NULL);
    }
}



void remove_client(int client_sock) 
{
    pthread_mutex_lock(&mutex);
    close(client_sock);
    int client_index = 0;
    for (int i = 0; i < num_clients; i++)
    {
        if(clients[i].socket == client_sock)
        {
            client_index = i;
            break;
        }
    }

    for (int i = client_index; i < num_clients - 1; i++)
    {
        clients[i] = clients[i + 1];

    }
    num_clients--;
    printf("Client '%s' disconnected\n", clients[client_index].username);
    pthread_mutex_unlock(&mutex);
}

void handle_client_commands(const char *message, int client_sock) 
{
    if (strncmp(message, "\\list", 5) == 0) 
    {
        char list_message[BUFFER_SIZE];
        snprintf(list_message, sizeof(list_message), "Users currently online:\n");
        for (int i = 0; i < num_clients; i++) 
        {
            strcat(list_message, clients[i].username);
            strcat(list_message, "\n");
        }
        send(client_sock, list_message, strlen(list_message), 0);
        memset(list_message, 0, BUFFER_SIZE);
    }
    else if (strncmp(message, "\\bye", 4) == 0) 
    {
        send(client_sock, "Goodbye!\n", 9, 0);
        char user_left_announcement[BUFFER_SIZE2];
        
        for (int i = 0; i < num_clients; i++) 
        {
            if (clients[i].socket == client_sock) 
            {
                strcat(user_left_announcement, clients[i].username);
                strcat(user_left_announcement, " left the chat!");
                break; 
            }
        }

        pthread_mutex_lock(&mutex);
        for (int i = 0; i < num_clients; i++) 
        {
            if (clients[i].socket != client_sock) 
            {
                send(clients[i].socket, user_left_announcement, strlen(user_left_announcement), 0);
            }
        }

        
        pthread_mutex_unlock(&mutex);

        memset(user_left_announcement, 0, BUFFER_SIZE2);
        remove_client(client_sock);
    }
    else
    {
        char new_message[BUFFER_SIZE2];
        for (int i = 0; i < num_clients; i++) 
        {
            if (clients[i].socket == client_sock) 
            {
                strcat(new_message, clients[i].username);
                strcat(new_message, ": ");
                strcat(new_message, message); 
                break; 
            }
        }

        pthread_mutex_lock(&mutex);
        for (int i = 0; i < num_clients; i++) 
        {
            if (clients[i].socket != client_sock) 
            {
                send(clients[i].socket, new_message, strlen(new_message), 0);
            }
        }
        pthread_mutex_unlock(&mutex);
        memset(new_message, 0, BUFFER_SIZE2);
    }
}
