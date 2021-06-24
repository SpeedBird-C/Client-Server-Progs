#define _CRT_SECURE_NO_WARNINGS
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define MAX_CONNECTORS 1000
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib") 
#include <windows.h>
#include <winsock2.h>
// ��������� ����������: ������������ ���������� �������
#pragma comment(lib, "ws2_32.lib")
#else // LINUX
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
int set_non_block_mode(int s)
{
#ifdef _WIN32
	unsigned long mode = 1;
	return ioctlsocket(s, FIONBIO, &mode);
#else
	int fl = fcntl(s, F_GETFL, 0);
	return fcntl(s, F_SETFL, fl | O_NONBLOCK);
#endif
}
int sock_err(const char* function, int s)
{
	int err;
#ifdef _WIN32
	err = WSAGetLastError();
#else
	err = errno;
#endif
	fprintf(stderr, "%s: socket error: %d\n", function, err);
	return -1;
}void deinit()
{
#ifdef _WIN32
	// ��� Windows ������� ������� WSACleanup � ����� ������
	WSACleanup();
#else
	// ��� ������ �� �������� �� ���������
#endif
}int init()
{
#ifdef _WIN32
	// ��� Windows ������� ������� WSAStartup ����� ������� ������������� �������
	WSADATA wsa_data;
	return (0 == WSAStartup(MAKEWORD(2, 2), &wsa_data));
#else
	return 1; // ��� ������ �� �������� �� ���������
#endif
}
int recv_string(int cs)
{
	char buffer[512];
	int curlen = 0;
	int rcv;
	do
	{
		int i;
		rcv = recv(cs, buffer, sizeof(buffer), 0);
		for (i = 0; i < rcv; i++)
		{
			if (buffer[i] == '\n')
				return curlen;
			curlen++;
		}
		if (curlen > 5000)
		{
			printf("input string too large\n");
			return 5000;
		}
	} while (rcv > 0);
	return curlen;
}
int send_notice(int cs, int len)
{
	char buffer[1024];
	int sent = 0;
	int ret;
#ifdef _WIN32
	int flags = 0;
#else
	int flags = MSG_NOSIGNAL;
#endif
	sprintf(buffer, "Length of your string: %d chars.", len);
	while (sent < (int)strlen(buffer))
	{
		ret = send(cs, buffer + sent, strlen(buffer) - sent, flags);
		if (ret <= 0)
			return sock_err("send", cs);
		sent += ret;
	}
	return 0;
}
void s_close(int s)
{
#ifdef _WIN32
	closesocket(s);
#else
	close(s);
#endif
}
int main(int argc, char* argv[])
{
	
	unsigned int port = 0;
	if (argc == 2)
		port = (unsigned short int)atoi((const char*)argv[1]);
	else
		port = 9000;
	FILE* f;
	f = fopen("msg.txt", "a+");

	int ls=0;// �����,�������������� ����������
	int cs[MAX_CONNECTORS];//������ � ������������� ���������
	struct sockaddr_in addr;
	fd_set rfd;
	fd_set wfd;
	int nfds = ls;
	int i;
	struct timeval tv = { 1,0 };
	init();
	ls = socket(AF_INET, SOCK_STREAM, 0);
	//������� ������ � ������������� �����
	set_non_block_mode(ls);
	if (ls < 0)
		return sock_err("socket", ls);
	for (int i = 0; i < MAX_CONNECTORS; i++)
	{
		cs[i] = socket(AF_INET, SOCK_STREAM, 0);
		//������� ������ � ������������� �����
		set_non_block_mode(cs[i]);
	}
	// ���������� ������ �������������
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);			  // ������ �� ��������� ������������ ���� 9000
	addr.sin_addr.s_addr = htonl(INADDR_ANY); // ��� ������

	// ���������� ������ � ������ �������������
	if (bind(ls, (struct sockaddr *)&addr, sizeof(addr)) < 0)
		return sock_err("bind", ls);

	//������� ������ � ��������� �������������
	if (listen(ls, MAX_CONNECTORS) < 0)
	{
		return sock_err("listen", ls);
		closesocket(ls);

		for (int i = 0; i < MAX_CONNECTORS; i++)
		{
			closesocket(cs[i]);

		}
		return 1;
	}

	
	char bufer[100] = "\0";

	int k = 0;
	while (1)
	{
		FD_ZERO(&rfd);
		FD_ZERO(&wfd);
		FD_SET(ls, &rfd);
		for (i = 0; i < MAX_CONNECTORS; i++)
		{
			FD_SET(cs[i], &rfd);
			FD_SET(cs[i], &wfd);
			if (nfds < cs[i])
				nfds = cs[i];
		}
		if (select(nfds + 1, &rfd, &wfd, 0, &tv) > 0)
		{
			// ���� �������
			if (FD_ISSET(ls, &rfd))
			{
				int addrlen = sizeof(addr);
				// ���� ������� �� �������������� ������, ����� ������� accept, �������
				int sockfd=accept(ls, (struct sockaddr*) &addr, &addrlen);
				// ����������� � �������� ����� ��������������� ������� � ������ cs
				cs[k] = sockfd;
				k++;

			}
			for (i = 0; i < MAX_CONNECTORS; i++)
			{
				if (FD_ISSET(cs[i], &rfd))
				{
					// ����� cs[i] �������� ��� ������. ������� recv ������ ������,recvfrom - �����������
						recv(cs[i], bufer, 100, 0);
						fprintf(f, "%s", bufer);
				}
				if (FD_ISSET(cs[i], &wfd))
				{
					// ����� cs[i] �������� ��� ������. ������� send � sendto ����� ������� ���������
					send(cs[i], "ok", 2, 0);
				}
			}
		}
		else
		{
			fprintf(stdout, "err");// ��������� ������� ��� ������
		}
	}


	closesocket(ls);

	for (int i = 0; i < MAX_CONNECTORS; i++)
	{
		closesocket(cs[i]);
		
	}



	

	/*int s;
	struct sockaddr_in addr;
	// �������������� ������� ����������
	init();
	// �������� TCP-������
	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0)
		return sock_err("socket", s);
	// ���������� ������ �������������
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(9000); // ������ ������������ ���� 9000
	addr.sin_addr.s_addr = htonl(INADDR_ANY); // ��� ������
											  // ���������� ������ � ������ �������������
	if (bind(s, (struct sockaddr*) &addr, sizeof(addr)) < 0)
		return sock_err("bind", s);
	// ������ �������������
	if (listen(s, MAX_CONNECTORS) < 0)
		return sock_err("listen", s);
	do
	{
		// �������� ���������� ��������������� �������
		int addrlen = sizeof(addr);
		int cs = accept(s, (struct sockaddr*) &addr, &addrlen);
		unsigned int ip;
		int len;
		if (cs < 0)
		{
			sock_err("accept", s);
			break;
		}
		// ����� ������ �������
		ip = ntohl(addr.sin_addr.s_addr);
		printf(" Client connected: %u.%u.%u.%u ",
			(ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, (ip) & 0xFF);
		// ����� �� ������� ������
		len = recv_string(cs);
		// �������� ������� ��������� � ����� ���������� ������
		send_notice(cs, len);
		printf(" string len is: %d\n", len);
		// ���������� �������
		s_close(cs);
	} while (1); // ���������� ����� ��������� � ����. �����
	s_close(s);
	deinit();*/
	return 0;
}