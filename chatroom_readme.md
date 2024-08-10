Simple Chatroom Server
How it works:

Overview:
The chat server is used for communication between multiple clients in a chat room. Upon running, the server creates a socket, binds it to a specified port, and listens for incoming connections. When a client connects, the server creates a new thread to handle the client's communication. Clients can send messages to the server, which are then broadcasted to all other connected clients. Additionally, clients can use special commands like `\list` to view online users and `\bye` to exit the chat. The server manages client connections, usernames, and timeouts.

1. **Main Functionality Overview:**
   - The `main` function initializes the server by creating a socket, binding it to a port, and listening for incoming connections, i.e., a welcoming socket.
   - It then accepts incoming connections and creates a new thread to handle each client's communication.

2. **Creating Server Socket (`create_server_socket`):**
   - This function creates and configures the server socket for accepting client connections.
   - It binds the socket to the specified port and starts listening for incoming connections.

3. **Handling Client Communication (`handle_client`):**
   - Each client connection is handled in a separate thread by the `handle_client` function.
   - Upon connection, the client is prompted to enter a username.
   - The server manages client usernames and broadcasts messages to all connected clients.
   - Clients can use special commands like `\list` to view online users and `\bye` to exit the chat.

4. **Client Management:**
   - Clients are represented by the `client_info` structure, which stores their socket file descriptor and username.
   - The server maintains an array of client information to manage connections and usernames.

5. **Thread Management:**
   - The server creates a new thread for each client connection using POSIX threads (`pthread_create`).
   - Threads are run independently, and client handling is done concurrently.

6. **Timeout:**
   - It sets a timeout for client connections, after which inactive clients are disconnected.

Note:
- Ensure to specify appropriate values for `<max_clients>` and `<timeout>` based on your server's capacity and requirements.