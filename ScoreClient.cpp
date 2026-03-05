#pragma execution_character_set("utf-8")

// winsock2.h は windows.h より必ず先にインクルードする
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

#include <fstream>
#include <string>
#include "ScoreClient.h"
#include "debug_ostream.h"

#include <thread>
#include <atomic>

// デフォルト値（設定ファイルが読めなかった場合のフォールバック
static const char* DEFAULT_IP = "192.168.10.19";
static const int   DEFAULT_PORT = 5000;

// server_config.txt からIPとポートを読み込む
// 書式:
//   1行目: IPアドレス（例: 192.168.10.19）
//   2行目: ポート番号（例: 5000）
static bool LoadServerConfig(std::string& outIP, int& outPort)
{
	std::ifstream file("server_config.txt");
	if (!file.is_open())
	{
		return false;
	}

	std::string ip;
	int port = 0;

	if (!std::getline(file, ip) || ip.empty())
	{
		return false;
	}

	// 先頭のUTF-8 BOM（EF BB BF）を除去
	if (ip.size() >= 3 &&
		(unsigned char)ip[0] == 0xEF &&
		(unsigned char)ip[1] == 0xBB &&
		(unsigned char)ip[2] == 0xBF)
	{
		ip.erase(0, 3);
	}

	// 末尾の改行・空白を除去（Windowsの \r\n 対策）
	while (!ip.empty() && (ip.back() == '\r' || ip.back() == '\n' || ip.back() == ' '))
	{
		ip.pop_back();
	}

	std::string portStr;
	if (!std::getline(file, portStr) || portStr.empty())
	{
		return false;
	}

	try
	{
		port = std::stoi(portStr);
	}
	catch (...)
	{
		return false;
	}

	outIP = ip;
	outPort = port;
	return true;
}

bool Score_SendToServer(int score)
{
	// 設定ファイルを読み込む（失敗時はデフォルト値を使用）
	std::string serverIP = DEFAULT_IP;
	int         serverPort = DEFAULT_PORT;
	LoadServerConfig(serverIP, serverPort);

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		return false;
	}

	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET)
	{
		WSACleanup();
		return false;
	}

	sockaddr_in serverAddr{};
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(serverPort);
	serverAddr.sin_addr.s_addr = inet_addr(serverIP.c_str());

	if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
	{
		closesocket(sock);
		WSACleanup();
		return false;
	}

	// int をそのまま送信（サーバーと同じ形式）
	int totalSent = 0;
	while (totalSent < (int)sizeof(int))
	{
		int sent = send(sock,
			(char*)&score + totalSent,
			sizeof(int) - totalSent,
			0);
		if (sent == SOCKET_ERROR)
		{
			closesocket(sock);
			WSACleanup();
			return false;
		}
		totalSent += sent;
	}

	closesocket(sock);
	WSACleanup();
	return true;
}