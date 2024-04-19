#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <thread>

#pragma comment(lib, "Ws2_32.lib")

using namespace std;

SOCKET clientSocket = INVALID_SOCKET;
int shift = 3;  // Shift for Caesar cipher

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

string caesarDecrypt(const string& text, int s) {
    return caesarEncrypt(text, 26 - s); 
}

void broadcastToClient(const string& message) {
    if (clientSocket != INVALID_SOCKET) {
        string encrypted = caesarEncrypt(message, shift);
        send(clientSocket, encrypted.c_str(), encrypted.length(), 0);
    }
}

void clientHandler() {
    char buffer[1024];
    int result;

    while (true) {
        result = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (result > 0) {
            buffer[result] = '\0';
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
    clientSocket = INVALID_SOCKET;
}

void serverInputHandler() {
    string input;
    cout << "Server ready to send messages. Type 'exit' to stop the server." << endl;

    while (getline(cin, input)) {
        if (input == "exit") {
            broadcastToClient("Server is shutting down.");
            exit(0);
        }
        broadcastToClient("Server: " + input);
    }
}

int main() {
    WSADATA WSAData;
    SOCKET serverSocket;
    SOCKADDR_IN serverAddr;
    int port = 1500;

    WSAStartup(MAKEWORD(2, 2), &WSAData);
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        cout << "Socket creation failed with error: " << WSAGetLastError() << endl;
        WSACleanup();
        return 1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cout << "Bind failed with error: " << WSAGetLastError() << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    listen(serverSocket, 1);
    cout << "Listening for incoming connections..." << endl;

    clientSocket = accept(serverSocket, NULL, NULL);
    if (clientSocket == INVALID_SOCKET) {
        cout << "Accept failed with error: " << WSAGetLastError() << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    cout << "Client connected!" << endl;

    thread userInputThread(serverInputHandler);
    clientHandler();

    userInputThread.join();
    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
