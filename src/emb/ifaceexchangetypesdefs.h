#ifndef IFACEEXCHANGETYPESDEFS_H
#define IFACEEXCHANGETYPESDEFS_H


#define IFACE_CLOSE_AFTER   180
#define MAX_MESS_COUNT_PER_ONE_TASK     1000
#define MAX_IDENTICAL_MESS_PER_SESION   55//SETT_MAX_IDENTICAL_MESS_PER_SESION
#define MAX_DIFF_MSEC_FOR_TASK_SESION   1500000  //25 MINUTES

#define ATND_STATUS_OK      1
#define ATND_STATUS_OLD     2
#define ATND_STATUS_ERR     3
#define ATND_STATUS_IGNORE  4
#define ATND_STATUS_NMODEL  5



#define DA_MODE_ON              1 //always on
#define DA_MODE_MGC             2 //always on + magic sequence
#define DA_MODE_OFF             3 //always off (only local IPv4)
#define DA_MODE_TCP_SERVICE_OFF 4 //always all tcp services off (only localserver




#define MIN_READ_TO 1
#define MAX_READ_TO 120000



#define IFACE_CHANNEL_MAIN  0
#define IFACE_CHANNEL_GROUP 1
#define IFACE_CHANNEL_FIRST 2
#define IFACE_CHANNEL_LAST  101
#define IFACE_CHANNEL_DEF   IFACE_CHANNEL_GROUP //it is for compatibility with quick-collect input meter to iface settings

#define DEF_SETT_TCP_MEDIUM_SERVER_PENDING_CONN 10
#define DEF_SETT_TCP_MEDIUM_SERVER_ZOMBIE_SEC   60


#endif // IFACEEXCHANGETYPESDEFS_H
