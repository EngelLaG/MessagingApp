#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <thread>

#pragma comment(lib, "Ws2_32.lib")  // Links the Winsock library

using namespace std;

// Global variable to store the client socket
SOCKET clientSocket = INVALID_SOCKET;
int shift = 3;  // Shift for Caesar cipher

// Encrypts text using a Caesar cipher
string caesarEncrypt(const string& text, int s) {
    string result = "";
    for (auto c : text) {
        if (isalpha(c)) {  
            char base = islower(c) ? 'a' : 'A';  
            c = (c - base + s) % 26 + base;  
        }
        result += c;  
    }
    return result;
}

// Decrypts text using a Caesar cipher
string caesarDecrypt(const string& text, int s) {
    return caesarEncrypt(text, 26 - s); 
}

// Sends an encrypted message to the client
void broadcastToClient(const string& message) {
    if (clientSocket != INVALID_SOCKET) {
        string encrypted = caesarEncrypt(message, shift);
        send(clientSocket, encrypted.c_str(), encrypted.length(), 0);
    }
}

// Handles incoming messages from the client
void clientHandler() {
    char buffer[1024];
    int result;

    while (true) {
        result = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        buffer[result] = '\0';  // Ensure the string is null-terminated

        if (result > 0) {
            string decrypted = caesarDecrypt(buffer, shift);
            cout << "Client says: " << decrypted << endl;
        } else if (result == 0) {
            cout << "Client disconnected." << endl;
            break;
        } else {
            cout << "recv failed with error: " << WSAGetLastError() << endl;
            break;
        }
    }

    closesocket(clientSocket);
    clientSocket = INVALID_SOCKET; // Reset client socket to its initial state
}

// Handles server input to broadcast messages to the client
void serverInputHandler() {
    string input;
    cout << "Server ready to send messages. Type 'exit' to stop the server." << endl;

    while (getline(cin, input)) {
        if (input == "exit") {
            broadcastToClient("Server is shutting down.");
            exit(0);  // Exit the server application
        }
        broadcastToClient("Server: " + input);
    }
}

// Main function to set up the server
int main() {
    WSADATA WSAData;
    SOCKET serverSocket;
    SOCKADDR_IN serverAddr;
    int port = 1500;  // Server port

    WSAStartup(MAKEWORD(2, 2), &WSAData);  // Initialize Winsock

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);  // Create TCP server socket
    if (serverSocket == INVALID_SOCKET) {
        cout << "Socket creation failed with error: " << WSAGetLastError() << endl;
        WSACleanup();
        return 1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);  // Host to network short
    serverAddr.sin_addr.s_addr = INADDR_ANY;  // Use any available interface

    // Bind socket to the specified IP and port
    if (bind(serverSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cout << "Bind failed with error: " << WSAGetLastError() << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    listen(serverSocket, 1);  // Listen for incoming connections
    cout << "Listening for incoming connections..." << endl;

    // Accept a client socket
    clientSocket = accept(serverSocket, NULL, NULL);
    if (clientSocket == INVALID_SOCKET) {
        cout << "Accept failed with error: " << WSAGetLastError() << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    cout << "Client connected!" << endl;

    thread userInputThread(serverInputHandler);  // Thread for handling server input
    clientHandler();  // Main thread handles client communication

    userInputThread.join();  // Wait for input thread to finish
    closesocket(serverSocket);
    WSACleanup();  // Clean up Winsock
    return 0;
}
