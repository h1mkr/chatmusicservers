MP3 Streaming Server
How it works:

Overview:
1. The server listens for incoming connections on the specified port.
2. When a client connects, the server accepts the connection and creates a new thread to handle the client's request.
3. The client sends the index of the MP3 file it wants to stream.
4. The server reads the MP3 file from the specified directory and streams it back to the client.
5. Upon completion, the server closes the connection with the client.

1. **Main Functionality Overview:**
   - The `main` function initializes the server by creating a socket, binding it to a port, and listening for incoming connections.
   - It then accepts incoming connections and creates a new thread to handle each client's request.

2. **Handle Client Function (`handle_client`):**
   - This function is executed in a separate thread for each client.
   - It receives the client's request for an MP3 file, opens the requested file from the specified directory, and streams its contents back to the client.
   - If any errors occur during this process, it logs the error and terminates the connection with the client.

3. **Accepting Client Connections:**
   - In the main loop of the `main` function, the server accepts incoming connections using the `accept` system call.
   - Upon accepting a connection, it creates a new thread to handle the client's request using the `pthread_create` function.

4. **Thread Creation and Management:**
   - Each time a new client connects, a new thread is created using `pthread_create`.
   - The server limits the number of simultaneous client connections based on the `max_clients` parameter.
   - Threads are joined in the main function after all client connections have been handled.

5. **Sending MP3 Files to Clients:**
   - Upon receiving a client request, the server reads the requested MP3 file from the specified directory.
   - It then sends the contents of the MP3 file to the client using the `send` function in a loop until the entire file is transmitted.

Note:
- Ensure that the specified directory contains MP3 files with filenames "0.mp3", "1.mp3", ..., "9.mp3" for the provided code to work correctly.