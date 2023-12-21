#include "funcs.h"

extern std::vector<SOCKET> clients;
extern std::vector<std::string> badWords;
extern std::vector<int> indexesOfBadWords;
extern std::vector<HANDLE> threads;
extern HANDLE mutex;
extern std::vector<int> badWordsCount;

std::vector<std::string> history;
struct Parameters {
    char* text;
    int number;
};

void startAcceptingIncomingConnections(SOCKET listenSocket)
{
    while (true) {
        SOCKET clientSocket = accept(listenSocket, NULL, NULL);

        clients.push_back(clientSocket);
        sendForumHistory(clientSocket);

        receiveAndPrintIncomingData(clientSocket);
    }
}

void sendReceivedToAllClients(char buffer[1024]) {
    for (SOCKET socket : clients) {
        send(socket, buffer, strlen(buffer) + 1, 0);
    }
}

std::vector<std::string> getBadWords()
{
    std::vector<std::string> badWords;
    
    std::ifstream inputFile("badWords.txt");

    if (inputFile.is_open()) {
        std::string line;

        while (std::getline(inputFile, line)) {
            badWords.push_back(line);
        }

        inputFile.close();
    }
    else {
        std::cout << "Unable to open file!" << std::endl;
    }

    return badWords;
}

void sendForumHistory(SOCKET clientSocket)
{
    for (std::string message : history) {
        message += '\n';
        char buffer[1024];
        strcpy_s(buffer, sizeof buffer, message.c_str());
        send(clientSocket, buffer, strlen(buffer) + 1, 0);
    }
}

void recalculateBadWords()
{
    std::ofstream badWordsCountStream("badWordsCount.txt", std::ios::trunc);
    if (!badWordsCountStream.is_open()) {
        std::cout << "Something went wrong opening bad words count file.\n";
    }

    int i = 0;
    for (std::string word : badWords) {
        std::string badWord = word + ": " + std::to_string(badWordsCount[i]);
        ++i;
        badWordsCountStream << badWord << std::endl;
    }
}

DWORD WINAPI receiveAndPrintIncomingDataTHREAD(LPVOID lpParam) {
    SOCKET* clientSocket = static_cast<SOCKET*>(lpParam);

    int iResult, i = 0;
    char buffer[1024];

    while (true) {
        i = 0;
        iResult = recv(*clientSocket, buffer, sizeof buffer, 0);
        
        int i = 0;
        for (std::string word : badWords) {
            Parameters* param = new Parameters();
            param->text = buffer;
            param->number = i;
            HANDLE hThread = CreateThread(NULL, 0, checkForBadWord, param, 0, NULL);
            threads.push_back(hThread);
            ++i;
        }

        for (HANDLE thread : threads) {
            WaitForSingleObject(thread, INFINITE);
        }

        if (iResult > 0) printf("%s\n", buffer);
        if (iResult <= 0) break;

        history.push_back(std::string(buffer));

        sendReceivedToAllClients(buffer);

        recalculateBadWords();
    }
    return 0;
}

std::vector<int> computeLPS(const char* pattern)
{
    int patternLen = strlen(pattern);

    std::vector<int> LPSarr(patternLen, 0);
    LPSarr[0] = 0; //для пустого рядка (і = 0) завжди 0

    int i = 1;
    int len = 0; // довжина найдовшого префікса, що є суфіксом

    //проходимось по всій довжині патерну
    while (i < patternLen) {

        if (pattern[i] == pattern[len]) {
            ++len;
            LPSarr[i] = len;
            ++i;
        }
        else {
            if (len != 0) len = LPSarr[len - 1];
            else {
                LPSarr[i] = 0;
                ++i;
            }
        }
    }
    return LPSarr;
}

DWORD WINAPI checkForBadWord(LPVOID lpParam)
{
    Parameters* param = static_cast<Parameters*>(lpParam);

    char* text = param->text;
    const char* pattern = badWords[param->number].c_str();

    std::vector<int> LPSarr = computeLPS(pattern);

    int textLen = strlen(text);
    int patternLen = strlen(pattern);

    int i = 0; // індекс для стрічки text
    int j = 0; // індекс для стрічки pattern

    std::vector<int> indexes;

    while (i < textLen) {
        if (pattern[j] == text[i]) {
            ++i;
            ++j;
        }

        if (j == patternLen) {
            //WaitForSingleObject(mutex, INFINITE);
            ///*indexesOfBadWords.push_back(i - j);*/
            indexes.push_back(i - j);
            //ReleaseMutex(mutex);
            j = LPSarr[j - 1];
        }
        else if (i < textLen && pattern[j] != text[i]) {
            if (j != 0) j = LPSarr[j - 1];
            else ++i;
        }
    }

    WaitForSingleObject(mutex, INFINITE);
    for (int index : indexes) {
        ++badWordsCount[param->number];
        for (int j = index; j < index + patternLen; ++j)
            param->text[j] = '*';
    }
    ReleaseMutex(mutex);

    return 0;
}

void receiveAndPrintIncomingData(SOCKET clientSocket)
{
    HANDLE hThread = CreateThread(NULL, 0, receiveAndPrintIncomingDataTHREAD, &clientSocket, 0, NULL);
}