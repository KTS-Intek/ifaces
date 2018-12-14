#ifndef IFACEEXCHANGESERIALIZEDTYPES_H
#define IFACEEXCHANGESERIALIZEDTYPES_H

#include "ifaceexchangetypes.h"

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


struct UniverslaMeterOnlyCache
{
//    QString enrg4vrsn;//coma separeted list, cached info about energy that meter support
    QMap<quint8,QString> enrg4vrsn;
    UniverslaMeterOnlyCache() {}
};


struct UniversalMeterSett
{

    quint8 meterType;//readOnly
    QString model;//write if Auto
    QString sn;//write

    QString ni;//readOnly
    QString memo;//readOnly
    QString passwd;//readOnly
    bool pollEnbl;//readOnly

    QString enrg; //readOnly coma separated line, this is a sleep profile to water meters
    quint8 tariff;//readOnly

    QString coordinate;//readOnly
    QString version;//write

    QString powerin;//readOnly
    MeterTransformer transformer;//readOnly
    bool disableTimeCorrection;//readOnly

    UniverslaMeterOnlyCache cache;

    UniversalMeterSett() {}

    UniversalMeterSett(const quint8 &meterType, const QString &model, const QString &sn, const QString &ni, const QString &memo, const QString &passwd, const bool &pollEnbl, const QString enrg,
                       const quint8 tariff, const QString coordinate, const QString &version, const QString &powerin, const MeterTransformer &transformer, const bool &disableTimeCorrection) :
        meterType(meterType), model(model), sn(sn), ni(ni), memo(memo), passwd(passwd), pollEnbl(pollEnbl), enrg(enrg),
        tariff(tariff), coordinate(coordinate), version(version), powerin(powerin), transformer(transformer), disableTimeCorrection(disableTimeCorrection) {}


    UniversalMeterSett(const QString &model, const QString &sn, const QString &ni, const QString &memo, const QString &passwd, const bool &pollEnbl, const QString enrg,
                       const quint8 tariff, const QString coordinate, const QString &version, const QString &powerin, const MeterTransformer &transformer, const bool &disableTimeCorrection) :
        meterType(0xFF), model(model), sn(sn), ni(ni), memo(memo), passwd(passwd), pollEnbl(pollEnbl), enrg(enrg),
        tariff(tariff), coordinate(coordinate), version(version), powerin(powerin), transformer(transformer), disableTimeCorrection(disableTimeCorrection) {}

};


typedef QList<UniversalMeterSett> UniversalMeterSettList;


struct PollDateMemoExt
{
    qint32 depth;
    quint8 pollCode;
    QString tableName;
    QString fullTableName;
    bool ignorePrevData;

    QDateTime pollDateTime;
    QDateTime pollDateTimeFirstIntrvl;
    QDateTime pollDateTimeNextIntrvl;
    QDateTime dtLastStartPoll;

    QString operationArgument;

    PollDateMemoExt() : depth(1), pollCode(0), ignorePrevData(false) {}
};



class IfaceExchangeSerializedTypes
{

public:
    static void makeRegistration();

};
//must be in a header file, outside the class!!!

QDataStream &operator <<(QDataStream &out, const ZbyrConnSett &m);
QDataStream &operator >>(QDataStream &in, ZbyrConnSett &m);
QDebug operator<<(QDebug d, const ZbyrConnSett &m);

QDataStream &operator <<(QDataStream &out, const UniversalMeterSett &m);
QDataStream &operator >>(QDataStream &in, UniversalMeterSett &m);
QDebug operator<<(QDebug d, const UniversalMeterSett &m);

QDataStream &operator <<(QDataStream &out, const MeterTransformer &m);
QDataStream &operator >>(QDataStream &in, MeterTransformer &m);
QDebug operator<<(QDebug d, const MeterTransformer &m);

QDataStream &operator <<(QDataStream &out, const UniverslaMeterOnlyCache &m);
QDataStream &operator >>(QDataStream &in, UniverslaMeterOnlyCache &m);
QDebug operator<<(QDebug d, const UniverslaMeterOnlyCache &m);

QDataStream &operator <<(QDataStream &out, const PollDateMemoExt &m);
QDataStream &operator >>(QDataStream &in, PollDateMemoExt &m);
QDebug operator<<(QDebug d, const PollDateMemoExt &m);


bool operator ==(const MeterTransformer &t0, const MeterTransformer &t1);
bool operator ==(const UniversalMeterSett &s, const UniversalMeterSett &s1);

Q_DECLARE_METATYPE(ZbyrConnSett) //must be in header
Q_DECLARE_METATYPE(UniversalMeterSett)
Q_DECLARE_METATYPE(MeterTransformer)
Q_DECLARE_METATYPE(UniverslaMeterOnlyCache)
Q_DECLARE_METATYPE(PollDateMemoExt)


#endif // IFACEEXCHANGESERIALIZEDTYPES_H
