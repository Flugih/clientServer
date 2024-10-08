#define WIN32_LEAN_AND_MEAN
#pragma comment(lib, "WS2_32.lib")
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <string>
#include <windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <map>
#include <thread>
#include <mutex>
#include <queue>
#include <list>
#include <chrono>
#include <ctime>
#include <cmath>


using namespace std;

map<string, SOCKET> connectionsData;

class MessageDecryptor { // decrypt message
private:
	vector <string> hexadecimals;
	vector <int> decimals;
	short int shift = 0;
	string decryptMessage = "";

	void separation(string message) {
		string hexadecimalsString = "";
		decryptMessage = "";

		for (auto i : message) {
			if (i == '/') {
				hexadecimals.push_back(hexadecimalsString);
				hexadecimalsString = "";
			}
			else {
				hexadecimalsString += i;
			}
		}
		shift = stoi(hexadecimals.back());
		hexadecimalToDecimal();
	}

	void hexadecimalToDecimal() {
		short int iter = 0, decimal = 0;

		for (auto i : hexadecimals) {
			if (i != hexadecimals.back()) {
				for (auto k : i) {
					iter++;
					if (k >= 'A' && k <= 'Z') {
						k -= 55;
						decimal += int(k) * pow(16, i.size() - iter);
					}
					else {
						k -= 48;
						decimal += int(k) * pow(16, i.size() - iter);
					}
				}
				decimals.push_back(decimal);
				decimal = 0, iter = 0;
			}
		}

		formuala();
	}

	void formuala() {
		short int iter = 0;

		for (auto& i : decimals) {
			iter++;
			if (i >= 600) {
				i = ((i - (decimals.size() * 2) - iter + 741) / 58);
			}
			else {
				i = i;
			}
		}

		for (auto i : decimals) {
			decryptMessage += i;
		}

		caesarsCipher();
	}

	void caesarsCipher() {
		for (auto& i : decryptMessage) {
			if (i >= 'A' && i <= 'Z') {
				i = (i - 'A' - shift + 26) % 26 + 'A';
			}
			else if (i >= 'a' && i <= 'z') {
				i = (i - 'a' - shift + 26) % 26 + 'a';
			}
		}

		hexadecimals.clear();
		decimals.clear();
	}

public:
	string decrypt(string message) {
		separation(message);
		return decryptMessage;
	}
};

class MessageCryptor { // encrypts message in 3 steps
private:
	vector <int> Nums;
	short int seconds = 0;
	string cryptMessage = "";

	void caesarsCipher(string message) {
		auto now = chrono::system_clock::now();
		time_t currentTime = chrono::system_clock::to_time_t(now);
		tm* localTime = localtime(&currentTime);
		int currentSecond = localTime->tm_sec % 19 + 1;
		seconds = currentSecond;
		string cryptedMessage;

		for (auto i : message) {
			if ((i >= 'A' && i <= 'Z') || (i >= 'a' && i <= 'z')) {
				if (i >= 'A' && i <= 'Z') {
					if (currentSecond == 0) {
						currentSecond++;
					}
					if (i + currentSecond > 'Z') {
						i = ('A' + (currentSecond - ('Z' - i))) - 1;
					}
					else {
						
						i += currentSecond;
					}
				}
				else if (i >= 'a' && i <= 'z') {
					if (currentSecond == 0) {
						currentSecond++;
					}
					if (i + currentSecond > 'z') {
						i = ('a' + (currentSecond - ('z' - i))) - 1;
					}
					else {
						i += currentSecond;
					}
				}
				cryptedMessage += string(1, i);
			}
			else {
				cryptedMessage += i;
			}
		}

		formula(cryptedMessage);
	}

	void formula(string message) {
		vector <int> asciiNums;
		short int iter = 0, messageLenght;

		for (auto i : message) {
			if (i == ' ') {
				asciiNums.push_back(' ');
			}
			else {
				asciiNums.push_back(i);
			}
		}
		messageLenght = asciiNums.size();
		for (auto i : asciiNums) {
			iter++;
			if (i == ' ') {
				Nums.push_back(' ');
			}
			else if ((i >= 'A' && i <= 'Z') || (i >= 'a' && i <= 'z')) {
				Nums.push_back((i + 57 * (i - 13)) + iter + (messageLenght * 2));
			}
			else {
				Nums.push_back(i);
			}
		}

		decimalToHexadecimal();
	}

	void decimalToHexadecimal() {
		string hexadecimalString, hexadecimalChar, cryptedMessage;
		for (auto decimal : Nums) {
			while (decimal >= 16) {
				int remains = decimal % 16;

				if (remains >= 10) {
					hexadecimalChar = string(1, 'A' + remains - 10);
				}
				else {
					hexadecimalChar = to_string(remains);
				}

				hexadecimalString = hexadecimalChar + hexadecimalString;
				decimal = (decimal - remains) / 16;
			}

			if (decimal >= 10) {
				hexadecimalString = string(1, 'A' + decimal - 10) + hexadecimalString;
			}
			else {
				hexadecimalString = to_string(decimal) + hexadecimalString;
			}
			cryptedMessage += hexadecimalString + "/";
			hexadecimalString = "";
		}

		cryptedMessage += to_string(seconds) + "/";
		cryptMessage = cryptedMessage;
		Nums.clear();
	}

public:
	string crypt(string message) {
		caesarsCipher(message);
		string cryptedMessage = cryptMessage;
		return cryptedMessage;
	}

};

class ConnectionBlock { // class for creating data structure
public:
	SOCKET socket;
	bool connected;

	ConnectionBlock(SOCKET s, bool c) : socket(s), connected(c) {}
};

typedef list<ConnectionBlock> ConnectionList;
ConnectionList connections;

class ConnectionHandler   { // includes "keep-alive" and connections handler
private:
	mutex mtx;

public:
	void newConnections(SOCKET clientSocket) {
		connections.push_back({clientSocket, true});
		viewConnections();
	}

	void viewConnections() { // displays all active connections on server side
		system("cls");
		string firstTitle = " Active Clients - ", secondTitle = " | Total Clients - ", changedTitle;

		for (ConnectionList::iterator it = connections.begin(); it != connections.end(); ++it) {
			SOCKET clientSocket = it->socket;
			bool connected = it->connected;
			
			cout << "Сокет:" << socket << " Подключение:" << connected << endl;
		}

		changedTitle = firstTitle + to_string(connections.size());
		wstring wideChangedTitle;
		wideChangedTitle.assign(changedTitle.begin(), changedTitle.end());
		SetConsoleTitle(wideChangedTitle.c_str());
	}

	void keepAlive() { // keep-alive function. Closing socket if client dont send packet throughout 0.8 sec
		while (true) {
			unique_lock<mutex> lock(mtx);
			for (ConnectionList::iterator it = connections.begin(); it != connections.end(); ) {
				SOCKET clientSocket = it->socket;
				WSAEVENT keepAliveEvent = WSACreateEvent();
				WSAEventSelect(clientSocket, keepAliveEvent, FD_READ | FD_CLOSE);

				DWORD waitResult = WSAWaitForMultipleEvents(1, &keepAliveEvent, FALSE, 800, FALSE);

				if (waitResult == WSA_WAIT_FAILED) {
					cerr << "WSAWaitForMultipleEvents failed with error " << WSAGetLastError() << endl;
				}
				else if (waitResult == WSA_WAIT_TIMEOUT) {
					closesocket(clientSocket);
					it = connections.erase(it);
					WSACloseEvent(keepAliveEvent);
					viewConnections();
					cout << "Keep-alive timeout, client disconnected" << endl;
				}
				else {
					WSANETWORKEVENTS networkEvents;
					WSAEnumNetworkEvents(clientSocket, keepAliveEvent, &networkEvents);

					if (networkEvents.lNetworkEvents & FD_CLOSE) {
						closesocket(clientSocket);
						it = connections.erase(it);
						WSACloseEvent(keepAliveEvent);
						viewConnections();
						cout << "Client disconnected" << endl;
					}
					else if (networkEvents.lNetworkEvents & FD_READ) {
						char buffer[1024];
						int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
						if (bytesReceived <= 0) {
							closesocket(clientSocket);
							it = connections.erase(it);
							WSACloseEvent(keepAliveEvent);
							viewConnections();
							cout << "Keep-alive failed, client disconnected" << endl;
						}
						else {
							++it;
						}
					}
				}
			}
			lock.unlock();
		}
	}
};
 
class SendingMessages { // sending messages to clients and executing them simultaneously
private:
	queue<string> messages;
	mutex mtx;

public:

	void entermessage() { // entering message from server side
		string message = "";
		thread(&SendingMessages::sendMessage, this).detach(); // create other thread for sending meassages
		while (true) {
			getline(cin, message);
			if (message != "") {
				unique_lock<mutex> lock(mtx);
				messages.push(message);
				lock.unlock();
			}
		}
	}

	void sendMessage() { // sending message to all connected clients
		string message;
		MessageCryptor MC;

		while (true) {
			unique_lock<mutex> lock(mtx);
			if (!messages.empty()) {
				message = messages.front();
				message = MC.crypt(message); // crypting message
				for (ConnectionList::iterator it = connections.begin(); it != connections.end(); ) {
					SOCKET clientSocket = it->socket;
					int result = send(clientSocket, message.c_str(), (int)message.size(), 0); // send
					if (result == SOCKET_ERROR) {
						cerr << "Send failed with error " << WSAGetLastError() << endl;
						closesocket(clientSocket);
						it = connections.erase(it);
					}
					else {
						++it;
					}
				}
				messages.pop();
			}
			lock.unlock();
		}
	}
}; 

class Connect { // code of client and server connect
private:
	void tryAgain() { // retrying connect every 5 sec
		Sleep(5000);
		client();
	}

	void keepAliveClient(SOCKET connectSocket) { // every 0.1 sec send packet to server
		while (true) {
			if (send(connectSocket, "", 1, 0) == SOCKET_ERROR) {
				tryAgain();
			}
			Sleep(100);
		}
	}

public:
	void client() {
		WSADATA wsaData;
		ADDRINFO hints;
		ADDRINFO* addrResult = NULL;
		SOCKET connectSocket = INVALID_SOCKET;
		char recvBuffer[512];
		string message;

		MessageDecryptor MDE;

		ZeroMemory(&hints, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;

		if (WSAStartup(MAKEWORD(2, 2), &wsaData) == 1) {
			cout << "(!) WSAStartup error\n";
			tryAgain();
			WSACleanup();
		}
		else if (getaddrinfo("localhost", "5461", &hints, &addrResult) == 1) {
			cout << "(!) getaddrinfo error\n";
			tryAgain();
			WSACleanup();
			freeaddrinfo(addrResult);
		}
		else if ((connectSocket = socket(addrResult->ai_family, addrResult->ai_socktype, addrResult->ai_protocol)) == INVALID_SOCKET) {
			cout << "(!) socket error\n";
			tryAgain();
			WSACleanup();
			freeaddrinfo(addrResult);
			closesocket(connectSocket);
		}
		else if (connect(connectSocket, addrResult->ai_addr, (int)addrResult->ai_addrlen) == SOCKET_ERROR) {
			cout << "(!) connect error\n";
			tryAgain();
			WSACleanup();
			freeaddrinfo(addrResult);
			closesocket(connectSocket);
		}
		else {
			cout << "(!) successful connect :)" << endl;
			thread(&Connect::keepAliveClient, this, connectSocket).detach();

			while (true) {
				memset(recvBuffer, 0, 512);
				if (recv(connectSocket, recvBuffer, 512, 0) > 0) {
					message = recvBuffer;
					cout << MDE.decrypt(message);
				}
			}
		}
	}

	void server() {
		WSADATA wsaData;
		ADDRINFO hints;
		ADDRINFO* addrResult = NULL;
		SOCKET clientSocket = INVALID_SOCKET;
		SOCKET listenSocket = INVALID_SOCKET;
		ConnectionHandler CH;
		SendingMessages SC;

		ZeroMemory(&hints, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;
		hints.ai_flags = AI_PASSIVE;

		if (WSAStartup(MAKEWORD(2, 2), &wsaData) == 1) {
			cout << "(!) WSAStartup error\n";
			WSACleanup();
		}
		else if (getaddrinfo(NULL, "5461", &hints, &addrResult) == 1) {
			cout << "(!) getaddrinfo error\n";
			WSACleanup();
			freeaddrinfo(addrResult);
		}
		else if ((listenSocket = socket(addrResult->ai_family, addrResult->ai_socktype, addrResult->ai_protocol)) == INVALID_SOCKET) {
			cout << "(!) socket error\n";
			WSACleanup();
			freeaddrinfo(addrResult);
			closesocket(listenSocket);
		}
		else if (bind(listenSocket, addrResult->ai_addr, (int)addrResult->ai_addrlen) == SOCKET_ERROR) {
			cout << "(!) bind error\n";
			WSACleanup();
			freeaddrinfo(addrResult);
			closesocket(listenSocket);
		}
		else if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
			cout << "(!) listen error\n";
			WSACleanup();
			freeaddrinfo(addrResult);
			closesocket(listenSocket);
		}
		else {
			thread(&SendingMessages::entermessage, &SC).detach();
			thread(&ConnectionHandler::keepAlive, &CH).detach();
			while (true) {
				if ((clientSocket = accept(listenSocket, NULL, NULL)) != INVALID_SOCKET) {
					CH.newConnections(clientSocket);
				}
				else {
					cout << "(!) accept error\n";
				}
			}
		}
	}
};

int main() {
	setlocale(LC_ALL, "ru");
	srand(time(0));									
	Connect Con;
	string CorS;

	cout << "c - client\ns - server\n"; // choose client or server
	cin >> CorS;

	if (CorS == "c")
		Con.client(); // start client part
	else if (CorS == "s")
		Con.server(); // start server part

	system("pause");
}