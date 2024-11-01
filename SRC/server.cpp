#include "../HEADER/setting.h"
#include "../HEADER/supportFunction.h"

#pragma comment(lib, "ws2_32.lib")

WSADATA wsaData; 
SOCKET localSocket, remoteSocket;

// Function to parse and get the host name from the HTTP request
bool getHostFromRequest(const std::string request, std::string &hostname, int &port) {
    port = (request.find("CONNECT") == 0 ? HTTPS_PORT : HTTP_PORT);
    std::string doman = request.substr(request.find("Host: ") + 6);

    if (port == HTTP_PORT) {
        hostname = doman.substr(0, doman.find('\n'));
        while (not IsCharAlpha(hostname.back())) hostname.pop_back();
        if (hostname.empty()) {
            std::cerr << "Host not found in request." << std::endl;
            return false;
        }  
        return true;
    } 

    size_t colonPos = doman.find(':');  
    if (colonPos != std::string::npos) {
        hostname = doman.substr(0, colonPos);           // Lấy phần tên miền
        port = std::stoi(doman.substr(colonPos + 1));   // Lấy phần cổng
    } else {
        hostname = doman;  // Nếu không có cổng, dùng tên miền như đã nhập
    }
    if (hostname.empty()) {
        std::cerr << "Host not found in request." << std::endl;
        return false; 
    }  
    return true;
}

bool isInitializedWinsock();
bool isInitializedSocket(SOCKET &socket);
bool bindSocketToAdressPort(ADDRESS_FAMILY, u_long, int);
bool handleClient(SOCKET&);
 
int runServerProxy() {
    WSADATA wsaData; if (not isInitializedWinsock()) return EXISTS_ERORRS;
    if (not isInitializedSocket(localSocket)) return EXISTS_ERORRS;

    if (not bindSocketToAdressPort(AF_INET, INADDR_ANY, LOCAL_PORT)) return EXISTS_ERORRS;

    // Start listening for connections
    if (listen(localSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Error listening on socket." << std::endl;
        closesocket(localSocket);
        WSACleanup();
        return 1;
    }
    std::cout << "Proxy is listening on port " << LOCAL_PORT << "..." << std::endl;

    while (true) {
        SOCKET clientSocket = accept(localSocket, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Error accepting connection from client." << std::endl;
            closesocket(remoteSocket);
            closesocket(localSocket);
            WSACleanup();
            return false;
        }
        if (handleClient(clientSocket) == false) 
            std::cerr << "HAVE SOME PROBLEMS\n";
        closesocket(clientSocket);
    }
    closesocket(remoteSocket);
    closesocket(localSocket);
    WSACleanup();
    return 0;
}

bool isInitializedWinsock() {
    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Error initializing Winsock." << std::endl;
        return false;
    }
    return true;
}

bool isInitializedSocket(SOCKET &sock) {
    // Create a socket to listen for client connections
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Error creating socket." << std::endl;
        closesocket(remoteSocket);
        closesocket(localSocket);
        WSACleanup();
        return false;
    }
    return true;
}

bool bindSocketToAdressPort(ADDRESS_FAMILY sin_family, u_long s_address, int port) {
    sockaddr_in localAddr;
    // Set address and port for the listening socket
    localAddr.sin_family = sin_family;
    localAddr.sin_addr.s_addr = s_address;
    localAddr.sin_port = htons(port);

    // Bind the listening socket to the address and port
    if (bind(localSocket, (sockaddr*)&localAddr, sizeof(localAddr)) == SOCKET_ERROR) {
        std::cerr << "ERROR binding socket!!!!" << std::endl;
        closesocket(remoteSocket);
        closesocket(localSocket);
        WSACleanup();
        return false;
    }
    return true;
}

bool handleClient(SOCKET &clientSocket) {
    // Receive request from client
    const int bufferSize = 9000;
    char buffer[bufferSize + 1]; int recvSize = bufferSize;
    if (not receiveMSG(clientSocket, buffer, recvSize)) return false;

    // Convert request to string and get the host name
    std::string request(buffer, recvSize);
    std::string host; int port; getHostFromRequest(request, host, port);
    hostent* hostInfo = gethostbyname(host.c_str());
    // Resolve the host name to an IP address
    if (hostInfo == nullptr) {
        std::cerr << "Failed to resolve host name." << std::endl;
        return false;
    }
    
    // Create a socket to connect to the remote server
    if (not isInitializedSocket(remoteSocket)) return EXISTS_ERORRS;
    // Set the remote server address
    sockaddr_in remoteAddr;
    remoteAddr.sin_family = AF_INET;
    remoteAddr.sin_port = htons(port); 
    remoteAddr.sin_addr.s_addr = *((unsigned long*)hostInfo->h_addr_list[0]);

    // Connect to the remote server
    if (connect(remoteSocket, (sockaddr*)&remoteAddr, sizeof(remoteAddr)) == SOCKET_ERROR) {
        std::cerr << "Error connecting to remote server." << std::endl;
        return false;
    }

    if (port == HTTPS_PORT) {
        char connectResponse[] = "HTTP/1.1 200 Connection Established\r\n\r\n"; int len = strlen(connectResponse);
        if (not sendMSG(clientSocket, connectResponse, len)) 
            return false;
    }

    fd_set readfds;
    int step = 0;
    while (true) {
        FD_ZERO(&readfds);
        FD_SET(clientSocket, &readfds);
        FD_SET(remoteSocket, &readfds);
        TIMEVAL delay; delay.tv_sec = 2; delay.tv_usec=0;
        int activity = select(0, &readfds, nullptr, nullptr, (TIMEVAL*)&delay);
        if (activity <= 0) break;
        int cnt = 0;
        if (FD_ISSET(clientSocket, &readfds)) {
            int bytesReceived = bufferSize;
            if (not receiveMSG(clientSocket, buffer, bytesReceived)) break;
            send(remoteSocket, buffer, bytesReceived, 0);
            cnt++;
        }
        if (FD_ISSET(remoteSocket, &readfds)) {
            int bytesReceived = bufferSize;
            if (not receiveMSG(remoteSocket, buffer, bytesReceived)) break;
            send(clientSocket, buffer, bytesReceived, 0);
            cnt++;
        }
    }
    return true;
}