#include "mbed.h"
#include "RCS620S.h"
#include "millis.h"

//#define COMMAND_TIMEOUT  400
#define PUSH_TIMEOUT    2100
//#define POLLING_INTERVAL 500

// RCS620S
#define COMMAND_TIMEOUT               400
#define POLLING_INTERVAL              500
//#define RCS620S_MAX_CARD_RESPONSE_LEN 30
 
// FeliCa Service/System Code
#define CYBERNE_SYSTEM_CODE           0x0003
#define COMMON_SYSTEM_CODE            0xFE00
#define PASSNET_SERVICE_CODE          0x090F
#define EDY_SERVICE_CODE              0x170F
#define NANACO_SERVICE_CODE           0x564F
#define WAON_SERVICE_CODE             0x680B

int requestService(uint16_t serviceCode);
int readEncryption(uint16_t serviceCode, uint8_t blockNumber, uint8_t *buf);
void printBalanceLCD(char *card_name, uint32_t *balance);

DigitalOut led(LED1);
RCS620S rcs620s(RCS620S_TX, RCS620S_RX);

void parse_history(uint8_t *buf)
{
        for(int i = 0; i < 16; i++) {
            printf("%02X ", buf[i]);
        }
        printf("\n");

        printf("機種種別: ");
        switch (buf[0]) {
            case 0x03:
                printf("のりこし精算機\n");
                break;
            case 0x12:
                printf("自動券売機\n");
                break;
            case 0x16:
                printf("自動改札機\n");
                break;
            case 0x17:
                printf("簡易改札機\n");
                break;
            case 0x18:
                printf("駅務機器\n");
                break;
            case 0x46:
                printf("ビューアルッテ端末\n");
                break;
            case 0xc7:
            case 0xc8:
                printf("物販端末\n");
                break;
            default:
                printf("不明\n");
                break;
        }

        printf("利用種別: ");
        switch (buf[1]) {
            case 0x01:
                printf("自動改札出場\n");
                break;
            case 0x02:
                printf("SFチャージ\n");
                break;
            case 0x03:
                printf("きっぷ購入\n");
                break;
            case 0x07:
                printf("新規\n");
                break;
            case 0x14:
                printf("オートチャージ\n");
                break;
            case 0x46:
                printf("物販\n");
                break;
            default:
                printf("不明\n");
                break;
        }

        if (buf[2] != 0) {
            printf("支払種別: ");
            switch (buf[2]) {
                case 0x02:
                    printf("VIEW\n");
                    break;
                case 0x0B:
                    printf("PiTaPa\n");
                    break;
                case 0x0d:
                    printf("オートチャージ対応PASMO\n");
                    break;
                case 0x3f:
                    printf("モバイルSuica(VIEW決済以外)\n");
                    break;
                default:
                    printf("不明\n");
                    break;
            }
        }

        printf("処理日付: %d/%02d/%02d\n", 2000+(buf[4]>>1), ((buf[4]&1)<<3 | ((buf[5]&0xe0)>>5)), buf[5]&0xf);
        setlocale(LC_NUMERIC,"ja_JP.utf8");
        printf("残額: %d円\n", (buf[11]<<8) + buf[10]); 
        printf("\n");
}

#if 1

int main()
{
    printf("\n*** RCS620S テストプログラム ***\n\n");
    rcs620s.initDevice();

    while(1) {
        uint32_t balance;
        uint8_t buf[RCS620S_MAX_CARD_RESPONSE_LEN];
        
        rcs620s.timeout = COMMAND_TIMEOUT;
        
        // サイバネ領域
        if(rcs620s.polling(CYBERNE_SYSTEM_CODE)){
            // Suica PASMO
            if(requestService(PASSNET_SERVICE_CODE)){
                for (int i = 0; i < 20; i++) {
                    if(readEncryption(PASSNET_SERVICE_CODE, i, buf) && buf[12] != 0){
#if 0
                        // Little Endianで入っているPASSNETの残高を取り出す
                        balance = buf[23];                  // 11 byte目
                        balance = (balance << 8) + buf[22]; // 10 byte目
                        // 残高表示
                        //printBalanceLCD("PASSNET", &balance);
#else
                        parse_history(&buf[12]);
#endif
                    }
                }
            }
        }
        
        // 共通領域
        else if(rcs620s.polling(COMMON_SYSTEM_CODE)){
            // Edy
            if(requestService(EDY_SERVICE_CODE)){
            if(readEncryption(EDY_SERVICE_CODE, 0, buf)){
                // Big Endianで入っているEdyの残高を取り出す
                balance = buf[26];                  // 14 byte目
                balance = (balance << 8) + buf[27]; // 15 byte目
                // 残高表示
                printBalanceLCD("Edy", &balance);
            }
            }
            
            // nanaco
            if(requestService(NANACO_SERVICE_CODE)){
            if(readEncryption(NANACO_SERVICE_CODE, 0, buf)){
                // Big Endianで入っているNanacoの残高を取り出す
                balance = buf[17];                  // 5 byte目
                balance = (balance << 8) + buf[18]; // 6 byte目
                balance = (balance << 8) + buf[19]; // 7 byte目
                balance = (balance << 8) + buf[20]; // 8 byte目
                // 残高表示
                printBalanceLCD("nanaco", &balance);
            }
            }
            
            // waon
            if(requestService(WAON_SERVICE_CODE)){
            if(readEncryption(WAON_SERVICE_CODE, 1, buf)){
                // Big Endianで入っているWaonの残高を取り出す
                balance = buf[17];                  // 21 byte目
                balance = (balance << 8) + buf[18]; // 22 byte目
                balance = (balance << 8) + buf[19]; // 23 byte目
                balance = balance & 0x7FFFE0;       // 残高18bit分のみ論理積で取り出す
                balance = balance >> 5;             // 5bit分ビットシフト
                // 残高表示
                printBalanceLCD("waon", &balance);
            }
            }
        }
        
        // デフォルト表示
        else{
/*
            LCD.clear();
            LCD.move(0);
            LCD.print("Touch");
            LCD.move(0x44);
            LCD.print("Card");
*/
        }
        
        rcs620s.rfOff();
        led = !led;
        thread_sleep_for(POLLING_INTERVAL);
    }
}
#else
int main() {
    AnalogIn ain(dp2);

    led = 1;
    rcs620s.initDevice();
    rcs620s.timeout = COMMAND_TIMEOUT;


    int pool = 1;
    while(1) {
        float f = ain.read();
        //printf("val = %5.2f\n", f);
        rcs620s.timeout = COMMAND_TIMEOUT;
        
        // サイバネ領域
        if(rcs620s.polling(CYBERNE_SYSTEM_CODE)){

        }
        pool = (int)(f * 100 + 1);
        thread_sleep_for(pool);
        rcs620s.rfOff();
        thread_sleep_for(pool);
        /*
        pool += 10;
        if (pool > 1000) {
            pool = 1;
        }
        */
    }
}

#endif

// request service
int requestService(uint16_t serviceCode){
    int ret;
    uint8_t buf[RCS620S_MAX_CARD_RESPONSE_LEN];
    uint8_t responseLen = 0;
    
    buf[0] = 0x02;
    memcpy(buf + 1, rcs620s.idm, 8);
    buf[9] = 0x01;
    buf[10] = (uint8_t)((serviceCode >> 0) & 0xff);
    buf[11] = (uint8_t)((serviceCode >> 8) & 0xff);
    
    ret = rcs620s.cardCommand(buf, 12, buf, &responseLen);
    
    if(!ret || (responseLen != 12) || (buf[0] != 0x03) ||
        (memcmp(buf + 1, rcs620s.idm, 8) != 0) || ((buf[10] == 0xff) && (buf[11] == 0xff))) {
        return 0;
    }
    
    return 1;
}
 
int readEncryption(uint16_t serviceCode, uint8_t blockNumber, uint8_t *buf){
    int ret;
    uint8_t responseLen = 0;
    
    buf[0] = 0x06;
    memcpy(buf + 1, rcs620s.idm, 8);
    buf[9] = 0x01; // サービス数
    buf[10] = (uint8_t)((serviceCode >> 0) & 0xff);
    buf[11] = (uint8_t)((serviceCode >> 8) & 0xff);
    buf[12] = 0x01; // ブロック数
    buf[13] = 0x80;
    buf[14] = blockNumber;
    
    ret = rcs620s.cardCommand(buf, 15, buf, &responseLen);
    
    if (!ret || (responseLen != 28) || (buf[0] != 0x07) ||
        (memcmp(buf + 1, rcs620s.idm, 8) != 0)) {
        return 0;
    }

    return 1;
}

void printBalanceLCD(char *card_name, uint32_t *balance){
    printf("%s: %d\n", card_name, *balance);
}
