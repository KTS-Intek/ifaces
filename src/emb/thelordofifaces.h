#ifndef THELORDOFIFACES_H
#define THELORDOFIFACES_H

#include <QObject>
//#include "src/meter/zbyratortypes4poll.h"
//#include "src/zbyrator-v2/zbyratordatatypehelper.h"
//#include "src/zbyrator-v2/zbyratorsharedtypes.h"
#include "ifaceexchangetypes.h"
#include "ifaceexchangeserializedtypes.h"

#define METER_DO_NOT_SLEEP_MINUTES      16
#define METER_DO_NOT_SLEEP_MINUTES_CHEK 5
#define METER_DO_NOT_SLEEP_MINUTES_CHEKMIN 2

#define METER_GO_TO_SLEEP_SECONDS_FL    30
#define METER_GO_TO_SLEEP_SECONDS       6


class TheLordOfIfaces : public QObject
{
    Q_OBJECT
public:
    explicit TheLordOfIfaces(const quint32 &dbgSrcType, const bool &verboseMode, const ZbyrConnSett &connSett, QObject *parent = nullptr);
    bool activeDbgMessages;
    quint32 dbgSrcType;


    bool dtNowValid;
    QDateTime dtRelease;
    bool verboseMode;
    ZbyrConnSett connSett;//host port timeouts
    bool stopAll;
    bool closeDataBase;
    ExchangeWithOneModemStat zbyrExchngOneMeterStat;
    quint8 retry2peredavator;
    TaskState lastTaskState;

    PollSavedSettings pollConstSett;
    IfaceParams lastIface;
    PollObjectState lastPollObjState;

    struct MetersPowerManagement
    {
        qint64 msecLastSendMess;
        QVariantHash doNotSleepMessage;
        QVariantHash goToSleepMessage;
        QVariantHash goToSleepMessageLast;
        quint8 meterType;
        MetersPowerManagement() : msecLastSendMess(0) {}
        MetersPowerManagement(const qint64 &msecLastSendMess, const QVariantHash &doNotSleepMessage, const QVariantHash &goToSleepMessage, const QVariantHash &goToSleepMessageLast, const quint8 &meterType) :
            msecLastSendMess(msecLastSendMess), doNotSleepMessage(doNotSleepMessage), goToSleepMessage(goToSleepMessage), goToSleepMessageLast(goToSleepMessageLast), meterType(meterType) {}
    };
    QMap<QString,MetersPowerManagement> meterModel2powerManagement;

    struct PowerManagementState
    {
        bool powerManagementSettChecked;
        bool enablePowerManagement;
        bool sendSleepCommandForced;
        bool wasOkFromTheModem;
        bool lockAllSleepMessages;
        quint8 lastMeterType;
        QDateTime lastSended;
        quint8 sendCounter;
        QMap<QString,bool> model2hasPmMessage;
        PowerManagementState() : wasOkFromTheModem(false), sendCounter(0) {}

    } pmState;

    struct SendGo2sleepParams
    {
        qint64 msecDt ;
        int sendNotSleepCount;
        QStringList listDoNotSleep;
        QList<QString> lk;
        SendGo2sleepParams() {}
    };

    bool isTime4updateAboutModem(const QVariantHash &aboutModem);

    qint64 checkTime4multicast();

    qint64 suspendPoll(const int &msec);

    bool incrementTotalMessCounter();

    void wait4signalReady();

    bool checkConnection2coordinator(const bool &simpleConnection);

    bool connect2peredavator(const bool &simpleConnection);

    bool connect2peredavator(const bool &simpleConnection, const QVariantHash &lastinfoaboutcoordinator);

    quint8 readDecodeEmbeeData(const UniversalMeterSett &oneMeter, const PollDateMemoExt &pollDt, CurrentMeterPoll &meterInfo, const int &daMsec);

    quint8 wait4embeeReady();

    bool isTime2breakRead(const QByteArray &readArr, const qint32 &readLen, const qint32 &lastLen, const qint32 &apiMaskLen, const QByteArray &answr, const QByteArray &endSymb, const QByteArray &endSymb2);

    bool isCommandOk4ok(const QByteArray &readArr, const bool &multicastOrOK);

    SendGo2sleepParams getNextDoNotSleepCount(const quint8 &meterType);

    void resetPmStateByModel(const bool &modelDone, const QString &model, const qint64 msecDt, const int &doNotSleepDone, const quint8 &meterType);

    void incrementSleepCounter(const int &doNotSleepDone, const quint8 &meterType);

    bool canIsendGo2sleep4all(bool &rez);

    bool isReleaseDateValid(const QDateTime &currDtUtc);

    void checkEnablePowerManagement4dev(const bool &enablePowerManagement, const quint8 &meterType, const QString &model);

signals:
    void appendDbgExtData(quint32 sourceType, QString data);
    void onTmrCheckTaskTimeout();//to task manager
    void startTmrCheckTaskNow(int msec);
    void onTaskReady(quint32 taskId, quint8 rez, QString qckType, qint64 dtLastExchange);
    void stopAllConfModem();

    void resetStopAllConfModem();


    void killPeredavatorRequest();
    void resetCoordinatorRequest();//hardware
    void onAboutZigBee(QVariantHash hash);


    void closeDbConnection();

    void pollEvent(QString line);

    void add2logWarn(QStringList warnlist);
    void add2logError(QStringList errlist);


    void restartKillTmr();

    void stopAllCloseDatabaseNow();

    void allowDatabase();

    void reloadPollSettings();

    void meterInfoChanged(QDateTime dtlocal, QString line);

    void meterInfoChangedExt(QDateTime dtlocal, QString ni, QString model, int retry, quint8 pollCode, int taskCounter, int taskSrc, qint64 time4poll, quint8 writePrtt, bool isBlockByPrtt);

    void onSleepCommandDone();
    void flushSmplExecStr2db();

#ifdef HASGUI4USR
    void showHexDump(QByteArray arr, QString ifaceName, bool isRead);

#endif


#ifdef ENABLE_EXTSUPPORT_OF_IFACES

    void onConnectionStateChanged(bool isActive);

    void onConnectionDown();

    void onReadWriteOperation(bool isRead);

#endif

    void add2forwardTable(QString lastMeterId, QString ni);

    void pollStatisticChanged(QStringList listOneMeter, QString ni, QString pollCode, bool hasGoodExchange);

    void pollStatisticChanged2history(QStringList listOneMeter);

    void onUsfulDataLen(QString ni, quint8 pollCode, int bytes);


    //internal
    void trafficFromDevices(QByteArray readarr);

public slots:

    void initTheLordOfIfaces();

    void startTmrCheckTask(int msec);

    void closeConnectionLater();

    void closeConnection();

    void closeConnectionNow();

    void stopAllSlot();

    void stopAllSlotDirect();

    void stopAllCloseDatabase();


    void resetNoAnswerStat();

    void resetStatistic();

    void add2logWarnSlot(const QStringList &warnlist);
    void add2logErrorSlot(const QStringList &errlist);

    void pollEventSlot(const QString &line);
    void updatePollHslot(const quint8 &currPollCode, const qint64 &msecElpsd, const int &depth, const QDateTime &dtPoll);


    void resetTimers();

    void onTcpDisconn();



    void setPollSaveSettings(quint16 meterRetryMax, quint16 meterRetryMaxFA, bool hardAddrsn, bool enableW4E, bool corDTallow, qint32 messCountBeforeReady, qint32 messCountAfter, qint32 corDTintrvl);
    void setPowerSleepProfileSettings(bool enablePowerSleepCheck, int go2sleepSeconds);


#ifdef ENABLE_EXTSUPPORT_OF_IFACES
    void setThisIfaceSett(QVariantMap interfaceSettings);
    void onConnectionDownSlot();
#else
    void setZbyratorCommunicationSett(QString host, quint16 port, qint32 timeoutB, qint32 timeoutG);
#endif



    void meterInfoChangedSlot(const QString &mess);

    void meterInfoChangedSlotExt(const UniversalMeterSett &oneMeter, const PollDateMemoExt &pollDt, const CurrentMeterPoll &meterInfo);


    void lockAllSleepMessages(bool lock);

    void onSleepCommandSended(const bool &oneSended, const QStringList &model2remove);

    void onDataFromIface(const QByteArray &readArr, const bool &uartIsBusy);

    void updateStatistic(const CurrentMeterPoll &meter);

    void preparyWarnAndErrorLogs(const QVariantHash &hashTmpData, const QVariantHash &hashConstData);

};

#endif // THELORDOFIFACES_H
