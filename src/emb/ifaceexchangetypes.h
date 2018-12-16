#ifndef IFACEEXCHANGETYPES_H
#define IFACEEXCHANGETYPES_H

#include <QStringList>
#include <QDateTime>
#include <QTime>
#include "src/emb/conf2modem.h"


#define IFACE_CLOSE_AFTER   180
#define MAX_MESS_COUNT_PER_ONE_TASK     1000
#define MAX_IDENTICAL_MESS_PER_SESION   55//SETT_MAX_IDENTICAL_MESS_PER_SESION
#define MAX_DIFF_MSEC_FOR_TASK_SESION   1500000  //25 MINUTES


struct PollObjectState
{
    bool block4waitSignal;
    QDateTime block4waitStarted;

    bool dbClientNready;


    bool blockDb;//pollSett.blockDb = false;
    bool embeeReady;//wait4signalready

    QTime blockTime;
    QTime blockDbTime;

    bool updateAboutModemForced;//оновити на старті про координатор

    QString lastNi;
    QString lastFullTableName;

    PollObjectState() : block4waitSignal(false), dbClientNready(true), blockDb(false), embeeReady(false), updateAboutModemForced(true) {}
} ;

struct TaskState
{
    quint32 taskId;
    bool noTasks;
    TaskState() : taskId(-1), noTasks(true) {}
} ;


struct IfaceParams
{
    QString lastIfaceParams;
    Conf2modem *confModemHelper;
    quint16 closeConnLater;
    bool niRecover;
    IfaceParams() : closeConnLater(0), niRecover(false) {}
} ;



struct PollSavedSettings
{
    quint16 meterRetryMax;
    quint16 meterRetryMaxFA;
    qint32 zatrymkaDoZapuskuSek;//pollSett.zatrymkaDoZapuskuSek = 30;
    bool hardAddrsn;//pollSett.hardAddrsn = false;

    bool enableW4E;//pollSett.enableW4E = true;
    bool corDTallow;

    //wait4embeeReady
    qint32 messCountBeforeReady;//pollSett.messCountBeforeReady = 1;
    qint32 messCountAfter; //pollSett.messCountAfter = 100;

    qint32 corDTintrvl/*, corDTintrvlMax*/;

    bool useForwardTable;
    bool autoAdd2forwardTable;//pollSett.autoAdd2forwardTable = true;

    bool checkPowerSleepProfile;
    int go2sleepPowerSleepProfile;//seconds

    PollSavedSettings() : meterRetryMax(1), meterRetryMaxFA(1), zatrymkaDoZapuskuSek(30), hardAddrsn(false), enableW4E(true), corDTallow(false),
        messCountBeforeReady(1), messCountAfter(100), corDTintrvl(300), useForwardTable(false), autoAdd2forwardTable(true), checkPowerSleepProfile(true), go2sleepPowerSleepProfile(30)  {}

    PollSavedSettings(const quint16 &meterRetryMax, const quint16 &meterRetryMaxFA, const qint32 &zatrymkaDoZapuskuSek, const bool &hardAddrsn, const bool &enableW4E, const bool &corDTallow,
                      const qint32 &messCountBeforeReady, const qint32 &messCountAfter, const qint32 &corDTintrvl, const bool &useForwardTable, const bool &autoAdd2forwardTable,
                      const bool &checkPowerSleepProfile, const int &go2sleepPowerSleepProfile)
        : meterRetryMax(meterRetryMax), meterRetryMaxFA(meterRetryMaxFA), zatrymkaDoZapuskuSek(zatrymkaDoZapuskuSek), hardAddrsn(hardAddrsn), enableW4E(enableW4E), corDTallow(corDTallow),
        messCountBeforeReady(messCountBeforeReady), messCountAfter(messCountAfter), corDTintrvl(corDTintrvl), useForwardTable(useForwardTable), autoAdd2forwardTable(autoAdd2forwardTable),
        checkPowerSleepProfile(checkPowerSleepProfile), go2sleepPowerSleepProfile(go2sleepPowerSleepProfile) {}

};


struct ExchangeWithOneModemStat
{
    QDateTime startPollDate;//


    quint32 byte2meter;
    quint32 bytesFromMeter;

    quint32 totalMessCounter;
    quint32 usflDataLen;

    qint32 emptyAnswerCounter;

    QHash<QByteArray,quint32> hashMessRetryCounter;

    QTime timeFromEmptyChanged;//для аналізу порожніх відповідей
    QTime time4poll;//час між новими завданнями
    QTime time4multicast;//час для паузи в обміні, для системи агрегатування

    ExchangeWithOneModemStat() : byte2meter(0), bytesFromMeter(0), totalMessCounter(0), usflDataLen(0)  {}
};


#endif // IFACEEXCHANGETYPES_H
