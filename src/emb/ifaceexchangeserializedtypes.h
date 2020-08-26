#ifndef IFACEEXCHANGESERIALIZEDTYPES_H
#define IFACEEXCHANGESERIALIZEDTYPES_H

//#include "ifaceexchangetypes.h"

#include <QList>

#include <QString>
#include <QByteArray>
#include <QDateTime>
#include <QDataStream>
#include <QDebug>




struct ZbyrConnSett
{
    quint8 connectionType;//1 -tcp client, 2 - m2m client, other - serial port

    QString prdvtrAddr;//default uart or host name
    qint32 prdvtrPort;//baudRate, tcp port

    qint32 timeOutB;
    qint32 timeOutG;

    QStringList uarts;//uarts to detect modem,

    QVariantHash m2mhash;

//one main and a set of additionals connectins
    bool try2recoverNI;
    bool unknownProtocolAsData;
    bool disableAPImode;
    bool forceHrdAddrsn;


    qint8 databits;
    qint8 stopbits;
    qint8 parity;
    qint8 flowcontrol;


    ZbyrConnSett() : connectionType(1), prdvtrPort(8989), timeOutB(100), timeOutG(1000),
        try2recoverNI(true), unknownProtocolAsData(true), disableAPImode(false), forceHrdAddrsn(false) {}

    ZbyrConnSett( const QString &prdvtrAddr, const qint32 &prdvtrPort, const qint32 &timeOutB, const qint32 &timeOutG) :
        connectionType(1), prdvtrAddr(prdvtrAddr), prdvtrPort(prdvtrPort), timeOutB(timeOutB), timeOutG(timeOutG),
        try2recoverNI(true), unknownProtocolAsData(true), disableAPImode(false), forceHrdAddrsn(false) {}

    ZbyrConnSett(const quint8 &connectionType, const QString &prdvtrAddr, const qint32 &prdvtrPort, const qint32 &timeOutB, const qint32 &timeOutG, const QStringList &uarts,
                 const QVariantHash &m2mhash, const bool &try2recoverNI, const bool &unknownProtocolAsData, const bool &disableAPImode, const bool &forceHrdAddrsn) :
        connectionType(connectionType), prdvtrAddr(prdvtrAddr), prdvtrPort(prdvtrPort), timeOutB(timeOutB), timeOutG(timeOutG), uarts(uarts), m2mhash(m2mhash),
        try2recoverNI(try2recoverNI), unknownProtocolAsData(unknownProtocolAsData), disableAPImode(disableAPImode), forceHrdAddrsn(forceHrdAddrsn) {}
};

//лише те що стосується опитування поточного лічильника
struct CurrentMeterPoll
{
    QVariantHash hashConstData;
    QVariantHash hashTmpData;
    quint16 meterRetry;
    bool wasOk;
    bool updateMeterSett;
    qint64 timeElapsed4multicast;//щоб потім відняти при обчисленні статистики
    bool hasSettings;
    int taskCounter;
    int taskSrc;
    CurrentMeterPoll() : meterRetry(0), wasOk(false), updateMeterSett(false) , timeElapsed4multicast(0), hasSettings(false), taskCounter(-1), taskSrc(-1) {}
};

struct DeviceTimeouts
{
    int block;
    int global;
    DeviceTimeouts() : block(77), global(7777) {}
};




struct EMBZombieExchangeTypes
{
    DeviceTimeouts timeouts;

    QString endseq;
    int readlen ;
    QString answr ;
    int convert8to7;

    EMBZombieExchangeTypes() : endseq("\r\n"), readlen(0), convert8to7(0) {}
};




//додана можливість опитувати пристрої більше ніж по одному каналу, прив'язка виконується по NI
typedef QMap<QString, quint16> NI2IfaceChannel;
typedef QMap<quint16, QVariantMap> IfaceChannel2sett;


class IfaceExchangeSerializedTypes
{

public:
    static void makeRegistration();

    static ZbyrConnSett getZbyrConnSettFromArgs(const QStringList &list, bool &ok);

    static EMBZombieExchangeTypes getDefaultValues4zombie(const DeviceTimeouts &timeouts);

};
//must be in a header file, outside the class!!!

QDataStream &operator <<(QDataStream &out, const ZbyrConnSett &m);
QDataStream &operator >>(QDataStream &in, ZbyrConnSett &m);
QDebug operator<<(QDebug d, const ZbyrConnSett &m);






Q_DECLARE_METATYPE(ZbyrConnSett) //must be in header



#endif // IFACEEXCHANGESERIALIZEDTYPES_H
