#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include "funcs.h"

#pragma comment (lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

std::vector<SOCKET> clients;
std::vector<std::string> badWords;
std::vector<int> indexesOfBadWords;
std::vector<HANDLE> threads;
HANDLE mutex;
std::vector<int> badWordsCount;

//extern std::vector<SOCKET> clients;
//extern std::vector<std::string> badWords;

int __cdecl main(void)
{
    mutex = CreateMutex(NULL, 0, NULL);

    WSADATA wsaData;
    int iResult;

    SOCKET ListenSocket = INVALID_SOCKET;
    SOCKET ClientSocket = INVALID_SOCKET;

    struct addrinfo* result = NULL;
    struct addrinfo hints;

    int iSendResult;
    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Create a SOCKET for the server to listen for client connections.
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    // Setup the TCP listening socket
    iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);
    

    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    printf("Server is ready to accept new messages!\n");

    badWords = getBadWords();

    printf("List of bad words:\n");
    for (std::string word : badWords) {
        std::cout << word << std::endl;
    }

    std::ofstream badWordsCountStream("badWordsCount.txt", std::ios::trunc);
    if (badWordsCountStream.is_open()) {
        std::cout << "Calculating the number of bad words started!\n";
        for(std::string word : badWords) {
            badWordsCount.push_back(0);
        }
        badWordsCountStream.close();
        recalculateBadWords();
    }
    else {
        std::cout << "Something went wrong opening bad words count file.\n";
        return -1;
    }

    startAcceptingIncomingConnections(ListenSocket);
    
    // shutdown the connection since we're done
    iResult = shutdown(ClientSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ClientSocket);
        WSACleanup();
        return 1;
    }

    // cleanup
    closesocket(ClientSocket);
    WSACleanup();

    printf("%s", recvbuf);

    return 0;
}


