#ifndef IFACEEXCHANGETYPES_H
#define IFACEEXCHANGETYPES_H

#include <QStringList>
#include <QDateTime>
#include <QElapsedTimer>

///[!] ifaces
#include "src/emb/conf2modem.h"


#include "ifaceexchangetypesdefs.h"



struct PollObjectState
{
    bool block4waitSignal;
    QDateTime block4waitStarted;

    bool dbClientNready;


    bool blockDb;//pollSett.blockDb = false;
    bool embeeReady;//wait4signalready

    QElapsedTimer blockTime;
    QElapsedTimer blockDbTime;

    bool updateAboutModemForced;//оновити на старті про координатор

    QString lastNi;
    QString lastFullTableName;

    PollObjectState() : block4waitSignal(false), dbClientNready(true), blockDb(false), embeeReady(false), updateAboutModemForced(true) {}
} ;

struct TaskState
{
    quint32 taskId;
    bool noTasks;//завершено обробку завдання, будь яка причина
    bool tasksProcessingEnded;//true - інтерфейс закрито бо обмін з лічильником завершено, а нові завдання відсутні. false - є завдання і була спроба відкриття з'єднання
    QString lastDevNI;
    TaskState() : taskId(0), noTasks(true), tasksProcessingEnded(true) {}
} ;


struct IfaceParams
{
    QString lastIfaceParams;
    Conf2modem *confModemHelper;
    quint16 closeConnLater;
//    bool niRecover; moved to connSett.try2recoverNI


    IfaceParams() : closeConnLater(0) {}
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

    bool enableDatabaseLoggin;

    PollSavedSettings() : meterRetryMax(1), meterRetryMaxFA(1), zatrymkaDoZapuskuSek(30), hardAddrsn(false), enableW4E(true), corDTallow(false),
        messCountBeforeReady(1), messCountAfter(100), corDTintrvl(300), useForwardTable(false), autoAdd2forwardTable(true), checkPowerSleepProfile(true),
        go2sleepPowerSleepProfile(30), enableDatabaseLoggin(false)  {}

    PollSavedSettings(const quint16 &meterRetryMax, const quint16 &meterRetryMaxFA, const qint32 &zatrymkaDoZapuskuSek, const bool &hardAddrsn, const bool &enableW4E, const bool &corDTallow,
                      const qint32 &messCountBeforeReady, const qint32 &messCountAfter, const qint32 &corDTintrvl, const bool &useForwardTable, const bool &autoAdd2forwardTable,
                      const bool &checkPowerSleepProfile, const int &go2sleepPowerSleepProfile, const bool &enableDatabaseLoggin)
        : meterRetryMax(meterRetryMax), meterRetryMaxFA(meterRetryMaxFA), zatrymkaDoZapuskuSek(zatrymkaDoZapuskuSek), hardAddrsn(hardAddrsn), enableW4E(enableW4E), corDTallow(corDTallow),
        messCountBeforeReady(messCountBeforeReady), messCountAfter(messCountAfter), corDTintrvl(corDTintrvl), useForwardTable(useForwardTable), autoAdd2forwardTable(autoAdd2forwardTable),
        checkPowerSleepProfile(checkPowerSleepProfile), go2sleepPowerSleepProfile(go2sleepPowerSleepProfile), enableDatabaseLoggin(enableDatabaseLoggin) {}

};


struct ExchangeWithOneModemStat
{
    QDateTime startPollDate;//

    QStringList listWriteReadHistory;
    QElapsedTimer time4taskProcessing;//it counts  processing time for one task from the beggining to the end

    quint32 byte2meter;
    quint32 bytesFromMeter;

    quint32 totalMessageCounter;
    quint32 usflDataLen;

    qint32 badAnswerCounter;//empty or bad data, it resets by a good answer
    qint32 emptySequenceAnswerCounter;//it counts only empty answers that follow each other, it resets by a good answer

    QHash<QByteArray,quint32> hashMessRetryCounter;

    QElapsedTimer timeFromEmptyChanged;//для аналізу порожніх відповідей
    QElapsedTimer time4poll;//час між новими завданнями
    QElapsedTimer time4multicast;//час для паузи в обміні, для системи агрегатування

    ExchangeWithOneModemStat() : byte2meter(0), bytesFromMeter(0), totalMessageCounter(0), usflDataLen(0), badAnswerCounter(0), emptySequenceAnswerCounter(0)  {}
};




#endif // IFACEEXCHANGETYPES_H
