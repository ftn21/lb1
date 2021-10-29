/*
	Принимающий интерфейс
*/
#include <iostream>
#include<stdio.h>
#include<string.h>  
#include<stdlib.h> 
#include<arpa/inet.h>
#include<sys/socket.h>

using namespace std;

#define BUFLEN 512	// максимальная длина буфера
#define PORT 12346	// порт

void die(char *s)
{
	perror(s);
	exit(1);
}

int main(void)
{
	struct sockaddr_in si_me, si_other;
	
	int s, i, slen = sizeof(si_other) , recv_len;
	char buf[BUFLEN];
	
	// создаем UDP socket
	if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
	{
		cout << "connection error\n";
	}
	
	memset((char *) &si_me, 0, sizeof(si_me));
	
	si_me.sin_family = AF_INET;
	si_me.sin_port = htons(PORT);
	si_me.sin_addr.s_addr = htonl(INADDR_ANY);
	
	// bind socket to port
	if( bind(s , (struct sockaddr*)&si_me, sizeof(si_me) ) == -1)
	{
		cout << "binding error\n";
	}
	
	// получение данных
	while(1)
	{
		printf("Waiting for data...");
		fflush(stdout);
		
		if ((recv_len = recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *) &si_other, (socklen_t *)&slen)) == -1)
		{
			die("recvfrom()");
		}
		
		// вывод данных о клиенте
		printf("Received packet from %s:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
		printf("Data: %s\n" , buf);
		
	}

	return 0;
}