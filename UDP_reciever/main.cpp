/*
	Принимающий интерфейс
*/
#include <iostream>
#include <stdio.h>
#include <string.h>  
#include <stdlib.h> 
#include <arpa/inet.h>
#include <sys/socket.h>
#include <math.h>
#include <iterator>
#include <vector>

using namespace std;

#define BUFLEN 512	// максимальная длина буфера
#define PORT 12346	// порт


/*  структуры и объединения  */

/* ARINC-429 */

// BNR
#pragma pack(push,1)
union ARINC426_BNR_UNION {
    struct ARINC429_BNR_STRUCTURE
    {
        unsigned short label : 8; // он же адрес

        // data
        unsigned int data : 20; // 65536 ??
        unsigned short sign : 1;

        unsigned short SSM : 2;
        unsigned short P : 1;
    } bnr;
    unsigned int Word;
}; 
#pragma pack(pop)

// BCD
#pragma pack(push,1)
union ARINC429_BCD_UNION
{
    struct ARINC429_BCD_STRUCTURE
    {
        unsigned short label : 8; // он же адрес
        unsigned short SDI : 2;
        unsigned short empt1 : 1; // не использующийся разряд(ы)

        // data
        unsigned short sec : 6;   // 32 ??
        unsigned short min : 6;   // 32 ??
        unsigned short hrs : 5;   // 16 ??
        unsigned short empt2 : 1; // не использующийся разряд(ы)

        unsigned short SSM : 2;
        unsigned short P : 1;
    } bcd;
    unsigned int Word;
};
#pragma pack(pop)

// DISCRETE
#pragma pack(push,1)
union ARINC429_DISCRETE_UNION
{
    struct ARINC429_DISCRETE_STRUCTURE
    {
        unsigned short label : 8; // он же адрес 
        unsigned short SDI : 2;

        // data
        unsigned short prep_ZK : 1; // подготовка по ЗК
        unsigned short control : 1;
        unsigned short navigation : 1;
        unsigned short gyrocopmassing : 1; // 0-ЗК, 1-ГК
        unsigned short empt1 : 1;          // не использующийся разряд(ы)
        unsigned short relaunch : 1;
        unsigned short prep_scale : 3;
        unsigned short heat : 1;      // исправность обогрева
        unsigned short termostat : 1; // термостатирование
        unsigned short init_data : 1; // начальные данные 0-есть н.д., 1- нет н.д. 20
        unsigned short H_abc : 1;     // 0-есть, 1-нет
        unsigned short seviceability: 1;  // исправность
        unsigned short boost : 1;     // готовность ускорения
        unsigned short ready : 1;     // готовность 23
        unsigned short empt2 : 3;

        unsigned short SSM : 2;
        unsigned short P : 1;
    } dsc;
    unsigned int Word;
};
#pragma pack(pop)

// SNS SRNS DISCRETE
#pragma pack(push,1)
union ARINC429_SRNS_DISCRETE_UNION
{
    struct ARINC429_SRNS_DISCRETE_STRUCTURE
    {
        unsigned short label : 8; // он же адрес 
        unsigned short zapros_init_data : 1;
        unsigned short sns_type : 3;

        // data
        unsigned short GPS_almanach : 1; 
        unsigned short GLONASS_almanach : 1;
        unsigned short work_mode : 2;
        unsigned short submode : 1; 
        unsigned short time_sign : 1;          
        unsigned short empty1 : 2;
        unsigned short diff_mode : 1;
        unsigned short failure : 1;
        unsigned short signal_limit : 1;     
        unsigned short coord_system : 2; 
        unsigned short empty2 : 4;
        unsigned short state_matrix : 2;

        unsigned short P : 1;
    } dsc;
    unsigned int Word;
};
#pragma pack(pop)

// SNS DATE DISCRETE
#pragma pack(push,1)
union ARINC429_SNS_DATE_DISCRETE_UNION
{
    struct ARINC429_SNS_DATE_DISCRETE_STRUCTURE
    {
        unsigned short label : 8;
        unsigned short empty1 : 2;
        unsigned short year : 4;
        unsigned short empty2 : 4;
        unsigned short month : 4; 
        unsigned short day : 4;
        unsigned short empty3 : 3;
        unsigned short state_matrix : 2;
        unsigned short P : 1;
    } dsc;
    unsigned int Word;
};
#pragma pack(pop)

// SNS DATA DISCRETE
#pragma pack(push,1)
union ARINC429_SNS_DATA_DISCRETE_UNION
{
    struct ARINC429_SNS_DATA_DISCRETE_STRUCTURE
    {
        unsigned short label : 8;
        unsigned int data : 20;
        unsigned short empty : 3;
        unsigned short P : 1;
    } dsc;
    unsigned int Word;
};
#pragma pack(pop)

// структура данных СНС
#pragma pack(push,1)
struct SNS_DATA_STRUCTURE
{
    ARINC429_SNS_DATA_DISCRETE_UNION hiegt;
    ARINC429_SNS_DATA_DISCRETE_UNION HDOP;
    ARINC429_SNS_DATA_DISCRETE_UNION VDOP;
    ARINC429_SNS_DATA_DISCRETE_UNION PU;  // путевой угол
    ARINC429_SNS_DATA_DISCRETE_UNION R;  // текущая широта
    ARINC429_SNS_DATA_DISCRETE_UNION Rt;  // текущая широта (точно)
    ARINC429_SNS_DATA_DISCRETE_UNION L;  // текущая долгота
    ARINC429_SNS_DATA_DISCRETE_UNION Lt;  // текущая долгота (точно)
    ARINC429_SNS_DATA_DISCRETE_UNION delay;  // задержка выдачи обновленных НП
    ARINC429_SNS_DATA_DISCRETE_UNION UTC_time;  // текущее время UTC
    ARINC429_SNS_DATA_DISCRETE_UNION UTC_time_minor;  // текущее время UTC (младшие разряды)
    ARINC429_SNS_DATA_DISCRETE_UNION Vh;  // вертикальная скорость
    ARINC429_SNS_DATE_DISCRETE_UNION date;  // дата
    ARINC429_SRNS_DISCRETE_UNION srns;  // признаки СРНС
};
#pragma pack(pop)

// структура хранения данных ИНС
struct SNS_DATA_decoded
{
    float hiegt;
    float HDOP;
    float VDOP;
    float PU;  // путевой угол
    float R;  // текущая широта
    float Rt;  // текущая широта (точно)
    float L;  // текущая долгота
    float Lt;  // текущая долгота (точно)
    float delay;  // задержка выдачи обновленных НП
    float UTC_time;  // текущее время UTC
    float UTC_time_minor;  // текущее время UTC (младшие разряды)
    float Vh;  // вертикальная скорость
    int date_year;  // дата
    int date_month;  // дата
    int date_day;  // дата
    //float srns;  // признаки СРНС
};

// структура данных ИНС
// BNR
#pragma pack(push,1)
struct INS_DATA_STRUCTURE
{
    ARINC426_BNR_UNION latitude;     // широта
    ARINC426_BNR_UNION longtitude;   // долгота
    ARINC426_BNR_UNION height;       // высота
    ARINC426_BNR_UNION heading_true; // курс истинный
    ARINC426_BNR_UNION pitch;        // тангаж
    ARINC426_BNR_UNION roll;         // крен
    ARINC426_BNR_UNION speed_NS;     // скорость свер/юг
    ARINC426_BNR_UNION speed_WE;     // скорость восток/запад
    ARINC426_BNR_UNION speed_vert;   // скорость вертикальная инерциальная
    ARINC426_BNR_UNION accele_ax;    // ускорение продольное, ax
    ARINC426_BNR_UNION accele_az;    // ускорение поперечное, az
    ARINC426_BNR_UNION accel_ay;     // ускорение нормальное, ay
};
#pragma pack(pop)

// структура хранения данных ИНС
struct INS_DATA_decoded
{
    float latitude;     // широта
    float longtitude;   // долгота
    float height;       // высота
    float heading_true; // курс истинный
    float pitch;        // тангаж
    float roll;         // крен
    float speed_NS;     // скорость свер/юг
    float speed_WE;     // скорость восток/запад
    float speed_vert;   // скорость вертикальная инерциальная
    float accele_ax;    // ускорение продольное, ax
    float accele_az;    // ускорение поперечное, az
    float accel_ay;     // ускорение нормальное, ay
};

/*  объявление функций  */

// считывание беззнакового числа
uint32_t bit32u(const uint8_t *buff, int32_t pos, int32_t len)
{
    uint32_t bits = 0;
    for (int32_t i = pos; i < pos + len; i++) {
        bits = (bits << 1) + ((buff[i / 8] >> (7 - i % 8)) & 1u);
    }
    return bits;
}

// двоичное декодирование
double* decodering2 (int dec, int max_digit) {
    double* bin = new double[max_digit]();
    for (int i = 0 ; dec > 0; i++)
    {
		bin[max_digit -(i+1)] = (dec % 2) ;
		dec /= 2;
    }
    return bin;
}

// декодирование
double decodering(double max_value, int max_digit, int digit, int dec) {
	double* arr = decodering2(dec, max_digit);
	double sum = 0;
	for (int i = 0; i <= digit - 1; i++){
		sum = sum + round(arr[i]*(max_value / pow(2, i)));
	}
	return sum;
}

void die(char *s)
{
	perror(s);
	exit(1);
}

// глобальные переменные
ARINC429_DISCRETE_UNION ins_state;  // слово состояния ИНС

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
    printf("Waiting for data...");
	while(1)
	{	
		if ((recv_len = recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *) &si_other, (socklen_t *)&slen)) == -1)
		{
			die("recvfrom()");
		}
		
        vector<uint8_t> data(buf, buf + sizeof(buf));
		int label = bit32u(&data[0], 0, 8);
		if (label == 210) {
			printf("Получено слово состояния ИНС.\n");

            ins_state.dsc.label = bit32u(&data[0], 0, 8);
            ins_state.dsc.SDI = bit32u(&data[0], 9, 2);
            ins_state.dsc.prep_ZK = bit32u(&data[0], 11, 1);
            ins_state.dsc.control = bit32u(&data[0], 12, 1);
            ins_state.dsc.navigation = bit32u(&data[0], 13, 1);
            ins_state.dsc.gyrocopmassing = bit32u(&data[0], 14, 1);
            ins_state.dsc.relaunch = bit32u(&data[0], 16, 1);
            ins_state.dsc.prep_scale = bit32u(&data[0], 17, 3); // таблица 4а?
            ins_state.dsc.heat = bit32u(&data[0], 20, 1);
            ins_state.dsc.termostat = bit32u(&data[0], 21, 1);
            ins_state.dsc.init_data = bit32u(&data[0], 22, 1);
            ins_state.dsc.H_abc = bit32u(&data[0], 23, 1);
            ins_state.dsc.seviceability = bit32u(&data[0], 24, 1);
            ins_state.dsc.boost = bit32u(&data[0], 25, 1);
            ins_state.dsc.ready = bit32u(&data[0], 26, 1); // 1 - готовность
            
            if (ins_state.dsc.ready == 0) {
                printf("ИНС: идёт подготовка...");
            }
            else if (ins_state.dsc.ready == 1) {
                printf("ИНС: готовность.");
            }
		}
		
	}

	return 0;
}