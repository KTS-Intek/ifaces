#ifndef EMBNODEDISCOVERYTYPES_H
#define EMBNODEDISCOVERYTYPES_H


#include <QStringList>
#include <QVariantMap>

struct OneEmbeeModemSmpl
{
    QString typestr;
    QString eui64;
    QString ni;
    bool wasRestored;
    qint64 lastmsec;
    OneEmbeeModemSmpl() {}
};

struct NodeDiscoveryLimits
{
    quint16 limt; //total devices
    quint16 limn; //new devices

    qint32 minutesDuration;

    NodeDiscoveryLimits() : limt(0), limn(0), minutesDuration(60) {}

    NodeDiscoveryLimits(const quint16 &limt, const quint16 &limn, const qint32 &minutesDuration) : limt(limt), limn(limn), minutesDuration(minutesDuration) {}
};




struct AtndOneModeSettStruct
{
    int minnilen;
    int maxnilen;
    bool acceptlowercase;
    bool acceptuppercase;
    QString pattern;
    QString lasterror;
    AtndOneModeSettStruct() : minnilen(1), maxnilen(16), acceptlowercase(true), acceptuppercase(true) {}
};

struct NodeDiscoveryFindingState
{
    quint16 foundedNewNisCount;//only new NIs and accepted by the rule
    quint16 foundedEuisCount;//all uniq founded eui64
    quint16 totalDevCountAtTheBegining;//

    QStringList olddevnis;
    QStringList newdevnis;
    QMap<QString,QString> eui64toNI;
    QMap<QString,QString> ni2eui64;

    qint64 scanStarted;

    NodeDiscoveryFindingState() : foundedNewNisCount(0), foundedEuisCount(0), totalDevCountAtTheBegining(0), scanStarted(0) {}
};

struct NodeDiscoveryStruct
{
    bool enAutoExport;//only for GUI apps

    NodeDiscoveryLimits limits;

    AtndOneModeSettStruct sett;

    NodeDiscoveryFindingState state;
    QStringList mdls;//from space separated line
    QVariantMap defs;
    NodeDiscoveryStruct() : enAutoExport(true) {}
};

#endif // EMBNODEDISCOVERYTYPES_H
