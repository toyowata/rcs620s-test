#include <string.h>

#include "mbed.h"
#include "RCS620S.h"
#include "AS289R2.h"

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

#define PRINT_ENTRIES                 10    // Max. 20

int requestService(uint16_t serviceCode);
int readEncryption(uint16_t serviceCode, uint8_t blockNumber, uint8_t *buf);
void printBalanceLCD(const char *card_name, uint32_t *balance);

DigitalOut led(LED1);
RCS620S rcs620s(RCS620S_TX, RCS620S_RX);
AS289R2 tp(AS289R2_TX, AS289R2_RX);

void parse_history(uint8_t *buf)
{
    char info[80], info2[20];

    for(int i = 0; i < 16; i++) {
        printf("%02X ", buf[i]);
    }
    printf("\n");

    sprintf(info, "機種種別: ");
    switch (buf[0]) {
        case 0x03:
            strcat(info, "のりこし精算機\r");
            break;
        case 0x12:
            strcat(info, "自動券売機\r");
            break;
        case 0x15:
            strcat(info, "定期券発売機\r");
            break;            
        case 0x16:
            strcat(info, "自動改札機\r");
            break;
        case 0x17:
            strcat(info, "簡易改札機\r");
            break;
        case 0x18:
            strcat(info, "駅務機器\r");
            break;
        case 0x46:
            strcat(info, "ビューアルッテ端末\r");
            break;
        case 0xc7:
        case 0xc8:
            strcat(info, "物販端末\r");
            break;
        default:
            strcat(info, "不明\r");
            break;
    }
    printf("%s", info);
    // tp.printf("%s", info);

    sprintf(info, "利用種別: ");
    switch (buf[1]) {
        case 0x01:
            strcat(info, "自動改札出場\r");
            break;
        case 0x02:
            strcat(info, "SFチャージ\r");
            break;
        case 0x03:
            strcat(info, "きっぷ購入\r");
            break;
        case 0x04:
            strcat(info, "磁気券精算\r");
            break;
        case 0x05:
            strcat(info, "乗越精算\r");
            break;
        case 0x06:
            strcat(info, "窓口精算\r");
            break;
        case 0x07:
            strcat(info, "新規\r");
            break;
        case 0x08:
            strcat(info, "控除\r");
            break;
        case 0x0D:
            strcat(info, "バス\r");
            break;
        case 0x0F:
            strcat(info, "バス\r");
            break;
        case 0x14:
            strcat(info, "オートチャージ\r");
            break;
        case 0x46:
            strcat(info, "物販\r");
            break;
        default:
            strcat(info, "不明\r");
            break;
    }
    printf("%s", info);
    tp.printf("%s", info);

    if (buf[2] != 0) {
        sprintf(info, "支払種別: ");
        switch (buf[2]) {
            case 0x02:
                strcat(info, "VIEW\r");
                break;
            case 0x0B:
                strcat(info, "PiTaPa\r");
                break;
            case 0x0d:
                strcat(info, "オートチャージ対応PASMO\r");
                break;
            case 0x3f:
                strcat(info, "モバイルSuica\r");
                break;
            default:
                strcat(info, "不明\r");
                break;
        }
        printf("%s", info);
        tp.printf("%s", info);
    }

    if (buf[1] == 0x01 || buf[1] == 0x14) {
        sprintf(info, "入出場種別: ");
        switch (buf[3]) {
            case 0x01:
                strcat(info, "入場");
                break;
            case 0x02:
                strcat(info, "出場");
                break;
            case 0x03:
                strcat(info, "定期入場");
                break;
            case 0x04:
                strcat(info, "定期出場");
                break;
            case 0x0E:
                strcat(info, "窓口出場");
                break;
            case 0x0F:
                strcat(info, "バス入出場");
                break;
            case 0x12:
                strcat(info, "料金定期");
                break;
            case 0x17:
            case 0x1D:
                strcat(info, "乗継割引");
                break;
            case 0x21:
                strcat(info, "バス等乗継割引");
                break;
            default:
                strcat(info, "不明");
                break;
        }
        printf("%s\r", info);
        tp.printf("%s\r", info);
    }

    sprintf(info, "処理日付: %d/%02d/%02d", 2000+(buf[4]>>1), ((buf[4]&1)<<3 | ((buf[5]&0xe0)>>5)), buf[5]&0xf);
    if (buf[1] == 0x46) {   // 物販
        sprintf(info2, " %02d:%02d:%02d", (buf[6] & 0xF8) >> 3, ((buf[6] & 0x7) >> 5) | ((buf[7] & 0xe0) >> 5), (buf[7] & 0x1f));
        strcat(info, info2);
    }
    strcat(info, "\r");
    printf("%s", info);
    tp.printf("%s", info);

    sprintf(info, "残額: %d円\r\r", (buf[11]<<8) + buf[10]); 
    printf("%s\n", info);
    tp.printf("%s", info);
    
}

int main()
{
    uint8_t buffer[20][16];
    printf("\n*** RCS620S テストプログラム ***\n\n");
    rcs620s.initDevice();
    tp.initialize();
    tp.putLineFeed(2);

    while(1) {
        uint32_t balance;
        uint8_t buf[RCS620S_MAX_CARD_RESPONSE_LEN];
        int isCaptured = 0;
        
        rcs620s.timeout = COMMAND_TIMEOUT;
        
        // サイバネ領域
        if(rcs620s.polling(CYBERNE_SYSTEM_CODE)){
            // Suica PASMO
            if(requestService(PASSNET_SERVICE_CODE)){
                isCaptured = 1;
                for (int i = 0; i < 20; i++) {
                    if(readEncryption(PASSNET_SERVICE_CODE, i, buf) /*&& buf[12] != 0*/){
#if 0
                        // Little Endianで入っているPASSNETの残高を取り出す
                        balance = buf[23];                  // 11 byte目
                        balance = (balance << 8) + buf[22]; // 10 byte目
                        // 残高表示
                        //printBalanceLCD("PASSNET", &balance);
#else
                        memcpy(buffer[i], &buf[12], 16);
                        //parse_history(&buf[12]);
                        //parse_history(&buffer[i][0]);

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
        
        if (isCaptured) {
            for (int i = 0; i < PRINT_ENTRIES; i++) {
                if (buffer[i][0] != 0) {
                    parse_history(&buffer[i][0]);
                }
            }
            tp.putLineFeed(4);
            isCaptured = 0;
        }

        rcs620s.rfOff();
        led = !led;
        thread_sleep_for(POLLING_INTERVAL);
    }
}

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

void printBalanceLCD(const char *card_name, uint32_t *balance)
{
    printf("%s: %ld\n", card_name, *balance);
}
