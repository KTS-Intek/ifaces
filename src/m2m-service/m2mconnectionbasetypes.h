#ifndef M2MCONNECTIONBASETYPES_H
#define M2MCONNECTIONBASETYPES_H

#include <QString>
#include <QPointF>
#include <QVariantHash>

struct DEVICE_POSITION_SN
{
    QPointF point;
    bool isDefault;//true - it is calculated from timezone, false - it is a written value

    QString devicesn;
    bool hasSn;

    DEVICE_POSITION_SN() : point(0.0, 0.0), isDefault(true), hasSn(false) {}

    DEVICE_POSITION_SN(const QPointF &point, const bool &isDefault) : point(point), isDefault(isDefault), hasSn(false) {}

    DEVICE_POSITION_SN(const QPointF &point, const bool &isDefault, const QString &devicesn, const bool &hasSn)
        : point(point), isDefault(isDefault), devicesn(devicesn), hasSn(hasSn) {}

};

struct COMMAND_ERR_CODE_STRUCT
{
    quint16 lcmd;
    int errcode;//e
    QString message; //em
    COMMAND_ERR_CODE_STRUCT() : lcmd(0), errcode(0) {}
};


struct COMMAND_ZULU_STRUCT
{
    //these are mandatory
    QString name;//QString("Matilda")
    int version;//MATILDA_PROTOCOL_VERSION

    QString UTC;//yyyy-MM-dd hh:mm:ss dt of the message
    int UOFT;//secs from UTC

    //case 1 - everything is fine

    QString memo;//SETT_ABOUT_MEMOLINE, up to 1000 bytes
    int QDS;//QDataStream::Qt_5_6

    int BLC;//blocked ip list size
    int CTCT;//connection counter
    int CNTR;//optional, counts failes, for direct connection only

    QString cmprssn;//zlib, I don't think that it will be ever empty
    quint32 RND;//some random value, from 1 to 0xFFFFFFF


    //caes 2 - error happend
    QString message;
    int err;//ERR_IP_BLOCKED , if 0 ignore, ERR_NO_ERROR  99

    COMMAND_ZULU_STRUCT() : version(0), UOFT(0), QDS(0), BLC(0), CTCT(0), CNTR(0), RND(0) {}
};


struct COMMAND_AUTHORIZE_STRUCT
{
    QString buildversion;//v
    int devtype;//d
    DEVICE_POSITION_SN positionsn;//sn pos use positionsn
    quint8 accessLevel;//a

    QVariantHash hashMeterModelRules;//it contains b, bw, bext, bwext


    COMMAND_AUTHORIZE_STRUCT() : devtype(0), accessLevel(0) {}
};



struct BASE_CONNECTION_INFO
{
//    DEVICE_POSITION_SN positionsn; it must be in COMMAND_AUTHORIZE_STRUCT


    COMMAND_ZULU_STRUCT zulu;
    COMMAND_AUTHORIZE_STRUCT authorize;

    BASE_CONNECTION_INFO() {}
};

#endif // M2MCONNECTIONBASETYPES_H
