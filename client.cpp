#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <fstream>
#include <string>
#include <thread>

#pragma comment(lib, "Ws2_32.lib")

using namespace std;

int shift = 3;  // Shift for Caesar cipher

// Function declarations
void receiveMessages(SOCKET sock);
void sendMessage(SOCKET sock);

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

void receiveMessages(SOCKET sock) {
    char buffer[1024];
    int result;
    while (true) {
        result = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (result > 0) {
            buffer[result] = '\0';
            string decrypted = caesarDecrypt(buffer, shift);
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

void sendMessage(SOCKET sock) {
    string input;
    while (true) {
        getline(cin, input);
        if (input == "#exit") {
            string encrypted = caesarEncrypt(input, shift);
            send(sock, encrypted.c_str(), encrypted.size() + 1, 0);
            cout << "Disconnecting from server..." << endl;
            break;
        }
        string encrypted = caesarEncrypt(input, shift);
        send(sock, encrypted.c_str(), encrypted.size() + 1, 0);
    }
}

bool registerAccount() {
    string username, password, line, userInFile;
    cout << "Enter new username: ";
    getline(cin, username);
    cout << "Enter new password: ";
    getline(cin, password);

    ifstream check("accounts.txt");
    while (getline(check, line)) {
        size_t pos = line.find(' ');
        if (pos != string::npos) {
            userInFile = line.substr(0, pos);
            if (userInFile == username) {
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
    file << username << " " << caesarEncrypt(password, shift) << endl;
    file.close();
    return true;
}

bool authenticate() {
    string username, password, line, userInFile, passInFile;
    cout << "Enter username: ";
    getline(cin, username);
    cout << "Enter password: ";
    getline(cin, password);

    ifstream file("accounts.txt");
    while (getline(file, line)) {
        size_t pos = line.find(' ');
        if (pos != string::npos) {
            userInFile = line.substr(0, pos);
            passInFile = line.substr(pos + 1);
            if (userInFile == username && caesarDecrypt(passInFile, shift) == password) {
                file.close();
                return true;
            }
        }
    }
    file.close();
    cout << "Invalid username or password." << endl;
    return false;
}

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

    sendThread.join();
    receiveThread.detach();

    closesocket(sock);
    WSACleanup();
    return 0;
}
