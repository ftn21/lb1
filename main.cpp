#include <iostream>
#include<string.h>      //memset
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

using namespace std;

//константы
#define PORT 12346
#define IP_ADR ((in_addr_t) 0x7F000001) //127.0.0.1

//структуры

//глобальные переменные

//реализация
int main(int argc, char *argv[]) 
{
    cout << "hi\n";

    struct sockaddr_in adr, oth;
    memset((char *) &adr, 0, sizeof(adr));
    adr.sin_family = AF_INET;
    adr.sin_port = htons(PORT);             //Host TO Network Short
    adr.sin_addr.s_addr = htonl(IP_ADR);    //Host TO Network Long

    //int s = sizeof(adr), recv_len;
    int s, i, slen = sizeof(oth) , recv_len;

    //создание соединения
    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)  //IPPROTO_UDP
	{
		cout << "connection error\n";
	}

    //bind socket to port
	if( bind(s , (struct sockaddr*)&adr, sizeof(adr) ) == -1)
	{
		cout << "binding error\n";
	}
}