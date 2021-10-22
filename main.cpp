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
        unsigned short SDI : 2;

        // data
        unsigned int higehst : 18; // 65536 ??
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
};

/* ---------- СНС ---------- */
// самоконтроль
void sns_self_check() {
    cout << "СНС: тест-контроль устройств ИНС 20 сек...\n";

    timer(std::chrono::seconds(5));
    bool check = 1;

    if (check == 1) {
        cout << "СНС: исправность, работа, синхронизация\n";
    }
};

void sns_navigation() {
    std::default_random_engine generator;
    std::normal_distribution<double> distribution(0, 0.02);
    mtx.lock();
    if ( (lambda0 == 0) & (phi0 == 0) ) {
        lambda0 = 47;
        phi0 = 56;
        
        // широта
        ARINC426_BNR_UNION temporary;  // временная локальная переменная
        // добавить очистку temporary (зануление всех полей)
        temporary.bnr.label = 8;
        temporary.bnr.SDI = 2;
        temporary.bnr.higehst = 18;
        temporary.bnr.sign = 1;
        temporary.bnr.SSM = 2;
        temporary.bnr.P = 1;

        ins_data.latitude.Word = temporary.Word;

        // долгота
        temporary.bnr.label = 8;
        temporary.bnr.SDI = 2;
        temporary.bnr.higehst = 18;
        temporary.bnr.sign = 1;
        temporary.bnr.SSM = 2;
        temporary.bnr.P = 1;

        ins_data.longtitude.Word = temporary.Word;

        // высота
        temporary.bnr.label = 8;
        temporary.bnr.SDI = 2;
        temporary.bnr.higehst = 18;
        temporary.bnr.sign = 1;
        temporary.bnr.SSM = 2;
        temporary.bnr.P = 1;

        ins_data.height.Word = temporary.Word;

        // курс истинный
        temporary.bnr.label = 8;
        temporary.bnr.SDI = 2;
        temporary.bnr.higehst = 18;
        temporary.bnr.sign = 1;
        temporary.bnr.SSM = 2;
        temporary.bnr.P = 1;

        ins_data.heading_true.Word = temporary.Word;

        // угол тангажа
        temporary.bnr.label = 8;
        temporary.bnr.SDI = 2;
        temporary.bnr.higehst = 18;
        temporary.bnr.sign = 1;
        temporary.bnr.SSM = 2;
        temporary.bnr.P = 1;

        ins_data.pitch.Word = temporary.Word;

        // угол крена
        temporary.bnr.label = 8;
        temporary.bnr.SDI = 2;
        temporary.bnr.higehst = 18;
        temporary.bnr.sign = 1;
        temporary.bnr.SSM = 2;
        temporary.bnr.P = 1;

        ins_data.roll.Word = temporary.Word;

        // скорость север/юг
        temporary.bnr.label = 8;
        temporary.bnr.SDI = 2;
        temporary.bnr.higehst = 18;
        temporary.bnr.sign = 1;
        temporary.bnr.SSM = 2;
        temporary.bnr.P = 1;

        ins_data.speed_NS.Word = temporary.Word;

        // скорость восток/запад
        temporary.bnr.label = 8;
        temporary.bnr.SDI = 2;
        temporary.bnr.higehst = 18;
        temporary.bnr.sign = 1;
        temporary.bnr.SSM = 2;
        temporary.bnr.P = 1;

        ins_data.speed_WE.Word = temporary.Word;

        // скорость вертикальная инерциальная
        temporary.bnr.label = 8;
        temporary.bnr.SDI = 2;
        temporary.bnr.higehst = 18;
        temporary.bnr.sign = 1;
        temporary.bnr.SSM = 2;
        temporary.bnr.P = 1;

        ins_data.speed_vert.Word = temporary.Word;

        // ускорение продольное ax
        temporary.bnr.label = 8;
        temporary.bnr.SDI = 2;
        temporary.bnr.higehst = 18;
        temporary.bnr.sign = 1;
        temporary.bnr.SSM = 2;
        temporary.bnr.P = 1;

        ins_data.accele_ax.Word = temporary.Word;

        // ускорение поперечное az
        temporary.bnr.label = 8;
        temporary.bnr.SDI = 2;
        temporary.bnr.higehst = 18;
        temporary.bnr.sign = 1;
        temporary.bnr.SSM = 2;
        temporary.bnr.P = 1;

        ins_data.accele_az.Word = temporary.Word;

        // ускорение нормальное ay
        temporary.bnr.label = 8;
        temporary.bnr.SDI = 2;
        temporary.bnr.higehst = 18;
        temporary.bnr.sign = 1;
        temporary.bnr.SSM = 2;
        temporary.bnr.P = 1;

        ins_data.accel_ay.Word = temporary.Word;

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
    bool a = 1;
    while (a) {
        mtx.lock();
        timer_ins.add(std::chrono::milliseconds(1000), send_data, s, adr, &ins_state.Word, sizeof(ins_state.Word));
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

    cout << to_string(lambda0) << endl;
    cout << to_string(phi0) << endl;
    
    std::thread t1(ins);
    std::thread t2(sns);
    std::thread t3(send_ns_data);
    t1.join();
    t2.join();
    t3.join();

    cout << to_string(lambda0) << endl;
    cout << to_string(phi0) << endl;

    cout << endl;



    
}