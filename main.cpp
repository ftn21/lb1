#include <iostream>
#include <string.h> // memset
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <chrono>
#include <functional>
#include <thread>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <iomanip>
#include <thread>
#include <mutex>
#include <random>
#include <math.h>
#include <algorithm>

using namespace std;


// константы
#define PORT 12346
#define IP_ADR ((in_addr_t)0x7F000001) // 127.0.0.1

// глобальные переменные
float lambda0 = 56;
float phi0 = 47;
int stls = 4;
mutex mtx;

// структуры и объединения

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
        unsigned short zapros_init_data : 1;
        unsigned short SDI : 1;

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
        unsigned short init_data : 1; // начальные данные 0-есть н.д., 1- нет н.д.
        unsigned short H_abc : 1;     // 0-есть, 1-нет
        unsigned short boost : 1;     // готовность ускорения
        unsigned short ready : 1;     // готовность
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
        unsigned short data : 20;
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
    ARINC429_SNS_DATA_DISCRETE_UNION hiest;
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

/* MIL-1553B */

// командное слово
#pragma pack(push,1)
union MIL1553_COMMAND_WORD_UNION 
{
    struct MIL1553_COMMAND_WORD_STRUCTURE
    {
        unsigned short sync_signal : 3; // снихро-сигнал

        // data
        unsigned short adress_OU : 5; // адрес ОУ
        unsigned short K : 1;
        unsigned short subadres : 5; // подадрес/режим управления
        unsigned short SD_num : 5;   //  число СД/код команды

        unsigned short P : 1;
    };
    unsigned int Word;
};
#pragma pack(pop)

// слово данных
#pragma pack(push,1)
union MIL1553_DATA_WORD_STRUCTURE_UNION
{
    struct MIL1553_DATA_WORD_STRUCTURE
    {
        unsigned short sync_signal : 3; // снихро-сигнал

        // data
        unsigned short data : 16; // данные

        unsigned short P : 1;
    };
    unsigned int Word;
};
#pragma pack(pop)

// ответное слово
#pragma pack(push, 1)
union MIL1553_ANSWER_WORD_UNION
{
    struct MIL1553_ANSWER_WORD_STRUCTURE
    {
        unsigned short sync_signal : 3; // снихро-сигнал

        // data
        unsigned short adress_OU : 5; // адрес ОУ
        unsigned short msg_error : 1;
        unsigned short OS_transf : 1; // передача ОС
        unsigned short service : 1;   // запрос на обслуживание
        unsigned short reserve : 3;
        unsigned short group_cmd : 1; // принята групповая команда
        unsigned short busy : 1;      // абонент занят
        unsigned short defect : 1;    // неисправность абонента
        unsigned short interface : 1; // принято управление интерфейсом
        unsigned short OU_defect : 1;

        unsigned short P : 1;
    };
    unsigned int Word;
};
#pragma pack(pop)

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
struct INS_DATA_float
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
INS_DATA_float ins_float;

/* таймер */

class Timer
{
public:
    Timer(){};
    void add(std::chrono::milliseconds delay, void (*callback)(int socket, struct sockaddr_in addr, void* data, int dsize),
            int s, struct sockaddr_in a, void* d, int ds)
    {
        auto  start = std::chrono::system_clock::now();
        auto  current = std::chrono::system_clock::now();
        while ((current - start) < delay) 
        {
            current = std::chrono::system_clock::now();
        }
        callback(s, a, d, ds);
        return;
    };
};

class Timer_forming
{
public:
    Timer_forming(){};
    void add(std::chrono::microseconds delay, void (*callback)())
    {
        auto  start = std::chrono::system_clock::now();
        auto  current = std::chrono::system_clock::now();
        while ((current - start) < delay) 
        {
            current = std::chrono::system_clock::now();
        }
        callback();
        return;
    };
};

// объявление функций

void send_data(int socket, struct sockaddr_in addr, void* data, int dsize) 
{
    char buf[sizeof(dsize)];
    memcpy(buf, data, sizeof(buf));
    if (sendto(socket, buf, dsize, 0, (struct sockaddr *)&addr, sizeof(addr)) == -1)
		{
			perror("sending error");
		}
    return;
};

void timer(std::chrono::seconds delay) {
    auto  start = std::chrono::system_clock::now();
        auto  current = std::chrono::system_clock::now();
        while ((current - start) < delay) 
        {
            current = std::chrono::system_clock::now();
        }
        return;
};

int codering(float num, int l, float diap) {
// num - число для кодирования
// l - количество значащих разрядов
int q = 0; // результат
float m = pow(num, 1/(l-1)); // основание сс
int res_m[l];
int temp, k = 0;

for(int counter = diap; counter >= 1; counter /= m)
{
    temp = num / counter;
    num = round(std::fmod(num, counter)); // остаток от деления
    res_m[k] = temp;
    k++;
}

std::reverse(res_m, res_m+l);
for (int i = 0; i < l; i++) 
{
    q += res_m[i]*pow(2, i-1);
}
return q;
};

/* ---------- ИНС ---------- */

// слово состояния ИНС
ARINC429_DISCRETE_UNION ins_state;
// слово данных ИНС
INS_DATA_STRUCTURE ins_data;

// самоконтроль
void ins_self_check() {
    cout << "Тест-контроль устройств ИНС 20 сек...\n";

    timer(std::chrono::seconds(5));
    bool check = 1;

    if (check == 1) {
        cout << "ИНС: Исправность ИНС.\n";
        cout << "ИНС: Нет начальных данных.\n";

        mtx.lock();
        ins_state.dsc.label = 210;
        ins_state.dsc.SDI = 01;
        ins_state.dsc.prep_ZK = 0;
        ins_state.dsc.control = 0;
        ins_state.dsc.navigation = 0;
        ins_state.dsc.gyrocopmassing = 0;
        ins_state.dsc.relaunch = 0;
        ins_state.dsc.prep_scale = 1; // таблица 4а?
        ins_state.dsc.heat = 1;
        ins_state.dsc.termostat = 1;
        ins_state.dsc.init_data = 1;
        ins_state.dsc.H_abc = 0;
        ins_state.dsc.boost = 0;
        ins_state.dsc.ready = 0;
        ins_state.dsc.P = 0;
        mtx.unlock();
    }
    //return 1; // 1 - исправность
};

bool ins_prepare() {    
    while ( (lambda0 == 0) & (phi0 == 0) ) {};

    mtx.lock();
    ins_state.dsc.label = 210;
    ins_state.dsc.SDI = 01;
    ins_state.dsc.prep_ZK = 0;
    ins_state.dsc.control = 0;
    ins_state.dsc.navigation = 0;
    ins_state.dsc.gyrocopmassing = 1; // 1 - ГК
    ins_state.dsc.relaunch = 0;
    ins_state.dsc.prep_scale = 1; // таблица 4а?
    ins_state.dsc.heat = 1;
    ins_state.dsc.termostat = 1;
    ins_state.dsc.init_data = 0; // 0 - есть н.д.
    ins_state.dsc.H_abc = 0;
    ins_state.dsc.boost = 0;
    ins_state.dsc.ready = 0;
    ins_state.dsc.P = 0;
    mtx.unlock();

    cout << "ИНС: подготовка завершена.\n" << endl;
    return 1;
};

void ins_navigation() {
    std::default_random_engine generator;
    std::normal_distribution<double> distribution(0, 0.002);
    mtx.lock();
    if ( (ins_float.latitude == 0) & (ins_float.longtitude == 0) ) {
        ins_float.longtitude = 47;
        ins_float.latitude = 56;
        ins_float.height = 2000;       
        ins_float.heading_true = 20000; 
        ins_float.pitch = 5;        
        ins_float.roll = 0;         
        ins_float.speed_NS = 1000;     
        ins_float.speed_WE = 1000;     
        ins_float.speed_vert = 70;   
        ins_float.accele_ax = 5;    
        ins_float.accele_az = 0;   
        ins_float.accel_ay = 0;     
    }
    else {
        ins_float.longtitude += distribution(generator);
        ins_float.latitude += distribution(generator);
        ins_float.height += distribution(generator);       
        ins_float.heading_true += distribution(generator); 
        ins_float.pitch += distribution(generator);        
        ins_float.roll += distribution(generator);         
        ins_float.speed_NS += distribution(generator);     
        ins_float.speed_WE += distribution(generator);     
        ins_float.speed_vert += distribution(generator);   
        ins_float.accele_ax += distribution(generator);    
        ins_float.accele_az += distribution(generator);   
        ins_float.accel_ay += distribution(generator);
    } 
    mtx.unlock();
};

void ins_forming_dataWord() {
    mtx.lock();
    // очистка мусора
    ins_data.latitude.Word = 0;
    ins_data.longtitude.Word = 0;
    ins_data.height.Word = 0;
    ins_data.heading_true.Word = 0;
    ins_data.pitch.Word = 0;
    ins_data.roll.Word = 0;
    ins_data.speed_NS.Word = 0;
    ins_data.speed_WE.Word = 0;
    ins_data.speed_vert.Word = 0;
    ins_data.accele_ax.Word = 0;
    ins_data.accele_az.Word = 0;
    ins_data.accel_ay.Word = 0;
    // широта
    ARINC426_BNR_UNION temporary;  // временная локальная переменная
    temporary.Word = 0;
    temporary.bnr.label = 0x13;
    temporary.bnr.data = 18;
    temporary.bnr.sign = 1;
    temporary.bnr.SSM = 2;
    temporary.bnr.P = 1;

    ins_data.latitude.Word = temporary.Word;

    // долгота
    temporary.Word = 0;
    temporary.bnr.label = 0x93;
    temporary.bnr.data = ins_float.latitude;
    temporary.bnr.sign = 1;
    temporary.bnr.SSM = 2;
    temporary.bnr.P = 1;

    ins_data.longtitude.Word = temporary.Word;

    // высота
    temporary.Word = 0;
    temporary.bnr.label = 0x8F;
    temporary.bnr.data = ins_float.longtitude;
    temporary.bnr.sign = 1;
    temporary.bnr.SSM = 2;
    temporary.bnr.P = 1;

    ins_data.height.Word = temporary.Word;

    // курс истинный
    temporary.Word = 0;
    temporary.bnr.label = 0x33;
    temporary.bnr.data = ins_float.heading_true;
    temporary.bnr.sign = 1;
    temporary.bnr.SSM = 2;
    temporary.bnr.P = 1;

    ins_data.heading_true.Word = temporary.Word;

    // угол тангажа
    temporary.Word = 0;
    temporary.bnr.label = 0x2B;
    temporary.bnr.data = ins_float.pitch;
    temporary.bnr.sign = 1;
    temporary.bnr.SSM = 2;
    temporary.bnr.P = 1;

    ins_data.pitch.Word = temporary.Word;

    // угол крена
    temporary.Word = 0;
    temporary.bnr.label = 0xAB;
    temporary.bnr.data = ins_float.roll;
    temporary.bnr.sign = 1;
    temporary.bnr.SSM = 2;
    temporary.bnr.P = 1;

    ins_data.roll.Word = temporary.Word;

    // скорость север/юг
    temporary.Word = 0;
    temporary.bnr.label = 0x6F;
    temporary.bnr.data = ins_float.speed_NS;
    temporary.bnr.sign = 1;
    temporary.bnr.SSM = 2;
    temporary.bnr.P = 1;

    ins_data.speed_NS.Word = temporary.Word;

    // скорость восток/запад
    temporary.Word = 0;
    temporary.bnr.label = 0xEF;
    temporary.bnr.data = ins_float.speed_WE;
    temporary.bnr.sign = 1;
    temporary.bnr.SSM = 2;
    temporary.bnr.P = 1;

    ins_data.speed_WE.Word = temporary.Word;

    // скорость вертикальная инерциальная
    temporary.Word = 0;
    temporary.bnr.label = 0xAF;
    temporary.bnr.data = ins_float.speed_vert;
    temporary.bnr.sign = 1;
    temporary.bnr.SSM = 2;
    temporary.bnr.P = 1;

    ins_data.speed_vert.Word = temporary.Word;

    // ускорение продольное ax
    temporary.Word = 0;
    temporary.bnr.label = 0x9B;
    temporary.bnr.data = ins_float.accele_ax;
    temporary.bnr.sign = 1;
    temporary.bnr.SSM = 2;
    temporary.bnr.P = 1;

    ins_data.accele_ax.Word = temporary.Word;

    // ускорение поперечное az
    temporary.Word = 0;
    temporary.bnr.label = 0x5B;
    temporary.bnr.data = ins_float.accele_az;
    temporary.bnr.sign = 1;
    temporary.bnr.SSM = 2;
    temporary.bnr.P = 1;

    ins_data.accele_az.Word = temporary.Word;

    // ускорение нормальное ay
    temporary.Word = 0;
    temporary.bnr.label = 0xDB;
    temporary.bnr.data = ins_float.accel_ay;
    temporary.bnr.sign = 1;
    temporary.bnr.SSM = 2;
    temporary.bnr.P = 1;

    ins_data.accel_ay.Word = temporary.Word;

    temporary.Word = 0;

    mtx.unlock();
};

void ins() {
    ins_self_check();     // самоконтроль

    mtx.lock();
    ins_state.dsc.label = 210;
    ins_state.dsc.SDI = 01;
    ins_state.dsc.prep_ZK = 0;
    ins_state.dsc.control = 0;
    ins_state.dsc.navigation = 0;
    ins_state.dsc.gyrocopmassing = 1; // 1 - ГК
    ins_state.dsc.relaunch = 0;
    ins_state.dsc.prep_scale = 1; // таблица 4а?
    ins_state.dsc.heat = 1;
    ins_state.dsc.termostat = 1;
    ins_state.dsc.init_data = 1;
    ins_state.dsc.H_abc = 0;
    ins_state.dsc.boost = 0;
    ins_state.dsc.ready = 0;
    ins_state.dsc.P = 0;
    mtx.unlock();

    cout << "ИНС: подготовка...\n";
    while (!ins_prepare()) {}  // подготовка

    timer(std::chrono::seconds(15));
    cout << "ИНС: готовность.\n";

    mtx.lock();
    ins_state.dsc.label = 210;
    ins_state.dsc.SDI = 01;
    ins_state.dsc.prep_ZK = 0;
    ins_state.dsc.control = 0;
    ins_state.dsc.navigation = 0;
    ins_state.dsc.gyrocopmassing = 1; // 1 - ГК
    ins_state.dsc.relaunch = 0;
    ins_state.dsc.prep_scale = 1; // таблица 4а?
    ins_state.dsc.heat = 1;
    ins_state.dsc.termostat = 1;
    ins_state.dsc.init_data = 0;
    ins_state.dsc.H_abc = 0;
    ins_state.dsc.boost = 0;
    ins_state.dsc.ready = 1; // 1 - готовность
    ins_state.dsc.P = 0;
    mtx.unlock();

    cout << "ИНС: переключение в режим навигации.\n";
    
    // forming data
    Timer_forming timer_data;
    bool a = 1;
    while (a) {
        timer_data.add(std::chrono::microseconds(2500), ins_forming_dataWord);
    }
};

/* ---------- СНС ---------- */

// слово признаков СРНС
ARINC429_SRNS_DISCRETE_UNION sns_state;

// самоконтроль
void sns_self_check() {
    cout << "СНС: тест-контроль устройств СНС 10 сек...\n";

    timer(std::chrono::seconds(3));
    bool check = 1;

    if (check == 1) {
        cout << "СНС: исправность, работа, синхронизация\n";
        mtx.lock();
        sns_state.dsc.label = 0273;  
        sns_state.dsc.zapros_init_data = 0;
        sns_state.dsc.sns_type = 0;
        sns_state.dsc.GPS_almanach = 0; 
        sns_state.dsc.GLONASS_almanach = 0;
        sns_state.dsc.work_mode = 2;
        sns_state.dsc.submode = 1; 
        sns_state.dsc.time_sign = 0;          
        sns_state.dsc.empty1 = 0;
        sns_state.dsc.diff_mode = 0;
        sns_state.dsc.failure = 0;
        sns_state.dsc.signal_limit = 0;     
        sns_state.dsc.coord_system = 0; 
        sns_state.dsc.empty2 = 0;
        sns_state.dsc.state_matrix = 0;
        sns_state.dsc.P = 1;
        mtx.unlock();
    }
};

void sns_navigation() {
    std::default_random_engine generator;
    std::normal_distribution<double> distribution(0, 0.02);
    mtx.lock();
    if ( (lambda0 == 0) & (phi0 == 0) ) {
        lambda0 = 47;
        phi0 = 56;
    }
    else {
        lambda0 += distribution(generator);
        phi0 += distribution(generator);
    }
    mtx.unlock();
};

void sns() {
    sns_self_check();
    timer(std::chrono::seconds(15));
    if (stls >= 4) {
        cout << "СНС: переключение в режим навигации.\n";
        sns_navigation();
        // формирование слов данных
    }   
}

/* ---------- Передача данных ИНС и СНС ---------- */
void send_ns_data() {
    struct sockaddr_in adr, oth;
    memset((char *)&adr, 0, sizeof(adr));
    adr.sin_family = AF_INET;
    adr.sin_port = htons(PORT);          // Host TO Network Short
    adr.sin_addr.s_addr = htonl(IP_ADR); // Host TO Network Long

    int s, i, slen = sizeof(adr);

    // создание соединения
    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) //IPPROTO_UDP
    {
        perror("connection error");
    }

    // bind socket to port
    if (bind(s, (struct sockaddr *)&adr, slen) == -1)
    {
        perror("binding error");
    }

    // send data
    Timer timer_ins;
    Timer timer_sns;
    bool a = 1;
    while (a) {
        mtx.lock();
        timer_ins.add(std::chrono::milliseconds(1000), send_data, s, adr, &ins_state.Word, sizeof(ins_state.Word));
        timer_ins.add(std::chrono::milliseconds(1000), send_data, s, adr, &ins_data, sizeof(ins_state));
        timer_sns.add(std::chrono::milliseconds(1000), send_data, s, adr, &sns_state.Word, sizeof(sns_state.Word));
        mtx.unlock();
    }
}

// реализация
int main(int argc, char *argv[])
{
    cout << "start\n" << endl;

    cout << "подача питания?\n" << endl;
    bool on = 0;
    string on_s = "";
    cin >> on_s;
    if (on_s == "0") {
        on = 0;
        return 1;
    }
    else if (on_s != "1") {
         cout << "введите 0 или 1.\n";
         return 1;
    }

    on = 1;
    
    std::thread t1(ins);
    std::thread t2(sns);
    std::thread t3(send_ns_data);
    t1.join();
    t2.join();
    t3.join();

    cout << endl;
    
}