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

    ZbyrConnSett() {}

    ZbyrConnSett( const QString &prdvtrAddr, const qint32 &prdvtrPort, const qint32 &timeOutB, const qint32 &timeOutG) :
        connectionType(1), prdvtrAddr(prdvtrAddr), prdvtrPort(prdvtrPort), timeOutB(timeOutB), timeOutG(timeOutG) {}

    ZbyrConnSett(const quint8 &connectionType, const QString &prdvtrAddr, const qint32 &prdvtrPort, const qint32 &timeOutB, const qint32 &timeOutG, const QStringList &uarts, const QVariantHash &m2mhash) :
        connectionType(connectionType), prdvtrAddr(prdvtrAddr), prdvtrPort(prdvtrPort), timeOutB(timeOutB), timeOutG(timeOutG), uarts(uarts), m2mhash(m2mhash) {}
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








class IfaceExchangeSerializedTypes
{

public:
    static void makeRegistration();

    static ZbyrConnSett getZbyrConnSettFromArgs(const QStringList &list, bool &ok);


};
//must be in a header file, outside the class!!!

QDataStream &operator <<(QDataStream &out, const ZbyrConnSett &m);
QDataStream &operator >>(QDataStream &in, ZbyrConnSett &m);
QDebug operator<<(QDebug d, const ZbyrConnSett &m);






Q_DECLARE_METATYPE(ZbyrConnSett) //must be in header



#endif // IFACEEXCHANGESERIALIZEDTYPES_H
