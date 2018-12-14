#ifndef EMBEELIMITS_H
#define EMBEELIMITS_H

#define MAX_MSEC_4_EMB_READY            180000
#define MAX_READ_FROM_UART              260
#define MAX_READ_TRYES_FROM_UART        20000
#define MAX_TIME_FROM_EMPTY_MSEC        120000 //3 hvylyn for multicast
#define MAX_TIME_FOR_MULTICAST_MSEC     300000 //5 hvylyn for multicast
#define MAX_TRIES_FOR_CONFIG            50
#define MAX_TRIES_FOR_HARD_RESET        MAX_TRIES_FOR_CONFIG + 10//має бути завжди більшим за MAX_TRIES_FOR_CONFIG
#define MAX_MSEC_FOR_COORDINATOR_READY  1800000 //30 hv

#define MAX_MSEC_BETWEEN_UPDATE_ABOUTM  10800000//3*60*60*1000


#endif // EMBEELIMITS_H
