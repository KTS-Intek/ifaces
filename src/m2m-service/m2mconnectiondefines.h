#ifndef M2MCONNECTIONDEFINES_H
#define M2MCONNECTIONDEFINES_H





#define MAX_M2M_ZOMBIE_RETRIES 2
#define MAX_UNCOMPRSS_PACKET_SIZE  500

////matilda protocol low level commands
//#define COMMAND_ZULU                            0
//#define COMMAND_YOUR_ID_AND_MAC                 1

//#define COMMAND_AUTHORIZE                       2

//#define COMMAND_ACCESS_DENIED                   4
//#define COMMAND_CONNECT_2_THIS_SERVICE          5
////#define COMMAND_LAST_OPER_RESULT                5
//#define COMMAND_I_AM_A_ZOMBIE                     6
//#define COMMAND_ERROR_CODE                      7

//#define COMMAND_COMPRESSED_PACKET               8
//#define COMMAND_COMPRESSED_STREAM               9
//#define COMMAND_I_NEED_MORE_TIME                10
//#define COMMAND_CONNECT_ME_2_THIS_ID_OR_MAC     11


//#define COMMAND_ERROR_CODE_EXT                  13

////Backup Service (No authorization)
//#define COMMAND_CHECK_BACKUP_FILE_HASH_SUMM     14
//#define COMMAND_UNKNOWN                         15



#define COMMAND_DA_CLOSE                        0
#define COMMAND_DA_OPEN                         1
#define COMMAND_DA_OPEN_EXT                     2
#define COMMAND_DA_OPEN_TCP_SERVER_MEDIUM_ID_F  1000 //50000 ... X

#define COMMAND_DA_OPEN_CHANNEL_ID              2002 // 2002- CHANNEL 2, 2101 CHANNL  101
#define COMMAND_DA_OPEN_CHANNEL_ID_LAST         2101 // 2002- CHANNEL 2, 2101 CHANNL  101



#define M2M_CONN_STATE_DISCONNECTED     0
#define M2M_CONN_STATE_CONNECTING       1
#define M2M_CONN_STATE_CONNECTED        2
#define M2M_CONN_STATE_LOGGINING        3
#define M2M_CONN_STATE_LOGINED          4
#define M2M_CONN_STATE_DISCONNECTING    5
#define M2M_CONN_STATE_SEARCHINGM2M     6


#define M2M_ERR_NO_ERR              0 //it will be seldom used
#define M2M_ERR_OLDPROTOCOLVERSION  1 //<first>\n<second>  <first> - device version, <second> - library version
#define M2M_ERR_UNKNOWN_DEVICE      2 //comment is empty
#define M2M_ERR_ACCESS_DENIED       3 //
#define M2M_ERR_WRITE_ERROR         4 //

#define M2M_ERR_IP_IN_BLOCKLIST     13



#endif // M2MCONNECTIONDEFINES_H
