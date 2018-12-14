#include "thelordofifaces.h"

#include <QThread>
#include <QTimer>

#include "ifaceexchange.h"

#include "src/emb/embeelimits.h"
//#include "src/meter/zbyratorhelper.h"

#include "matildalimits.h"
#include "task_status.h"
#include "moji_defy.h"

TheLordOfIfaces::TheLordOfIfaces(const quint32 &dbgSrcType, const bool &verboseMode, const ZbyrConnSett &connSett, QObject *parent) : QObject(parent), dbgSrcType(dbgSrcType), verboseMode(verboseMode)
{
#ifdef ENABLE_VERBOSE
    activeDbgMessages = true;
#else
    activeDbgMessages = false;
#endif

#ifdef ENABLE_EXTSUPPORT_OF_IFACES
    Q_UNUSED(connSett);
#else
    if(connSett.prdvtrAddr.isEmpty())
        setZbyratorCommunicationSett("::1", 8989, 500, 11000);
#endif
    dtNowValid = false;
    stopAll = false;
    pmState.powerManagementSettChecked = pmState.lockAllSleepMessages = false;
}

//-----------------------------------------------------------------------------

bool TheLordOfIfaces::isTime4updateAboutModem(const QVariantHash &aboutModem)
{
    //SharedMemoProtocolHelper::readFromSharedMemory(MTD_EXT_NAME_ABOUT_ZB);  #include "src/shared/sharedmemoprotocolhelper.h"

      //AD, PL, CH, ID, EUI64 (SH:SL), HV, VR, HP
    const QDateTime dtLastUpdate = QDateTime::fromString(aboutModem.value("Updated").toString(), "yyyy-MM-dd hh:mm:ss");

    return (!dtLastUpdate.isValid() || qAbs(dtLastUpdate.msecsTo(QDateTime::currentDateTime()) ) > (MAX_MSEC_BETWEEN_UPDATE_ABOUTM));
}

//-----------------------------------------------------------------------------

qint64 TheLordOfIfaces::checkTime4multicast()
{
    if(zbyrExchngOneMeterStat.time4poll.elapsed() < MAX_TIME_FOR_MULTICAST_MSEC && zbyrExchngOneMeterStat.time4multicast.elapsed() > MAX_TIME_FOR_MULTICAST_MSEC){
        zbyrExchngOneMeterStat.time4multicast.restart();//має перезапускатися перед читанням, що в загальному період був трішки меншим
        return suspendPoll(17000);
    }
    return 0;
}

//-----------------------------------------------------------------------------

qint64 TheLordOfIfaces::suspendPoll(const int &msec)
{
    QTime time;
    time.restart();
    for(int i = 0; i < MAX_READ_TRYES_FROM_UART && time.elapsed() < msec && !stopAll; i++){
        lastIface.confModemHelper->readAll();
        QThread::msleep(100);
    }

    return time.elapsed();
}

//-----------------------------------------------------------------------------

bool TheLordOfIfaces::incrementTotalMessCounter()
{
    if(zbyrExchngOneMeterStat.totalMessCounter > SETT_MAX_IDENTICAL_MESS_PER_SESION){
        if(!IfaceExchange::unikalnistPovidomlenVnormi(zbyrExchngOneMeterStat.hashMessRetryCounter)){

            if(verboseMode)
                qDebug() << "!unikalnistPovidomlenVnormi";

            return true;
        }
    }
    return false;
}

//-----------------------------------------------------------------------------

void TheLordOfIfaces::wait4signalReady()
{
    if(verboseMode)
        qDebug() << "wait4signalReady "  << pollConstSett.enableW4E;

    if(!pollConstSett.enableW4E){
        lastPollObjState.embeeReady = true;
        lastPollObjState.block4waitSignal = 0;
        return;
    }

    emit pollEvent(tr("Waiting for the ready signal..."));

    lastPollObjState.block4waitSignal = true;
    lastPollObjState.block4waitStarted = QDateTime::currentDateTimeUtc();
}
//-----------------------------------------------------------------------------
bool TheLordOfIfaces::checkConnection2coordinator(const bool &simpleConnection)
{
    return !((!lastIface.confModemHelper->isConnectionWorks(3333) && (!connect2peredavator(simpleConnection) || !lastIface.confModemHelper->isCoordinatorFree())) || (lastIface.confModemHelper->isUartBusy() && !lastIface.confModemHelper->isCoordinatorFree()));
//        return false;// RD_EXCHANGE_CRDNTR_BUSY;
    //    return true;
}

bool TheLordOfIfaces::connect2peredavator(const bool &simpleConnection)
{
    return connect2peredavator(simpleConnection, QVariantHash());
}

//-----------------------------------------------------------------------------

bool TheLordOfIfaces::connect2peredavator(const bool &simpleConnection, const QVariantHash &lastinfoaboutcoordinator)
{
    lastIface.confModemHelper->closeDevice();

#ifdef ENABLE_EXTSUPPORT_OF_IFACES
    emit onConnectionStateChanged(true);
    emit resetStopAllConfModem();
    bool r;
    switch(connSett.connectionType){
    case 1  : r = lastIface.confModemHelper->openTcpConnection(connSett.prdvtrAddr, connSett.prdvtrPort); break;
    case 2  : r = lastIface.confModemHelper->openM2mConnection(connSett.m2mhash); break;
    default : r = lastIface.confModemHelper->openSerialPort(connSett.prdvtrAddr, connSett.prdvtrPort, connSett.uarts); break;
    }



    if(r && (simpleConnection || lastIface.confModemHelper->isCoordinatorGood(lastPollObjState.updateAboutModemForced , false))){
        if(!simpleConnection)
            lastPollObjState.updateAboutModemForced = false;
        retry2peredavator = 0;

        return true;
    }
    emit onConnectionStateChanged(false);

    lastIface.confModemHelper->closeDevice();

    emit pollEvent(tr("can't connect to the coordinator"));


#else

    if(lastIface.confModemHelper->openTcpConnection(connSett.prdvtrAddr, connSett.prdvtrPort) && (simpleConnection || lastIface.confModemHelper->isCoordinatorGood(lastPollObjState.updateAboutModemForced , isTime4updateAboutModem(lastinfoaboutcoordinator)))){
        if(!simpleConnection)
            lastPollObjState.updateAboutModemForced = false;
        retry2peredavator = 0;
        return true;
    }
    const bool tcpok = lastIface.confModemHelper->isConnectionWorks();
    const quint8 maxPeredavator = (tcpok && lastIface.confModemHelper->checkUartAccess(lastIface.confModemHelper->readDeviceQuick("\r\n", true), (qint32)retry2peredavator * 1000 )) ? 120 : 10;


    if(activeDbgMessages )
        emit appendDbgExtData(dbgSrcType, QString("connect2peredavator host=%1, port=%2, err=%3, retry2peredavator=%4, isBusy=%5").arg(connSett.prdvtrAddr).arg(connSett.prdvtrPort).arg(lastIface.confModemHelper->socket->errorString()).arg((int)retry2peredavator).arg(lastIface.confModemHelper->isUartBusy()));

    if(retry2peredavator > maxPeredavator){
        retry2peredavator = 0;
        emit killPeredavatorRequest();
    }else{
        retry2peredavator++;
    }
//    emit command2extensionClient(MTD_EXT_COMMAND_KILL_PEREDAVATOR, true);
    if(!tcpok)
        emit pollEvent(tr("can't connect to the coordinator, err: %1").arg(lastIface.confModemHelper->socket->errorString()));
#endif
    return false;
}

//-----------------------------------------------------------------------------

quint8 TheLordOfIfaces::readDecodeEmbeeData(const UniversalMeterSett &oneMeter, const PollDateMemoExt &pollDt, CurrentMeterPoll &meterInfo, const int &daMsec)
{
    Q_UNUSED(meterInfo);
    const quint8 pollCode = pollDt.pollCode;
    if(activeDbgMessages)
        emit appendDbgExtData(dbgSrcType, QString("readDecodeEmbeeData pollCode%1, ni=%2").arg(pollDt.pollCode).arg(oneMeter.ni));


    if(pollCode == 0)
        return  RD_EXCHANGE_BAD_TASK;

    if(!checkConnection2coordinator(true))
        return RD_EXCHANGE_CRDNTR_BUSY;

    if(lastIface.confModemHelper->isUartBusy() && lastIface.confModemHelper->checkUartAccess(lastIface.confModemHelper->readDeviceQuick("\r\n", true), daMsec))
        return RD_EXCHANGE_CRDNTR_BUSY;

    QString mess;
    const bool r = lastIface.confModemHelper->networkReset(mess) ;

    if(lastIface.confModemHelper->isUartBusy())
        return RD_EXCHANGE_CRDNTR_BUSY;

    if(r)
        closeConnectionNow();
    else
        emit pollEvent(mess);

    if(activeDbgMessages)
        emit appendDbgExtData(dbgSrcType, QString("readDecodeEmbeeData pollCode%1, ni=%2").arg(pollDt.pollCode).arg(oneMeter.ni));


    return r ? RD_EXCHANGE_DONE_WO_POLL : RD_EXCHANGE_NALL;//RD_EXCHANGE_DONE -
}

//-----------------------------------------------------------------------------

quint8 TheLordOfIfaces::wait4embeeReady()
{
    if(lastPollObjState.block4waitSignal){
        const QDateTime dt = QDateTime::currentDateTimeUtc();

        if(qAbs(dt.msecsTo(lastPollObjState.block4waitStarted)) < MAX_MSEC_4_EMB_READY && lastPollObjState.block4waitStarted.isValid()){
            const QByteArray readArr = lastIface.confModemHelper->readDevice();//уже з перевіркою прямого доступу readFromSocket("\r\n", "", "", -1, "");

            if(lastIface.confModemHelper->isDirectAccess()){
                if(activeDbgMessages)
                    emit appendDbgExtData(dbgSrcType, QString("wait4embeeReady directAccess=true"));
                return RD_EXCHANGE_NALL;
            }

            if(readArr.isEmpty())
                return RD_EXCHANGE_NALL;

            if(readArr.contains("\r\n") && readArr != "OK\r\n"){
                emit trafficFromDevices(readArr);

            }else{
                if(activeDbgMessages)
                    emit appendDbgExtData(dbgSrcType, QString("wait4embeeReady readArr=%1").arg(QString(readArr.toHex())));
            }
            lastPollObjState.block4waitStarted = dt;

            emit restartKillTmr();
            return RD_EXCHANGE_NALL;
        }
    }
    emit pollEvent(tr("The network is ready"));

    if(activeDbgMessages)
        emit appendDbgExtData(dbgSrcType, QString("wait4embeeReady resumePoll=true, elapsed=%2 [msec]").arg(lastPollObjState.block4waitStarted.msecsTo(QDateTime::currentDateTimeUtc())));
    lastPollObjState.block4waitSignal = false;
    return RD_EXCHANGE_DONE;
}

//-----------------------------------------------------------------------------

bool TheLordOfIfaces::isTime2breakRead(const QByteArray &readArr, const qint32 &readLen, const qint32 &lastLen, const qint32 &apiMaskLen, const QByteArray &answr, const QByteArray &endSymb, const QByteArray &endSymb2)
{
    if(readLen >= 0 &&  (readLen + apiMaskLen) <= lastLen)//(readLen + apiMaskLen) >= lastLen){
        return true;

    if(!answr.isEmpty() && readArr.endsWith(answr))
        return true;

    if(!endSymb.isEmpty() && readArr.endsWith(endSymb) /*&& readLen == 0*/)
        return true;

    if(!endSymb2.isEmpty()){
        QByteArray readArr2(readArr);
        readArr2.chop(1);
        if(readArr2.right(endSymb2.length()) == endSymb2 )
            return true;
    }
    return false;
}

//-----------------------------------------------------------------------------

bool TheLordOfIfaces::isCommandOk4ok(const QByteArray &readArr, const bool &multicastOrOK)
{
    return (multicastOrOK && (readArr == "OK\r\n" || readArr == "ERROR\r\n"));
}
//-----------------------------------------------------------------------------
TheLordOfIfaces::SendGo2sleepParams TheLordOfIfaces::getNextDoNotSleepCount(const quint8 &meterType)
{
    SendGo2sleepParams rez;
    //power management DoNotSleep
    rez.msecDt = QDateTime::currentMSecsSinceEpoch();
    const qint64 diffMsec = ((pmState.sendCounter < 3) ? METER_DO_NOT_SLEEP_MINUTES_CHEKMIN : METER_DO_NOT_SLEEP_MINUTES_CHEK) * 60 * 1000;
    const qint64 msecDtLast = rez.msecDt - diffMsec;

    rez.sendNotSleepCount = 0;
    const QList<QString> lk = meterModel2powerManagement.keys();

    for(int i = 0, imax = lk.size(); i < imax; i++){
        if(meterModel2powerManagement.value(lk.at(i)).meterType != meterType)
            continue;

        if(verboseMode)
            qDebug() << "powerManager check send " << QDateTime::fromMSecsSinceEpoch(meterModel2powerManagement.value(lk.at(i)).msecLastSendMess).toLocalTime().toString("yyyy/MM/dd hh:mm:ss")
                     << QDateTime::fromMSecsSinceEpoch(msecDtLast).toLocalTime().toString("yyyy/MM/dd hh:mm:ss");

        if(meterModel2powerManagement.value(lk.at(i)).msecLastSendMess > msecDtLast)
            continue;

        rez.listDoNotSleep.append(lk.at(i));
        rez.sendNotSleepCount++;
    }


    if(activeDbgMessages)
        emit appendDbgExtData(dbgSrcType, QString("sendNextDoNotSleep meterType=%1, sendNotSleepCount=%2, models=%3").arg(int(meterType)).arg(rez.sendNotSleepCount).arg(lk.join(" ")));

    if(rez.sendNotSleepCount > 0)
        rez.lk = lk;
    return rez;

}
//-----------------------------------------------------------------------------
void TheLordOfIfaces::resetPmStateByModel(const bool &modelDone, const QString &model, const qint64 msecDt, const int &doNotSleepDone, const quint8 &meterType)
{
    if(modelDone){

        MetersPowerManagement pm = meterModel2powerManagement.value(model);
        pm.msecLastSendMess = msecDt;
        meterModel2powerManagement.insert(model, pm);
        if(verboseMode)
            qDebug() << "powerManager updated " << QDateTime::fromMSecsSinceEpoch(msecDt).toLocalTime().toString("yyyy/MM/dd hh:mm:ss");


    }

    if(activeDbgMessages)
        emit appendDbgExtData(dbgSrcType, QString("sendNextDoNotSleep meterType=%1, doNotSleepDone=%2, sendCounter=%3, modelDone=%4")
                              .arg(int(meterType)).arg(doNotSleepDone).arg(int(pmState.sendCounter)).arg(int(modelDone)));
}
//-----------------------------------------------------------------------------
void TheLordOfIfaces::incrementSleepCounter(const int &doNotSleepDone, const quint8 &meterType)
{
    if(doNotSleepDone > 0 && (pmState.sendCounter < 100)){
        pmState.sendCounter++;
    }

    if(activeDbgMessages)
        emit appendDbgExtData(dbgSrcType, QString("sendNextDoNotSleep meterType=%1, doNotSleepDone=%2, sendCounter=%3")
                              .arg(int(meterType)).arg(doNotSleepDone).arg(int(pmState.sendCounter)));
}
//-----------------------------------------------------------------------------
bool TheLordOfIfaces::canIsendGo2sleep4all(bool &rez)
{
    rez = false;
    if(activeDbgMessages)
        emit appendDbgExtData(dbgSrcType, QString("sendGo2sleep4all meterModel2powerManagement.size=%1, enablePowerManagement=%2, sendSleepCommandForced=%3, lockAllSleepMessages=%4")
                              .arg(int(meterModel2powerManagement.size())).arg(int(pmState.enablePowerManagement)).arg(int(pmState.sendSleepCommandForced)).arg(pmState.lockAllSleepMessages));

    if(stopAll || meterModel2powerManagement.isEmpty() || pmState.lockAllSleepMessages){
        rez = true;
        return false;
    }

    if(!lastIface.confModemHelper->isConnectionWorks()){
        if(meterModel2powerManagement.isEmpty() || stopAll){
            rez = true;
            return false;
        }

        if(!checkConnection2coordinator(false)){
            rez = false;//можлива ситуація коли не відправив команду сну, а з'єдннання було розірвано, тому перевіряю з'єднання
            return false;
        }

    }
    return true;
}

//-----------------------------------------------------------------------------

bool TheLordOfIfaces::isReleaseDateValid(const QDateTime &currDtUtc)
{

    if(currDtUtc < dtRelease && dtRelease.isValid()){
        if(!dtNowValid)
            emit pollEvent(tr("Device's date is invalid. The device's date is %1, date of the release is %2").arg(currDtUtc.toLocalTime().toString("yyyy-MM-dd hh:mm:ss")).arg(dtRelease.toString("yyyy-MM-dd hh:mm:ss")));
        dtNowValid = true;
        if(verboseMode)
            qDebug() << QString("readDecodeMeterData dtRelease= " + dtRelease.toString("yyyy-MM-dd hh:mm:ss") + ", currDt=" + QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
        return false;
    }
    if(dtNowValid)
        dtNowValid = false;
    return true;

}

void TheLordOfIfaces::checkEnablePowerManagement4dev(const bool &enablePowerManagement, const quint8 &meterType, const QString &model)
{
    //при переході опитування з лічильників води на е/е не відсилає команду сну, тому необхідно переглянути це місце
        if(pmState.model2hasPmMessage.value(model, true)){//if this model has a message to a meter to disable a sleep mode
            if((pmState.enablePowerManagement && !enablePowerManagement) || !pmState.lastSended.isValid() || qAbs(QDateTime::currentDateTimeUtc().secsTo(pmState.lastSended)) > 300){
                pmState.sendSleepCommandForced = true;

            }else{
                if(enablePowerManagement)
                    pmState.sendSleepCommandForced = false;
            }
            if((pmState.powerManagementSettChecked && pmState.lastMeterType != meterType) || pmState.sendSleepCommandForced){
                pmState.powerManagementSettChecked = false;
                pmState.sendSleepCommandForced = true;
            }
        }else{
            //this model doesnt have a sleep mode
            pmState.sendSleepCommandForced = false;
            pmState.powerManagementSettChecked = true;
    //        pmState.lastSended = QDateTime::currentDateTimeUtc().addSecs()

        }
        pmState.lastMeterType = meterType;
        pmState.enablePowerManagement = enablePowerManagement;

}

//-----------------------------------------------------------------------------

void TheLordOfIfaces::initTheLordOfIfaces()
{
    IfaceExchangeSerializedTypes::makeRegistration();
    qRegisterMetaType<CurrentMeterPoll>("CurrentMeterPoll");
    retry2peredavator = 0;
    if(activeDbgMessages){
        connect(this, SIGNAL(pollEvent(QString)), this, SLOT(pollEventSlot(QString)) );
        connect(this, SIGNAL(updatePollH(quint8,qint64,int,QDateTime)), this, SLOT(updatePollHslot(quint8,qint64,int,QDateTime)) );
        connect(this, SIGNAL(add2logError(QStringList)), this, SLOT(add2logErrorSlot(QStringList)) );
        connect(this, SIGNAL(add2logWarn(QStringList)), this, SLOT(add2logWarnSlot(QStringList)) );
    }


    QTimer *taskTimer = new QTimer(this);
    taskTimer->setInterval(111);
    taskTimer->setSingleShot(true);
    connect(this, SIGNAL(startTmrCheckTaskNow(int)), taskTimer, SLOT(start(int)) );
    connect(taskTimer, SIGNAL(timeout()), this, SIGNAL(onTmrCheckTaskTimeout()) );

    //reset task state
    lastTaskState.noTasks = true;
    lastTaskState.taskId = -1;

    lastIface.confModemHelper = new Conf2modem(dbgSrcType, verboseMode, this);
    lastIface.confModemHelper->createDevices();// setTcpDevice(lastIface.confModemHelper->socket);
    lastIface.confModemHelper->setTimeouts(6500, 200);

    connect(lastIface.confModemHelper, SIGNAL(onConnectionClosed()), this, SLOT(onTcpDisconn()) );


    connect(lastIface.confModemHelper, &Conf2modem::killPeredavatorRequest  , this, &TheLordOfIfaces::killPeredavatorRequest    );
    connect(lastIface.confModemHelper, &Conf2modem::resetCoordinatorRequest , this, &TheLordOfIfaces::resetCoordinatorRequest   );
    connect(lastIface.confModemHelper, &Conf2modem::onAboutZigBee           , this, &TheLordOfIfaces::onAboutZigBee             );
    connect(lastIface.confModemHelper, &Conf2modem::appendDbgExtData        , this, &TheLordOfIfaces::appendDbgExtData          );
#ifndef HASGUI4USR //якщо дозволити то буде дублювання подій в лозі програми
    connect(lastIface.confModemHelper, &Conf2modem::currentOperation        , this, &TheLordOfIfaces::meterInfoChangedSlot      );
#endif
    connect(lastIface.confModemHelper, &Conf2modem::currentOperation        , this, &TheLordOfIfaces::pollEvent                 );



    connect(lastIface.confModemHelper, &Conf2modem::onApiModeEnabled    , this, &TheLordOfIfaces::resetTimers);
    connect(lastIface.confModemHelper, &Conf2modem::openingTcpConnection, this, &TheLordOfIfaces::resetTimers);
//    connect(lastIface.confModemHelper, &Conf2modem::onDaStateChanged    , this, &TheLordOfIfaces::resetTimers);

    connect(this, &TheLordOfIfaces::stopAllConfModem, lastIface.confModemHelper, &Conf2modem::stopAllSlot);
    connect(this, &TheLordOfIfaces::resetStopAllConfModem, lastIface.confModemHelper, &Conf2modem::resetStopAllConfModem);

    stopAll = closeDataBase = false;


#ifdef ENABLE_EXTSUPPORT_OF_IFACES
    connect(lastIface.confModemHelper, SIGNAL(dataReadWriteReal(QByteArray,QString,bool)), this, SIGNAL(showHexDump(QByteArray,QString,bool)) );
    connect(lastIface.confModemHelper, SIGNAL(onConnectionDown()), this, SLOT(onConnectionDownSlot()) );
    connect(lastIface.confModemHelper, SIGNAL(onConnectionDown()), this, SIGNAL(onConnectionDown()) );

    connect(lastIface.confModemHelper, SIGNAL(onReadWriteOperation(bool)), this, SIGNAL(onReadWriteOperation(bool)) );

#endif
    resetTimers();
    emit reloadPollSettings();
}

//-----------------------------------------------------------------------------

void TheLordOfIfaces::startTmrCheckTask(int msec)
{
    emit startTmrCheckTaskNow(msec );

}

//-----------------------------------------------------------------------------

void TheLordOfIfaces::closeConnectionLater()
{
    if(lastIface.closeConnLater > 180)
        closeConnection();
    else
        lastIface.closeConnLater++;
}

//-----------------------------------------------------------------------------

void TheLordOfIfaces::closeConnection()
{
    if(!lastTaskState.noTasks){
        lastTaskState.noTasks = true;
        lastIface.confModemHelper->closeDevice();

#ifdef HASGUI4USR
    emit flushSmplExecStr2db();
#endif
    }
}

//-----------------------------------------------------------------------------

void TheLordOfIfaces::closeConnectionNow()
{
    lastIface.confModemHelper->closeDevice();
    retry2peredavator = 0;
}

//-----------------------------------------------------------------------------

void TheLordOfIfaces::stopAllSlot()
{
    stopAll = true;
    emit stopAllConfModem();
}

//-----------------------------------------------------------------------------

void TheLordOfIfaces::stopAllSlotDirect()
{
    stopAll = true;
    emit stopAllConfModem();
}

//-----------------------------------------------------------------------------

void TheLordOfIfaces::stopAllCloseDatabase()
{
    stopAll = true;
    closeDataBase = true;
    emit flushSmplExecStr2db();
}

//-----------------------------------------------------------------------------

void TheLordOfIfaces::resetNoAnswerStat()
{
    zbyrExchngOneMeterStat.emptyAnswerCounter = 0;
    zbyrExchngOneMeterStat.timeFromEmptyChanged.restart();
}

//-----------------------------------------------------------------------------

void TheLordOfIfaces::resetStatistic()
{
    zbyrExchngOneMeterStat.totalMessCounter = 0;
    zbyrExchngOneMeterStat.hashMessRetryCounter.clear();
    zbyrExchngOneMeterStat.byte2meter = 0;
    zbyrExchngOneMeterStat.bytesFromMeter = 0;
    zbyrExchngOneMeterStat.usflDataLen = 0;
    zbyrExchngOneMeterStat.startPollDate = QDateTime::currentDateTime();
    lastIface.confModemHelper->resetBlockByPrtt();
    lastIface.confModemHelper->resetDaState();
}

//-----------------------------------------------------------------------------

void TheLordOfIfaces::add2logWarnSlot(const QStringList &warnlist)
{
    if(warnlist.isEmpty())
        return;
    const int imax = warnlist.size();
    emit appendDbgExtData(dbgSrcType, QString("add2logWarnSlot list=%1").arg(imax));
    for(int i = 0; i < imax; i++)
        emit appendDbgExtData(dbgSrcType, warnlist.at(i));
}

//-----------------------------------------------------------------------------

void TheLordOfIfaces::add2logErrorSlot(const QStringList &errlist)
{
    if(errlist.isEmpty())
        return;
    const int imax = errlist.size();
    emit appendDbgExtData(dbgSrcType, QString("add2logErrorSlot list=%1").arg(imax));
    for(int i = 0; i < imax; i++)
        emit appendDbgExtData(dbgSrcType, errlist.at(i));
}

//-----------------------------------------------------------------------------

void TheLordOfIfaces::pollEventSlot(const QString &line)
{
    if(line.isEmpty())
        return;

    emit appendDbgExtData(dbgSrcType, QString("pollEventSlot line=%1").arg(line));

}

//-----------------------------------------------------------------------------

void TheLordOfIfaces::updatePollHslot(const quint8 &currPollCode, const qint64 &msecElpsd, const int &depth, const QDateTime &dtPoll)
{
    emit appendDbgExtData(dbgSrcType, QString("updatePollHslot code=%1, msec=%2, depth=%3, dtPoll=%4").arg((int)currPollCode).arg(msecElpsd).arg(depth).arg(dtPoll.toString("yyyy/MM/dd hh:mm:ss.zzz t")));

}

//-----------------------------------------------------------------------------

void TheLordOfIfaces::resetTimers()
{
    zbyrExchngOneMeterStat.time4multicast.restart();
    zbyrExchngOneMeterStat.timeFromEmptyChanged.restart();
    zbyrExchngOneMeterStat.time4poll.restart();
}

//-----------------------------------------------------------------------------

void TheLordOfIfaces::onTcpDisconn()
{
    emit appendDbgExtData(dbgSrcType, QString("onTcpDisconn"));

}

//-----------------------------------------------------------------------------

void TheLordOfIfaces::setPollSaveSettings(quint16 meterRetryMax, quint16 meterRetryMaxFA, bool hardAddrsn, bool enableW4E, bool corDTallow, qint32 messCountBeforeReady, qint32 messCountAfter, qint32 corDTintrvl)
{
    pollConstSett.meterRetryMax         = meterRetryMax         ;
    pollConstSett.meterRetryMaxFA       = meterRetryMaxFA       ;
    pollConstSett.hardAddrsn            = hardAddrsn            ;
    pollConstSett.enableW4E             = enableW4E             ;
    pollConstSett.corDTallow            = corDTallow            ;
    pollConstSett.messCountBeforeReady  = messCountBeforeReady  ;
    pollConstSett.messCountAfter        = messCountAfter        ;
    pollConstSett.corDTintrvl           = corDTintrvl           ;
}

//-----------------------------------------------------------------------------

void TheLordOfIfaces::setPowerSleepProfileSettings(bool enablePowerSleepCheck, int go2sleepSeconds)
{
    pollConstSett.checkPowerSleepProfile = enablePowerSleepCheck;
    if(pmState.powerManagementSettChecked && pollConstSett.go2sleepPowerSleepProfile != go2sleepSeconds)
        pmState.powerManagementSettChecked = false;

    if(pmState.powerManagementSettChecked && meterModel2powerManagement.isEmpty())
        pmState.powerManagementSettChecked = false;

    pollConstSett.go2sleepPowerSleepProfile = go2sleepSeconds;

    if(activeDbgMessages)
        emit appendDbgExtData(dbgSrcType, QString("setPowerSleepProfileSettings enablePowerSleepCheck=%1, go2sleepSeconds=%2, powerManagementSettChecked=%3, meterModel2powerManagement.isEmpty=%4")
                              .arg(int(enablePowerSleepCheck)).arg(go2sleepSeconds).arg(int(pmState.powerManagementSettChecked)).arg(int(meterModel2powerManagement.isEmpty())));

}

//-----------------------------------------------------------------------------
#ifdef ENABLE_EXTSUPPORT_OF_IFACES

void TheLordOfIfaces::setThisIfaceSett(QVariantMap interfaceSettings)
{
    stopAll = closeDataBase = false;
    connSett.connectionType = interfaceSettings.value("ifaceMode").toInt();

    switch(connSett.connectionType){
    case 1  : connSett.prdvtrAddr   = interfaceSettings.value("hostName", "::1").toString(); connSett.prdvtrPort = interfaceSettings.value("port", 8989).toUInt(); break;
    case 2  : connSett.m2mhash      = interfaceSettings.value("m2mProfile").toHash(); break;
    default : connSett.uarts        = interfaceSettings.value("uarts").toStringList(); connSett.prdvtrAddr = interfaceSettings.value("uart").toString(); connSett.prdvtrPort = interfaceSettings.value("baudRate").toInt(); break;

    }
    QString ifaceParams;
    switch(connSett.connectionType){
    case 1  : ifaceParams = QString("%1\n%2").arg(connSett.prdvtrAddr).arg(connSett.prdvtrPort); break;
    case 2  :{
        QList<QString> lk = connSett.m2mhash.keys();
        lk.removeOne("t");//timeout
        std::sort(lk.begin(), lk.end());
        for(int i = 0, imax = lk.size(); i < imax; i++)
            ifaceParams.append(QString("%1\n%2\n\r\n").arg(lk.at(i)).arg(connSett.m2mhash.value(lk.at(i)).toString()));
        break;}
    default : ifaceParams = QString("%1\n\n\n%2\n%3").arg(connSett.uarts.join("\r")).arg(connSett.prdvtrAddr).arg(connSett.prdvtrPort); break;
    }

    if(ifaceParams != lastIface.lastIfaceParams){
        lastPollObjState.updateAboutModemForced = true;
        lastIface.lastIfaceParams = ifaceParams;
    }

    connSett.timeOutG = interfaceSettings.value("timeoutMsec", 11000).toInt();
    connSett.timeOutB = interfaceSettings.value("blockTimeout", 300).toInt();

    lastIface.confModemHelper->setTimeouts(connSett.timeOutG, connSett.timeOutB);
}

//-----------------------------------------------------------------------------

void TheLordOfIfaces::onConnectionDownSlot()
{
    emit onConnectionStateChanged(false);

}
#else
//-----------------------------------------------------------------------------

void TheLordOfIfaces::setZbyratorCommunicationSett(QString host, quint16 port, qint32 timeoutB, qint32 timeoutG)
{
    if(connSett.prdvtrAddr != host || connSett.prdvtrPort != port || connSett.timeOutB != timeoutB || connSett.timeOutG != timeoutG){
        if(activeDbgMessages)
            emit appendDbgExtData(dbgSrcType, QString("setZbyratorCommunicationSett old=%1, %2, %3, %4 | new=%5, %6, %7, %8 ")
                                  .arg(connSett.prdvtrAddr).arg(connSett.prdvtrPort).arg(connSett.timeOutG).arg(connSett.timeOutB)
                                  .arg(host).arg(port).arg(timeoutG).arg(timeoutB));
        connSett = ZbyrConnSett(host, port, timeoutB, timeoutG);
    }
}
#endif
//-----------------------------------------------------------------------------

void TheLordOfIfaces::meterInfoChangedSlot(const QString &mess)
{
    emit meterInfoChanged(QDateTime::currentDateTime(), mess);

}

//-----------------------------------------------------------------------------

void TheLordOfIfaces::meterInfoChangedSlotExt(const UniversalMeterSett &oneMeter, const PollDateMemoExt &pollDt, const CurrentMeterPoll &meterInfo)
{
    emit meterInfoChangedExt(QDateTime::currentDateTime(), oneMeter.ni, oneMeter.model, meterInfo.meterRetry, pollDt.pollCode, meterInfo.taskCounter, meterInfo.taskSrc, zbyrExchngOneMeterStat.time4poll.elapsed(),lastIface.confModemHelper->getWritePrtt(),lastIface.confModemHelper->isBlockByPrtt());
    zbyrExchngOneMeterStat.time4poll.restart();
}

//-----------------------------------------------------------------------------

void TheLordOfIfaces::lockAllSleepMessages(bool lock)
{
    pmState.lockAllSleepMessages = lock;

}

//-----------------------------------------------------------------------------

void TheLordOfIfaces::onSleepCommandSended(const bool &oneSended, const QStringList &model2remove)
{
    if(oneSended)
        emit onSleepCommandDone();

    for(int i = 0, imax = model2remove.size(); i < imax; i++)
        meterModel2powerManagement.remove(model2remove.at(i));

    if(pmState.sendSleepCommandForced)
        pmState.lastSended = QDateTime::currentDateTimeUtc();

    pmState.wasOkFromTheModem = pmState.sendSleepCommandForced = false;
    pmState.sendCounter = 0;
    pmState.powerManagementSettChecked = false;//тільки тоді коли робота зроблена

    if(activeDbgMessages)
        emit appendDbgExtData(dbgSrcType, QString("sendGo2sleep4all done meterModel2powerManagement.size=%1, model2remove=%2, sendSleepCommandForced=%3").arg(meterModel2powerManagement.size()).arg(model2remove.join(" ")).arg(pmState.sendSleepCommandForced));


}
//-----------------------------------------------------------------------------
void TheLordOfIfaces::onDataFromIface(const QByteArray &readArr, const bool &uartIsBusy)
{
    if(verboseMode)
        IfaceExchange::showHexDump(readArr, " spread:");

    if(activeDbgMessages )
        emit appendDbgExtData(dbgSrcType, QString("readFromSocket exit, uartIsBusy=%1, readArrIsEmpty=%2").arg(uartIsBusy).arg(readArr.isEmpty()));
#ifdef HASGUI4USR
    emit showHexDump(readArr, lastIface.confModemHelper->getLastIfaceName(), true);
#endif
}

//-----------------------------------------------------------------------------

void TheLordOfIfaces::updateStatistic(const CurrentMeterPoll &meter)
{
    if(zbyrExchngOneMeterStat.totalMessCounter > 0){

        const QDateTime dt = QDateTime::currentDateTime();
        preparyWarnAndErrorLogs(meter.hashTmpData, meter.hashConstData);

        const qint64 secsElapsed = (zbyrExchngOneMeterStat.startPollDate.msecsTo(dt) - meter.timeElapsed4multicast)/1000;
        if(verboseMode){
            qDebug() << "============================================== Statistic ================================================";
            qDebug() << "zbyrExchngOneMeterStat.totalMessCounter " << zbyrExchngOneMeterStat.totalMessCounter;
            qDebug() << "Start End Date, UTC " << zbyrExchngOneMeterStat.startPollDate << zbyrExchngOneMeterStat.startPollDate.toUTC() ;
            qDebug() << "End Date , UTC " << dt << dt.toUTC();
            qDebug() << "Elapsed sec " << secsElapsed;
            qDebug() << "lastRetry " << meter.meterRetry;
            qDebug() << "Model version SN NI " << meter.hashConstData.value("model").toString() << meter.hashConstData.value("vrsn").toString() << meter.hashConstData.value("SN").toString() << meter.hashConstData.value("NI").toString();
            qDebug() << "code politic tariff" << meter.hashConstData.value("pollCode").toInt() << meter.hashConstData.value("listEnrg").toStringList().join(',') << meter.hashConstData.value("trff").toInt();
            qDebug() << "staticsti by message " << zbyrExchngOneMeterStat.usflDataLen;
        }
        QStringList list;
        list.append(meter.hashConstData.value("NI").toString());
        list.append(meter.hashConstData.value("model").toString());
        list.append(QString::number(meter.hashConstData.value("pollCode").toInt()));
        list.append(dt.toString("yyyy-MM-dd hh:mm:ss"));
        list.append(QString::number(zbyrExchngOneMeterStat.totalMessCounter));
        list.append(IfaceExchange::byte2humanRead(zbyrExchngOneMeterStat.byte2meter) + "/" + IfaceExchange::byte2humanRead(zbyrExchngOneMeterStat.bytesFromMeter) + "/" + IfaceExchange::byte2humanRead(zbyrExchngOneMeterStat.usflDataLen));//Write/Read/Useful Data
        list.append(QString::number(secsElapsed));
        list.append(QString::number(meter.meterRetry));

        if(verboseMode){
            foreach ( QByteArray key, zbyrExchngOneMeterStat.hashMessRetryCounter.keys()) {
                qDebug() << "count val " << zbyrExchngOneMeterStat.hashMessRetryCounter.value(key) << key.toHex().toUpper();
            }
            qDebug() << "============================================== End ======================================================";
            qDebug() << "goodbye";
        }


        if(meter.meterRetry == 0){
            list.append(dt.toString("yyyy-MM-dd hh:mm:ss"));//last good exchange size==9
            emit pollStatisticChanged2history(list);
        }else{
            emit pollStatisticChanged(list, list.first(), list.at(2), false); //size == 8
        }

        if(!meter.hashTmpData.value("lastMeterId").toString().isEmpty() && meter.hashConstData.value("NI").toString() != meter.hashTmpData.value("lastMeterId").toString())
            emit add2forwardTable(meter.hashTmpData.value("lastMeterId").toString(), meter.hashConstData.value("NI").toString());
    }
}

//-----------------------------------------------------------------------------

void TheLordOfIfaces::preparyWarnAndErrorLogs(const QVariantHash &hashTmpData, const QVariantHash &hashConstData)
{
    if(hashTmpData.isEmpty())
        return;

    QStringList listWarn, listError;
    for(int i = 0; i < 10000; i++){
        if(hashTmpData.contains(QString("Warning_%1").arg(i))){
            if(!hashTmpData.value(QString("Warning_%1").arg(i)).toString().isEmpty())
                listWarn.append(hashTmpData.value(QString("Warning_%1").arg(i)).toString());
        }else
            break;
    }
    QString aboutMeter = QString("NI:%1,SN:%2(%3),model:%4,code:%5")
            .arg(hashConstData.value("NI").toString())
            .arg(hashConstData.value("SN").toString())
            .arg(hashTmpData.value("SN").toString())
            .arg(hashConstData.value("model").toString())
            .arg(hashConstData.value("pollCode").toString());

    if(!listWarn.isEmpty()){
        listWarn.append(aboutMeter);
        emit add2logWarn(listWarn);
    }
    listWarn.clear();
    for(int i = 0; i < 10000; i++){
        if(hashTmpData.contains(QString("Error_%1").arg(i))){
            if(!hashTmpData.value(QString("Error_%1").arg(i)).toString().isEmpty())
                listError.append(hashTmpData.value(QString("Error_%1").arg(i)).toString());
        }else
            break;
    }
    if(!listError.isEmpty()){
        listError.append(aboutMeter);
        emit add2logError(listError);
    }
}



//-----------------------------------------------------------------------------
