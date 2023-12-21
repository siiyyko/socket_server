#pragma once

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <mutex>

#pragma comment (lib, "Ws2_32.lib")

extern std::vector<SOCKET> clients;
extern std::vector<std::string> badWords;

void startAcceptingIncomingConnections(SOCKET clientSocket);
void receiveAndPrintIncomingData(SOCKET clientSocket);
DWORD WINAPI receiveAndPrintIncomingDataTHREAD(LPVOID lpParam);
DWORD WINAPI checkForBadWord(LPVOID lpParam);
void sendReceivedToAllClients(char buffer[1024]);
std::vector<std::string> getBadWords();
void sendForumHistory(SOCKET clientSocket);
void recalculateBadWords();