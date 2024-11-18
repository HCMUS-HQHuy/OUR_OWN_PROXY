#include <chrono>

#include "./../../HEADER/ClientHandler.h"


ClientHandler::ClientHandler(SOCKET sock) {
    // std::cerr << "Create new client Handler\n";
    // activeThreads++;
    // std::cout << "Thread started. Active threads: " << activeThreads.load() << std::endl;
    socketHandler = new SocketHandler(sock);
}

ClientHandler::~ClientHandler() {
    delete socketHandler;
    // activeThreads--;
    // std::cout << "Thread finished. Active threads: " << activeThreads.load() << std::endl;
    // std::cerr << "Destructure client Handler\n";
}

void ClientHandler::handleRequest() {
    if (socketHandler->isValid() == false) return;
    if (socketHandler->setSSLContexts() == false) return;

    struct pollfd fds[2];
    fds[0].fd = socketHandler->browserSocket;
    fds[0].events = POLLIN;  

    fds[1].fd = socketHandler->remoteSocket;
    fds[1].events = POLLIN;  

    #define TIMEOUT 1000 
    #define BUFFER_SIZE 1024
    #define MAX_IDLE_TIME 5000


    char buffer[BUFFER_SIZE];
    auto lastActivity = std::chrono::steady_clock::now();
    int STEP = 0;
    while (true) {
        int ret = WSAPoll(fds, 2, TIMEOUT);
        // std::cerr << "STEP: " << ++STEP << '\n';
        if (ret < 0) {
            std::cerr << "WSAPoll ERROR!\n";
            break;
        } else if (ret == 0) {
            // Kiểm tra idle timeout
            const auto& currentTime = std::chrono::steady_clock::now();
            const auto& idleDuration = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastActivity).count();
            if (idleDuration > MAX_IDLE_TIME) {
                // std::cerr << "TIMEOUT\n";
                break;
            }
            // std::cerr <<"NO activity\n";
            continue;
        }
        
        lastActivity = std::chrono::steady_clock::now();

        if (fds[0].revents & POLLIN) {
            // std::cerr << "STEP: " << STEP << " browser\n";
            RequestHandler request(socketHandler);
            if (request.handleRequest() == false) break;
        }
        if (fds[1].revents & POLLIN) {
            // std::cerr << "STEP: " << STEP << " server\n";
            ResponseHandler response(socketHandler);
            if (response.handleResponse() == false) break;
        }
        // std::cerr << "END STEP: " << STEP << '\n';
    }
    // std::cerr << "FINISHED!!!!\n";
}

