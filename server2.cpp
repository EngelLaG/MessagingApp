#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <thread>

#pragma comment(lib, "Ws2_32.lib")  // Links the Winsock library

using namespace std;

char key = 'K';  // Hard-coded key for XOR encryption

SOCKET clientSockets[2] = { INVALID_SOCKET, INVALID_SOCKET };  // Array to store client sockets (2 Users)

// Encrypts or decrypts text using XOR cipher
string xorEncryptDecrypt(const string& text, char key) {
    string result = text;
    for (size_t i = 0; i < text.size(); ++i) {
        result[i] ^= key;  // Perform XOR with the key
    }
    return result;
}

// Broadcasts messages from one client to another
void broadcastMessage(const string& message, int senderIndex) {
    for (int i = 0; i < 2; ++i) {
        if (i != senderIndex && clientSockets[i] != INVALID_SOCKET) {
            string encrypted = xorEncryptDecrypt(message, key);
            send(clientSockets[i], encrypted.c_str(), encrypted.length(), 0);
        }
    }
}

// Handles incoming messages from a specific client
void clientHandler(SOCKET clientSocket, int clientIndex) {
    char buffer[1024];
    int result;

    while (true) {
        result = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (result > 0) {
            buffer[result] = '\0';  // Ensure the string is null-terminated
            string decrypted = xorEncryptDecrypt(string(buffer), key);
            cout << "Client " << clientIndex + 1 << " says: " << decrypted << endl;

            // Broadcast message to the other client
            broadcastMessage("Client " + to_string(clientIndex + 1) + " says: " + decrypted, clientIndex);
        } else if (result == 0) {
            cout << "Client " << clientIndex + 1 << " disconnected." << endl;
            break;
        } else {
            cout << "recv failed with error: " << WSAGetLastError() << endl;
            break;
        }
    }

    closesocket(clientSocket);
    clientSockets[clientIndex] = INVALID_SOCKET; // Reset client socket to its initial state
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

    if (bind(serverSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cout << "Bind failed with error: " << WSAGetLastError() << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    listen(serverSocket, 2);  // Listen for up to 2 incoming connections
    cout << "Listening for incoming connections..." << endl;

    int clientCount = 0;
    while (clientCount < 2) {
        SOCKET clientSocket = accept(serverSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) {
            cout << "Accept failed with error: " << WSAGetLastError() << endl;
            continue;
        }

        clientSockets[clientCount] = clientSocket;
        cout << "Client " << clientCount + 1 << " connected!" << endl;
        thread(clientHandler, clientSocket, clientCount).detach();  // Handle each client in a separate thread
        clientCount++;
    }

    // Simple command interface for the server
    string input;
    while (getline(cin, input)) {
        if (input == "exit") {
            break;
        }
    }

    // Close all client sockets
    for (int i = 0; i < 2; i++) {
        if (clientSockets[i] != INVALID_SOCKET) {
            closesocket(clientSockets[i]);
        }
    }
    closesocket(serverSocket);
    WSACleanup();  // Clean up Winsock
    return 0;
}
