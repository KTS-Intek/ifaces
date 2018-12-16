#ifndef PEREDAVATORPRIORITY_H
#define PEREDAVATORPRIORITY_H

#define PEREDAVATOR_PRTT_4_OTHERS           -1
#define PEREDAVATOR_PRTT_CRITICAL           0
#define PEREDAVATOR_PRTT_VERY_HIGH          1 //читання показників лічильників, (по запиту)
#define PEREDAVATOR_PRTT_HIGH               2 //роздача потужності по групам, включення реле
#define PEREDAVATOR_PRTT_NORMAL             3 //читання показників лічильників, (лише збір)
#define PEREDAVATOR_PRTT_LOW                4 //читання стану ламп,
#define PEREDAVATOR_PRTT_VERY_LOW           5 //читання показників лічильників, (лише дозбір)
#define PEREDAVATOR_PRTT_VERY_VLOW          6
#define PEREDAVATOR_PRTT_VERY_VVLOW         7
#define PEREDAVATOR_PRTT_VERY_VVVLOW        8
#define PEREDAVATOR_PRTT_VERY_VVVVLOW       9
#define PEREDAVATOR_PRTT_VERY_VVVVVLOW      0xA
#define PEREDAVATOR_PRTT_VERY_VVVVVVLOW     0xB
#define PEREDAVATOR_PRTT_VERY_VVVVVVVLOW    0xC
#define PEREDAVATOR_PRTT_VERY_VVVVVVVVLOW   0xD
#define PEREDAVATOR_PRTT_VERY_VVVVVVVVVLOW  0xE
#define PEREDAVATOR_PRTT_VERY_VVVVVVVVVVLOW 0xF//reset an access level to UART

#endif // PEREDAVATORPRIORITY_H
