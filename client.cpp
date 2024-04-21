#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <fstream>
#include <string>
#include <thread>

#pragma comment(lib, "Ws2_32.lib")  // Link with the Windows Socket library

using namespace std;

char key = 'K';  // Hard-coded key for XOR encryption

// Encrypts or decrypts text using XOR cipher
string xorEncryptDecrypt(const string& text, char key) {
    string result = text;
    for (size_t i = 0; i < text.size(); ++i) {
        result[i] ^= key;  // Perform XOR with the key
    }
    return result;
}

// Thread function to receive messages from the server
void receiveMessages(SOCKET sock) {
    char buffer[1024];
    int result;
    while (true) {
        result = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (result > 0) {
            buffer[result] = '\0';  // Null-terminate the string
            string decrypted = xorEncryptDecrypt(buffer, key);
            cout << decrypted << endl;
        } else if (result == 0) {
            cout << "Server disconnected." << endl;
            break;
        } else {
            cout << "recv failed with error: " << WSAGetLastError() << endl;
            break;
        }
    }
}

// Thread function to send messages to the server
void sendMessage(SOCKET sock) {
    string input;
    while (true) {
        getline(cin, input);
        if (input == "#exit") {
            string encrypted = xorEncryptDecrypt(input, key);
            send(sock, encrypted.c_str(), encrypted.size() + 1, 0);
            cout << "Disconnecting from server..." << endl;
            break;
        }
        string encrypted = xorEncryptDecrypt(input, key);
        send(sock, encrypted.c_str(), encrypted.size() + 1, 0);
    }
}

// Registers a new account by writing to a file
bool registerAccount() {
    string username, password, line, encryptedUser, encryptedPass;
    cout << "Enter new username: ";
    getline(cin, username);
    cout << "Enter new password: ";
    getline(cin, password);

    encryptedUser = xorEncryptDecrypt(username, key);
    encryptedPass = xorEncryptDecrypt(password, key);

    ifstream check("accounts.txt");
    while (getline(check, line)) {
        size_t pos = line.find(' ');
        if (pos != string::npos) {
            string storedUser = line.substr(0, pos);
            storedUser = xorEncryptDecrypt(storedUser, key);
            if (storedUser == username) {
                cout << "Username already exists. Please try logging in." << endl;
                check.close();
                return false;
            }
        }
    }
    check.close();

    ofstream file("accounts.txt", ios::app);
    if (!file.is_open()) {
        cout << "Failed to open accounts file." << endl;
        return false;
    }
    file << encryptedUser << " " << encryptedPass << endl;
    file.close();
    return true;
}

// Authenticates a user against entries in a file
bool authenticate() {
    string username, password, line, encryptedUser, encryptedPass;
    cout << "Enter username: ";
    getline(cin, username);
    cout << "Enter password: ";
    getline(cin, password);

    encryptedUser = xorEncryptDecrypt(username, key);
    encryptedPass = xorEncryptDecrypt(password, key);

    ifstream file("accounts.txt");
    while (getline(file, line)) {
        size_t pos = line.find(' ');
        if (pos != string::npos) {
            string storedUser = line.substr(0, pos);
            string storedPass = line.substr(pos + 1);
            if (storedUser == encryptedUser && storedPass == encryptedPass) {
                file.close();
                return true;
            }
        }
    }
    file.close();
    cout << "Invalid username or password." << endl;
    return false;
}

// Main function: sets up connection and threads
int main() {
    cout << "1. Register\n2. Login\nChoose option: ";
    string option;
    getline(cin, option);

    if (option == "1") {
        if (!registerAccount()) {
            cout << "Registration failed." << endl;
            return 1;
        }
        cout << "Registration successful. Please login." << endl;
    }

    if (!authenticate()) {
        return 1;
    }

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

    sendThread.join();  // Wait for send thread to finish
    receiveThread.detach();  // Detach receive thread

    closesocket(sock);
    WSACleanup();
    return 0;
}
