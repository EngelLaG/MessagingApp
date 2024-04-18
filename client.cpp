#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <thread>

#pragma comment(lib, "Ws2_32.lib")

using namespace std;

void receiveMessages(SOCKET sock) {
    char buffer[1024];
    int result;
    while (true) {
        result = recv(sock, buffer, sizeof(buffer), 0);
        if (result > 0) {
            buffer[result] = '\0'; // Null-terminate the buffer
            cout << buffer << endl;  // Display the message directly
        } else if (result == 0) {
            cout << "Server disconnected." << endl;
            break;
        } else {
            cout << "recv failed with error: " << WSAGetLastError() << endl;
            break;
        }
    }
}

void sendMessage(SOCKET sock) {
    string input;
    while (true) {
        getline(cin, input);
        if (input == "#exit") {
            send(sock, input.c_str(), input.size() + 1, 0); // Send exit command to the server
            cout << "Disconnecting from server..." << endl;
            break;
        }
        send(sock, input.c_str(), input.size() + 1, 0);
    }
}

int main() {
    WSADATA WSAData;
    SOCKET sock;
    SOCKADDR_IN serverAddr;
    const char* ip = "127.0.0.1";
    int port = 1500;

    WSAStartup(MAKEWORD(2, 2), &WSAData);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        cout << "Socket creation failed with error: " << WSAGetLastError() << endl;
        WSACleanup();
        return 1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &serverAddr.sin_addr);

    if (connect(sock, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cout << "Failed to connect to server: " << WSAGetLastError() << endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }
    cout << "Connected to server! Type #exit to disconnect." << endl;

    thread receiveThread(receiveMessages, sock);
    thread sendThread(sendMessage, sock);

    sendThread.join(); // Wait for the send thread to finish on user request to exit
    receiveThread.detach(); // Detach the receive thread since it might be blocking on recv()

    closesocket(sock);
    WSACleanup();
    return 0;
}
