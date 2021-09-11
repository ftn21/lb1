#include <iostream>
#include <string.h> //memset
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <chrono>
#include <functional>

#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>

using namespace std;

//константы
#define PORT 12346
#define IP_ADR ((in_addr_t)0x7F000001) //127.0.0.1

//структуры и объединения

/* ARINC-429 */

// BNR
typedef union ARINC429_BNR_STRUCTURE
{
    unsigned short label : 8; //он же адрес
    unsigned short SDI : 2;

    //data
    unsigned int higehst : 18; //65536 ??
    unsigned short sign : 1;

    unsigned short SSM : 2;
    unsigned short P : 1;
} arinc429_bnr;

// BCD
typedef union ARINC429_BCD_STRUCTURE
{
    unsigned short label : 8; //он же адрес
    unsigned short SDI : 2;
    unsigned short empt1 : 1; //не использующийся разряд(ы)

    //data
    unsigned short sec : 6;   //32 ??
    unsigned short min : 6;   //32 ??
    unsigned short hrs : 5;   //16 ??
    unsigned short empt2 : 1; //не использующийся разряд(ы)

    unsigned short SSM : 2;
    unsigned short P : 1;
} arinc429_bcd;

// DISCRETE
typedef union ARINC429_BCD_DISCRETE
{
    unsigned short label : 8; //он же адрес
    unsigned short SDI : 2;

    //data
    unsigned short prep_ZK : 1; //подготовка по ЗК
    unsigned short control : 1;
    unsigned short navigation : 1;
    unsigned short gyrocopmassing : 1; //0-ЗК, 1-ГК
    unsigned short empt1 : 1;          //не использующийся разряд(ы)
    unsigned short relaunch : 1;
    unsigned short prep_scale : 3;
    unsigned short heat : 1;      //исправность обогрева
    unsigned short termostat : 1; //термостатирование
    unsigned short init_data : 1; //начальные данные 0-есть н.д., 1- нет н.д.
    unsigned short H_abc : 1;     //0-есть, 1-нет
    unsigned short boost : 1;     //готовность ускорения
    unsigned short ready : 1;     //готовность
    unsigned short empt2 : 3;

    unsigned short SSM : 2;
    unsigned short P : 1;
} arinc429_discrete;

/* MIL-1553B */

//командное слово
typedef union MIL1553_COMMAND_WORD_STRUCTURE
{
    unsigned short sync_signal : 3; //снихро-сигнал

    //data
    unsigned short adress_OU : 5; //адрес ОУ
    unsigned short K : 1;
    unsigned short subadres : 5; //подадрес/режим управления
    unsigned short SD_num : 5;   // число СД/код команды

    unsigned short P : 1;
} mil1553_command;

//слово данных
typedef union MIL1553_DATA_WORD_STRUCTURE
{
    unsigned short sync_signal : 3; //снихро-сигнал

    //data
    unsigned short data : 16; //данные

    unsigned short P : 1;
} mil1553_data;

//ответное слово
typedef union MIL1553_ANSWER_WORD_STRUCTURE
{
    unsigned short sync_signal : 3; //снихро-сигнал

    //data
    unsigned short adress_OU : 5; //адрес ОУ
    unsigned short msg_error : 1;
    unsigned short OS_transf : 1; //передача ОС
    unsigned short service : 1;   //запрос на обслуживание
    unsigned short reserve : 3;
    unsigned short group_cmd : 1; //принята групповая команда
    unsigned short busy : 1;      //абонент занят
    unsigned short defect : 1;    //неисправность абонента
    unsigned short interface : 1; //принято управление интерфейсом
    unsigned short OU_defect : 1;

    unsigned short P : 1;
} mil1553_answer;

/* таймер */

class Timer
{
public:
    Timer(){};
    void add(std::chrono::milliseconds delay,
             std::function<void()> callback);
};

//глобальные переменные

//реализация
int main(int argc, char *argv[])
{
    cout << "start\n";

    struct sockaddr_in adr, oth;
    memset((char *)&adr, 0, sizeof(adr));
    adr.sin_family = AF_INET;
    adr.sin_port = htons(PORT);          //Host TO Network Short
    adr.sin_addr.s_addr = htonl(IP_ADR); //Host TO Network Long

    int s, i, slen = sizeof(adr);

    //создание соединения
    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) //IPPROTO_UDP
    {
        perror("connection error");
    }

    //bind socket to port
    if (bind(s, (struct sockaddr *)&adr, slen) == -1)
    {
        perror("binding error");
    }

    //data
    mil1553_data mil_pack1;
    mil_pack1.sync_signal = 1;
    mil_pack1.data = 5;
    mil_pack1.P = 1;

    //char* pack_ptr = (char*)&mil_pack1;

    //send data
    char buf[20] = {char(*(char*)&mil_pack1)};
    if (sendto(s, buf, 3, 0, (struct sockaddr *)&adr, sizeof(adr)) == -1)
		{
			perror("sending error");
		} 
}