#ifndef HTTPHANDLER_HPP
#define HTTPHANDLER_HPP

#include "./setting.hpp"
#include "./SocketHandler.hpp"

#define BUFFER_SIZE 1024
#define sz(s) (int)s.size()

class HttpHandler {
private:
    string header; 
    string body;

    size_t curChunkID, chunkEnd;

    char *buffer;

    bool flagEndMessage;
    int contentLength;
    bool isChunked;
    bool headersParsed;

    int STEP;

    void handleMessage(int bytesReceived);
    void onFlagEnd() {flagEndMessage = true;}
    void offFlagEnd() { flagEndMessage = false; }
    
    bool isEndChunk() { return chunkEnd <= curChunkID; }

protected: 
    SocketHandler* socketHandler;
    void printHeader();
public:
    HttpHandler();
    ~HttpHandler();
    int sendMessage(Socket id, int size = BUFFER_SIZE);
    int receiveMessage(Socket id, int size = BUFFER_SIZE);
    bool isEndMessage();
};

#endif