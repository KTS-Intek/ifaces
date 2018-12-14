#include "matildamessages.h"
#include "showmesshelper4wdgtdef.h"
#include "moji_defy.h"

MatildaMessages::MatildaMessages(QObject *parent) : QObject(parent)
{

}

//-------------------------------------------------------------------------------------------------------------------------------------
QString MatildaMessages::messFromCode(const int &messCode)
{
    QString mess;
    switch(messCode){
    case MESS_NO_TABLE: mess = tr("No table for this period("); break;
    case MESS_CORRUPTED_DATA: mess = tr("Corrupted data("); break;
    case MESS_CORRUPTED_PACKET: mess = tr("Corrupted packet("); break;
    case MESS_NO_ANSWER_FROM_DEVICE: mess = tr("No answer from the device("); break;
    case MESS_NO_DATA: mess = tr("Data is not found("); break;
    case MESS_OPERATION_ABORTED: mess = tr("Operation aborted"); break;
    case MESS_CANT_CONNECT2DEV: mess = tr("Can't connect to the device("); break;


    }
    return mess;
}

//-------------------------------------------------------------------------------------------------------------------------------------
QString MatildaMessages::addRez2endOfLine(const QString m, const bool &rezGood)
{
    return tr("%1. Operation: %2").arg(m).arg( rezGood ? tr("Done)") : tr("Failed(")) ;
}

//-------------------------------------------------------------------------------------------------------------------------------------
QString MatildaMessages::name4command(const quint16 &command)
{
    QString s;
    switch(command){
    case COMMAND_ZULU:                          break;
    case COMMAND_YOUR_ID_AND_MAC:               break;

    case COMMAND_AUTHORIZE                      : s = tr("Authorization"); break;

    case COMMAND_ACCESS_DENIED:                 break;
    case COMMAND_CONNECT_2_THIS_SERVICE:        break;
        //case COMMAND_LAST_OPER_RESULT                5
    case COMMAND_I_AM_A_ZOMBIE:                 break;
    case COMMAND_ERROR_CODE:                    break;

    case COMMAND_COMPRESSED_PACKET:             break;
    case COMMAND_COMPRESSED_STREAM:             break;
    case COMMAND_I_NEED_MORE_TIME:              break;
    case COMMAND_CONNECT_ME_2_THIS_ID_OR_MAC:   break;
    case COMMAND_ERROR_CODE_EXT:                break;



        //READ ROOT AND GUEST
    case COMMAND_READ_SYSTEM_SETTINGS           : s = tr("system info")         ; break;
    case COMMAND_READ_TASK_INFO                 : s = tr("running process")     ; break;
    case COMMAND_READ_ABOUT_PLG                 : s = tr("plugins for meters")  ; break;
    case COMMAND_READ_DATE_SETT                 : s = tr("date and time")       ; break;
    case COMMAND_READ_GPRS_SETT                 : s = tr("GSM settings")        ; break;
    case COMMAND_READ_STATE                     : s = tr("state")               ; break;
    case COMMAND_READ_IFCONFIG                  : s = tr("network interfaces")  ; break;
    case COMMAND_READ_APP_LOG                   : s = tr("application events")  ; break;
    case COMMAND_READ_POLL_SCHEDULE             : s = tr("poll schedule")       ; break;
    case COMMAND_READ_METER_LIST                : s = tr("meter list")          ; break;

    case COMMAND_READ_DATABASE                  : s = tr("database")            ; break;

    case COMMAND_READ_DATABASE_GET_TABLES       : s = tr("database")            ; break;
    case COMMAND_READ_DATABASE_GET_VAL          : s = tr("database")            ; break;
    case COMMAND_READ_METER_LOGS                : s = tr("logs of meters")      ; break;
    case COMMAND_READ_METER_LOGS_GET_TABLES     : s = tr("logs of meters")      ; break;
    case COMMAND_READ_METER_LOGS_GET_VAL        : s = tr("logs of meters")      ; break;

    case COMMAND_READ_METER_LIST_FRAMED         : s = tr("meter list")          ; break;

    case COMMAND_READ_DATABASE_TABLES_PARSING   : s = tr("database")            ; break;
    case COMMAND_READ_ZBR_LOG                   : s = tr("zbyrator events")     ; break;
    case COMMAND_READ_ABOUT_OBJECT              : s = tr("about object")        ; break;
    case COMMAND_READ_POLL_SETT                 : s = tr("poll settings")       ; break;
    case COMMAND_READ_POLL_STATISTIC            : s = tr("statistic of exchange"); break;

    case COMMAND_READ_TABLE_HASH_SUMM           : s = tr("hash summ")           ; break;


    case COMMAND_READ_IP_FILTER_SETT            : s = tr("IP filter")           ; break;

    case COMMAND_READ_METER_LIST_HASH_SUMM      : s = tr("meter list hash summ"); break;
    case COMMAND_READ_SERIAL_LOG                : s = tr("serial port log")     ; break;
    case COMMAND_READ_COMMANDS                  : s = tr("device commands")     ; break;


    case COMMAND_READ_DA_DATA_FROM_COORDINATOR  :                                 break;
    case COMMAND_READ_DA_SERVICE_SETT           : s = tr("direct access")       ; break;
    case COMMAND_READ_PLUGIN_LOG_WARN           : s = tr("warning events")      ; break;
    case COMMAND_READ_PLUGIN_LOG_ERROR          : s = tr("error events")        ; break;

    case COMMAND_READ_PEREDAVATOR_AC_SETT       : s = tr("direct access client"); break;
    case COMMAND_READ_MATILDA_AC_SETT           : s = tr("M2M client"); break;
    case COMMAND_READ_BACKUP_LIST               : s = tr("backup")              ; break;
    case COMMAND_READ_UDP_BEACON                : s = tr("UDP beacon")          ; break;

    case COMMAND_READ_METER_LIST_HASH_SUMM_EXT  : s = tr("meter list hash summ"); break;
    case COMMAND_READ_METER_LIST_FRAMED_EXT     : s = tr("caching info about meters..."); break;

    case COMMAND_READ_ZIGBEE_SETT               : s = tr("ZigBee settings")     ; break;
    case COMMAND_READ_TCP_SETT                  : s = tr("TCP settings")        ; break;
    case COMMAND_READ_FRWRD_SETT                : s = tr("forward settings")    ; break;

   //protocol v2
    case COMMAND_READ_IPTABLE                   : s = tr("IP Table")            ; break;
    case COMMAND_READ_PPP_SUPERVISOR            : s = tr("PPP Supervisor")      ; break;

    case COMMAND_READ_DEVICE_SERIAL_NUMBER      : s = tr("Serial Number")       ; break;

        ///LED LAMP
    case COMMAND_READ_LEDLAMPLIST_FRAMED        : s = tr("Lights")       ; break;
    case COMMAND_READ_GROUP_SCHEDULE            : s = tr("Schedule")      ; break;
    case COMMAND_READ_FF_TASK_TABLE_FRAMED             : s = tr("Tasks")      ; break;
    case COMMAND_READ_POWER_RELAY_SETT          : s = tr("Relay")      ; break;
    case COMMAND_READ_FIREFLY_MODE_SETT         : s = tr("Modes")      ; break;
        //case COMMAND_READ_MEMO_SN                36


        //ROOT || OPERATOR WRITE
    case COMMAND_WRITE_FIRST_4_OPERATOR         : break;
    case COMMAND_WRITE_POLL_SCHEDULE            : s = tr("poll schedule")       ; break;
    case COMMAND_WRITE_METER_LIST               : s = tr("meter list")          ; break;
    case COMMAND_WRITE_METER_LIST_FRAMED        : s = tr("meter list")          ; break;
    case COMMAND_WRITE_DATE_SETT                : s = tr("date and time")       ; break;
    case COMMAND_WRITE_RESET_MODEM              : s = tr("reset modem")         ; break;
    case COMMAND_WRITE_POLL_SETT                : s = tr("poll settings")       ; break;
    case COMMAND_WRITE_METER_LIST_ONE_PART      : s = tr("meter list")          ; break;
    case COMMAND_WRITE_METER_LIST_POLL_ON       : s = tr("poll on")             ; break;
    case COMMAND_WRITE_METER_LIST_POLL_OFF      : s = tr("poll off")            ; break;
    case COMMAND_WRITE_METER_LIST_DEL_NI        : s = tr("delete meters")       ; break;
    case COMMAND_WRITE_COMMANDS                 : s = tr("command")             ; break;
    case COMMAND_WRITE_DA_SERVICE_SETT          : s = tr("direct access")       ; break;
    case COMMAND_WRITE_PEREDAVATOR_AC_SETT      : s = tr("direct access active client") ; break;
    case COMMAND_WRITE_DA_OPEN_CLOSE            : s = tr("open/close direct access over matilda protocol"); break;
    case COMMAND_WRITE_DA_DATA_2_COORDINATOR    : break;
    case COMMAND_WRITE_FRWRD_SETT               : s = tr("forward settings")    ; break;

        //protocol v2
        ///LED LAMP
    case COMMAND_WRITE_LEDLAMPLIST_FRAMED       : s = tr("Lights")               ; break;
    case COMMAND_WRITE_GROUP_SCHEDULE           : s = tr("Schedule")            ; break;
    case COMMAND_WRITE_POWER_RELAY_SETT         : s = tr("Relay")               ; break;
    case COMMAND_WRITE_FIREFLY_MODE_SETT        : s = tr("Modes")               ; break;


        //ROOT: ONLY WRITE
    case COMMAND_WRITE_FIRST                    : break;
    case COMMAND_WRITE_UPGRADE                  : s = tr("upgrade")             ; break;

    case COMMAND_WRITE_GPRS_SETT                : s = tr("GSM settings")        ; break;
    case COMMAND_WRITE_REBOOT                   : s = tr("reboot command")      ; break;
    case COMMAND_WRITE_DAEMON_RESTART           : s = tr("daemon restart command"); break;
    case COMMAND_WRITE_FULL_UPGRADE             : s = tr("full upgrade")        ; break;
    case COMMAND_WRITE_ROOT_LOGIN_PSWD          : s = tr("administrator's login and password"); break;
    case COMMAND_WRITE_GUEST_LOGIN_PSWD         : s = tr("guest's login and password"); break;
    case COMMAND_WRITE_OPEARTOR_LOGIN_PSWD      : s = tr("operator's login and password"); break;
    case COMMAND_WRITE_DROP_TABLE_GET_COUNT     : s = tr("drop table, get count"); break;
    case COMMAND_WRITE_DROP_TABLE               : s = tr("drop table")          ; break;
    case COMMAND_WRITE_DROP_TABLE_ALL           : s = tr("drop table all")      ; break;
    case COMMAND_WRITE_TIMEZONE                 : s = tr("timezone")            ; break;
    case COMMAND_WRITE_NTP_SETTINGS             : s = tr("NTP settings")        ; break;
    case COMMAND_WRITE_ABOUT_OBJECT             : s = tr("about object")        ; break;
    case COMMAND_WRITE_IP_FILTER_SETT           : s = tr("IP filter")           ; break;
    case COMMAND_WRITE_ERASE_ALL_DATA           : s = tr("erase all data")      ; break;
    case COMMAND_WRITE_MATILDA_AC_SETT          : s = tr("M2M client"); break;
    case COMMAND_WRITE_DELETE_BACKUP_FILE       : s = tr("delete a backup file"); break;
    case COMMAND_WRITE_BACK_IN_TIME             : s = tr("back in time")        ; break;
    case COMMAND_WRITE_CREATE_BACK_IN_TIME      : s = tr("create a backup file"); break;
    case COMMAND_WRITE_UDP_BEACON               : s = tr("UDP beacon")          ; break;

    case COMMAND_WRITE_ZIGBEE_SETT              : s = tr("ZigBee settings")     ; break;
    case COMMAND_WRITE_TCP_SETT                 : s = tr("TCP settings")        ; break;
    case COMMAND_WRITE_COMMAND2BASH             : s = tr("Simple BASH")         ; break;


    case COMMAND_GET_BACKUP_FILE                : s = tr("get backup file")     ; break;
    case COMMAND_PUSH_BACKUP_FILE_AND_APPLY     : s = tr("send backup file")    ; break;
    case COMMAND_WRITE_SVAHA_KILL_CLIENT        : s = tr("diconnecting the client")    ; break;
    default: s = QString("MTDCommand: %1").arg(command); break;
    }

    if(command >= COMMAND_READ_SYSTEM_SETTINGS)
        return (command > COMMAND_WRITE_FIRST_4_OPERATOR) ? tr("Write: %1").arg(s) : tr("Read: %1").arg(s);

    return s;
}

