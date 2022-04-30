﻿#include "pch.hpp"
#include "protocol.hpp"
#include "NetworkFramework.hpp"

CNetworkFramework::CNetworkFramework() : server_key(9999), over(nullptr), over_ex(nullptr), packet(nullptr)
{
}

CNetworkFramework::~CNetworkFramework()
{
}

void CNetworkFramework::OnCreate()
{
	WSADATA wsa;

	if (WSAStartup(MAKEWORD(2, 2), &wsa) != NOERROR)
	{
		ErrorQuit(L"WSAStartup fuction error", WSAGetLastError());
	}

	if (iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, NULL, NULL);
		iocp == nullptr)
	{
		ErrorQuit(L"IOCP Handle Creation Failed", WSAGetLastError());
	}

	server = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);

	sockaddr_in serverAddr;
	ZeroMemory(&serverAddr, sizeof(sockaddr_in));

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	CreateIoCompletionPort(reinterpret_cast<HANDLE>(server), iocp, server, 0);

	if (bind(server, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR)
	{
		ErrorQuit(L"bind function error", WSAGetLastError());
	}

	if (listen(server, SOMAXCONN) == SOCKET_ERROR)
	{
		ErrorQuit(L"listen function error", WSAGetLastError());
	}
}

void CNetworkFramework::OnDestroy()
{
	delete packet;
	delete over;
	delete over_ex;

	packet = nullptr;
	over = nullptr;
	over_ex = nullptr;

	closesocket(server);
	WSACleanup();
}

void CNetworkFramework::CreateThread()
{
	SOCKET client{ WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED) };
	OVERLAPPEDEX acceptOver;
	ZeroMemory(&acceptOver, sizeof(acceptOver));

	acceptOver.type = static_cast<char>(COMPLETION_TYPE::ACCEPT);
	acceptOver.wsa_buf.buf = reinterpret_cast<char*>(client);

	AcceptEx(server, client, acceptOver.data, 0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, 0, &acceptOver.over);

	for (int i = 0; i < MAX_USER; ++i)
	{
		worker_threads.emplace_back(&CNetworkFramework::ProcessThread, this);
	}

	for (auto& thread : worker_threads)
	{
		thread.join();
	}
}

void CNetworkFramework::ProcessThread()
{
	DWORD bytes;
	ULONG_PTR key;
	BOOL ret;

	while (true)
	{
		ret = GetQueuedCompletionStatus(iocp, &bytes, &key, &over, INFINITE);
		over_ex = reinterpret_cast<OVERLAPPEDEX*>(over);

		if (!ret)
		{
			if (over_ex->type == static_cast<int>(COMPLETION_TYPE::ACCEPT))
			{
				std::cout << "Accept error" << std::endl;
			}
			else
			{
				std::cout << "GetQueuedCompletionStatus error on client[" << key << "]" << std::endl;

				DisconnectClient(key);

				if (over_ex->type == static_cast<int>(COMPLETION_TYPE::SEND))
				{
					ZeroMemory(over_ex, sizeof(over_ex));
					over_ex = nullptr;
				}

				continue;
			}
		}

		switch (over_ex->type)
		{
		case static_cast<int>(COMPLETION_TYPE::ACCEPT):
		{
			AcceptClient();
		}
		break;
		case static_cast<int>(COMPLETION_TYPE::RECV):
		{
			RecvData(bytes, key);
		}
		break;
		case static_cast<int>(COMPLETION_TYPE::SEND):
		{
			SendData(bytes, key);
		}
		break;
		default:
			break;
		}
	}
}

void CNetworkFramework::AcceptClient()
{
	int id{ GetNewClientID() };
	SOCKET client{ reinterpret_cast<SOCKET>(over_ex->wsa_buf.buf) };

	if (id != -1)
	{
		clients[id].GetPlayer().SetPosX(0);
		clients[id].GetPlayer().SetPosY(0);
		clients[id].SetID(id);
		clients[id].GetPlayer().SetName(0);
		clients[id].SetSocket(client);
		clients[id].SetRemainSize(0);

		CreateIoCompletionPort(reinterpret_cast<HANDLE>(client), iocp, id, 0);

		clients[id].RecvData();

		client = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
	}
	else
	{
		std::cout << "Max user exceeded\n";
	}

	ZeroMemory(&over_ex->over, sizeof(over_ex->over));
	over_ex->wsa_buf.buf = reinterpret_cast<char*>(client);

	AcceptEx(server, client, over_ex->data, 0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, 0, &over_ex->over);
}

void CNetworkFramework::RecvData(DWORD bytes, ULONG_PTR key)
{
	if (!bytes)
	{
		DisconnectClient(key);
	}

	int remain_size{ static_cast<int>(bytes) + clients[key].GetRemainSize() };
	int packet_size{ over_ex->data[0] };

	packet = over_ex->data;

	//while (remain_size > 0)
	//{
	//	int packetSize{ packet[0] };
	//	if (packetSize <= remain_size)
	//	{
	//		ProcessPacket(key, packet);
	//		packet += packetSize;
	//		remain_size -= packetSize;
	//	}
	//	else
	//		break;
	//}

	for (int size = remain_size; size > 0 or packet_size <= size; size -= packet_size)
	{
		ProcessPacket(key);
		
		packet += packet_size;
	}

	clients[key].SetRemainSize(remain_size);

	if (remain_size > 0)
	{
		memcpy(over_ex->data, packet, remain_size);
	}

	clients[key].RecvData();

	// 임시 패킷 초기화
	packet[0] = '\0';
	packet = nullptr;
}

void CNetworkFramework::SendData(DWORD bytes, ULONG_PTR key)
{
	if (!bytes)
	{
		DisconnectClient(key);
	}

	ZeroMemory(&over_ex, sizeof(over_ex));
	over_ex = nullptr;
}

void CNetworkFramework::ProcessPacket(int id)
{
	int packet_type{ packet[1] };

	switch (packet_type)
	{
	case CS::LOGIN:
	{
		ProcessLoginPacket(id, packet);
	}
	break;
	case CS::MOVE_PLAYER:
	{
		ProcessMovePacket(id, packet);
	}
	break;
	}
}

void CNetworkFramework::ProcessLoginPacket(int id, char* pack)
{
	login_packet = static_cast<CS::PACKET::LOGIN>(*pack);

	clients[id].mu.lock();

	if (clients[id].GetState() == SESSION_STATE::FREE)
	{
		clients[id].mu.unlock();
		return;
	}

	if (clients[id].GetState() == SESSION_STATE::INGAME)
	{
		clients[id].mu.unlock();
		DisconnectClient(id);

		return;
	}

	clients[id].GetPlayer().SetName(login_packet.name);
	clients[id].SendLoginPakcet();
	clients[id].SetState(SESSION_STATE::INGAME);
	clients[id].mu.unlock();

	for (auto& client : clients)
	{
		if (client.GetID() == id)
		{
			continue;
		}

		std::lock_guard<std::mutex> a{ client.mu };

		if (client.GetState() != SESSION_STATE::INGAME)
		{
			continue;
		}

		// 모든 플레이어에게 새로 접속한 플레이어 정보 전송
		add_player_packet.id = id;
		strcpy_s(add_player_packet.name, clients[id].GetPlayer().GetName());
		add_player_packet.size = sizeof(SC::PACKET::ADD_PLAYER);
		add_player_packet.type = SC::ADD_PLAYER;
		add_player_packet.x = clients[id].GetPlayer().GetPosX();
		add_player_packet.y = clients[id].GetPlayer().GetPosY();

		client.SendData(&add_player_packet);

		// 나에게 접속해 있는 모든 플레이어의 정보 전송
		add_player_packet.id = client.GetID();
		strcpy_s(add_player_packet.name, client.GetPlayer().GetName());
		add_player_packet.x = client.GetPlayer().GetPosX();
		add_player_packet.y = client.GetPlayer().GetPosY();

		clients[id].SendData(&add_player_packet);
	}

	std::cout << "player login" << std::endl;
}

void CNetworkFramework::ProcessMovePacket(int id, char* pack)
{
	clients[id].GetPlayer().Move(static_cast<DIRECTION>(reinterpret_cast<CS::PACKET::MOVE_PLAYER*>(pack)->direction));

	for (auto& client : clients)
	{
		std::lock_guard<std::mutex> a{ client.mu };

		if (client.GetState() == SESSION_STATE::INGAME)
		{
			client.SendMovePlayerPacket(id, SC::MOVE_PLAYER, client.GetPlayer());
		}
	}
}

int CNetworkFramework::GetNewClientID()
{
	for (int i = 0; i < MAX_USER; ++i)
	{
		std::lock_guard<std::mutex> a{ clients[i].mu };

		if (clients[i].GetState() == SESSION_STATE::FREE)
		{
			clients[i].SetState(SESSION_STATE::ACCEPTED);

			return i;
		}
	}

	return -1;
}

void CNetworkFramework::DisconnectClient(ULONG_PTR id)
{
	clients[id].mu.lock();

	if (clients[id].GetState() == SESSION_STATE::FREE)
	{
		clients[id].mu.unlock();

		return;
	}

	closesocket(clients[id].GetSocket());
	clients[id].SetState(SESSION_STATE::FREE);

	clients[id].mu.unlock();

	SC::PACKET::REMOVE_PLAYER packet;

	for (auto& player : clients)
	{
		if (player.GetID() == id)
		{
			continue;
		}

		std::lock_guard<std::mutex> a{ player.mu };

		if (player.GetState() == SESSION_STATE::INGAME)
		{
			continue;
		}

		packet.id = id;
		packet.size = sizeof(packet);
		packet.type = SC::REMOVE_PLAYER;

		player.SendData(&packet);
	}
}