#include "mbed.h"
#include "RCS620S.h"

// RCS620S
#define COMMAND_TIMEOUT               400
#define POLLING_INTERVAL              500

// FeliCa Service/System Code
#define CYBERNE_SYSTEM_CODE           0x0003

//DigitalOut led(LED1);
RCS620S rcs620s(RCS620S_TX, RCS620S_RX);

int main() {
    AnalogIn ain(dp2);

    //led = 1;
    rcs620s.initDevice();
    rcs620s.timeout = COMMAND_TIMEOUT;

    int pool = 1;
    while(1) {
        float f = ain.read();

        rcs620s.timeout = COMMAND_TIMEOUT;
        
        // サイバネ領域
        if (rcs620s.polling(CYBERNE_SYSTEM_CODE)) {
        }

        pool = (int)(f * 100 + 1);
        thread_sleep_for(pool);

        rcs620s.rfOff();
        thread_sleep_for(pool);
    }
}
