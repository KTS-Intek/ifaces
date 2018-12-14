#include "conf2modem.h"
#include <QTime>
#include <QThread>

#include "matildalimits.h"
//#include "src/matilda/matildalimits.h"
#include "embeelimits.h"
#include "conf2modemhelper.h"


Conf2modem::Conf2modem(const int &dbgExtSrcId, const bool &verboseMode, QObject *parent) : QObject(parent), dbgExtSrcId(dbgExtSrcId), verboseMode(verboseMode)
{
    activeDbgMessages = (dbgExtSrcId > 0);
    createDevices();
    onDeviceDestr();

    writePrtt = 0;
    apiErrCounter = 0;
    msecWhenCoordinatorWasGood = 0;


}

quint16 Conf2modem::getConnectionDownCounter() const
{
    return connectionDownCounter;
}


void Conf2modem::createDevices()
{
    socket = new QTcpSocket(this);
    connect(socket, SIGNAL(disconnected()), this, SIGNAL(onConnectionClosed()) );

#ifdef ENABLE_EXTSUPPORT_OF_IFACES

        svahaConnector = new SvahaServiceConnector(this);
        connect(svahaConnector, SIGNAL(disconnected()), this, SIGNAL(onConnectionClosed()) );

        serialPort = new QSerialPort(this);
        CheckCurrPort *checkPort = new CheckCurrPort(this);

        connect(checkPort, SIGNAL(portDisconnected(bool)), this, SLOT(closeSerialPort()));
        connect(checkPort,SIGNAL(terminateNow()), this, SLOT(closeSerialPort()));

        connect(this, SIGNAL(onConnectionClosed()), checkPort, SLOT(terminate()) );
        connect(this, SIGNAL(stopCheckCurrPort()), checkPort, SLOT(terminate()) );
        connect(this, SIGNAL(onConnectionDown()), checkPort, SLOT(terminate()) );
        connect(this, SIGNAL(onSerialPortOpened(QString)), checkPort, SLOT(zapuskalka(QString)) );

#endif
}

quint8 Conf2modem::getWritePrtt() const
{
    return writePrtt;
}

QString Conf2modem::getLastIfaceName() const
{
    return ifaceName;
}

bool Conf2modem::isDirectAccess() const
{
    return directAccess;
}

bool Conf2modem::isBlockByPrtt() const
{
    return uartBlockPrtt;
}

bool Conf2modem::isUartBusy() const
{
    return (directAccess || uartBlockPrtt);
}

bool Conf2modem::enterCommandMode()
{

    QByteArray dataP("+++\r\n");/*tmpStr.toLocal8Bit();*/

    readAll();//clear a buffer


    write2dev(dataP);

    QByteArray readArr = readDeviceQuick("\r\n", false);
    if(readArr.isEmpty() && lastCommandWasAtcn){
        write2dev(dataP);
        readArr = readDevice();
    }

    if(sayGoodByeIfUartIsNFree("enterCommandMode 3643 ", 555))
        return false;



    if(readArr.left(6).toUpper() == "ERROR "){ //vidkryta proshyvka za4ekaty 4 cek i uvijty
        for(int i = 0; i < 5 && !stopAll; i++)
            lSleep(1000);

        readDevice();

        if(sayGoodByeIfUartIsNFree("enterCommandMode 3644 ", 555))
            return false;

        write2dev(dataP);

        readArr = readDevice();

    }

#ifdef IS_ZBYRATOR
    modemIsOverRS485 = (readArr == "O\r\n");
#endif
    for( int retry = 0, odyRaz = 0; retry < 7 && readArr.left(7).toUpper() != "OKERROR" && readArr.left(5).toUpper() != "ERROR" ; retry++) {//zminyty symvoly vhody v comandnyj rejym

        if(sayGoodByeIfUartIsNFree("enterCommandMode 3645 ", 555))
            return false;

        if(readArr.isEmpty()) {
            if(retry < 5){
                lSleep(((retry + 1) * 200));
            }else {
                for(int i = 6; i > 0; i--) {
                    if(sayGoodByeIfUartIsNFree("enterCommandMode 3653 ", 555))
                        return false;

                    lSleep(1000);
                }
            }
        }else{
            if(readArr.left(7).toUpper() != "OKERROR" && (readArr.left(2).toUpper() == "OK" || readArr.left(3).toUpper() == "O\r\n" || readArr.left(3).toUpper() == "K\r\n")){
                retry--;
            }
        }
        if(sayGoodByeIfUartIsNFree("directAccess 3654 ", 555))
            return false;


        write2dev(dataP);
        readArr = readDevice();
        if(retry == 6 && odyRaz == 0 && !readArr.isEmpty()){
            odyRaz = 1;
            retry--;
        }

#ifdef IS_ZBYRATOR
        if(readArr == "O\r\n")
            modemIsOverRS485 = true;
#endif

    }


    if(activeDbgMessages)  emit appendDbgExtData(dbgExtSrcId, QString("Conf2modem enterCommandMode 3643 directAccess=%1, uartBlockPrtt=%2, myPrtt=%3, readArr=%4, isEntered=%5")
                          .arg(directAccess).arg(uartBlockPrtt).arg(QString(writePreffix)).arg(QString(readArr.toHex())).arg((bool)(readArr.left(7).toUpper() == "OKERROR" || readArr.left(5).toUpper() == "ERROR")));

    return (readArr.left(7).toUpper() == "OKERROR" || readArr.left(5).toUpper() == "ERROR");
}

bool Conf2modem::networkReset(QString &errStr)
{

    for(int retr = 0; retr < 3; retr++){
        if(retr > 0)
            readAll();

        emit currentOperation(tr("Network reset"));

        if(stopAll){
            if(verboseMode)
                qDebug() << "power loss 3008";

            return true;
        }


        if(!enterCommandMode()){

            if(sayGoodByeIfUartIsNFree("networkReset ", 555))
                return false;

            errStr = tr("Can't enter a command mode(");
            continue;
        }

        writeATcommand("ATNR 5");

        QTime time;
        time.start();
        QByteArray readArr2 = readDevice();

        while(time.elapsed() < (9000) && !readArr2.contains("O") )
            readArr2.append(readDevice());


        if(readArr2.left(4) == "OK\r\n"){
            emit currentOperation(tr("Network reset..."));
            isCoordinatorConfigReady = false;
            return true;
        }
        errStr = tr("Unknown error (");
        writeATcommand("ATCN");
        readDevice();

    }
    return false;
}

bool Conf2modem::enableDisableApi(const bool &enable, const bool &readAboutZigBee)
{
    bool apiPlus = true;
    bool ifTrueATCN = true;

    bool breakNow = false;

    if(activeDbgMessages)  emit appendDbgExtData(dbgExtSrcId, QString("Conf2modem enableDisableApi a directAccess=%1, uartBlockPrtt=%2, myPrtt=%3, apiErrCounter=%4, readAboutZigBee=%5")
                          .arg(directAccess).arg(uartBlockPrtt).arg(QString(writePreffix)).arg(apiErrCounter).arg(int(readAboutZigBee)));

    bool wasOk4atfr = false;
    for(int i = 0; i < 3 && !breakNow; i++){
        emit currentOperation(tr("API enbl=%1, prtt=%2, rtr=%3").arg(enable).arg(QString(writePreffix)).arg(i));
        if(verboseMode)
            qDebug() << "ZbyratorObject::enableDisableApi ATFR " << enable << i << directAccess;

        if(!isCoordinatorFreeWithRead())
            continue;

        if(!isConnectionWorks())
            return false;


        if(!enterCommandMode()){
            emit currentOperation(tr("Can't enter a command mode."));


            if(!isCoordinatorFreeWithRead())
                readDeviceQuick("\r\n", false);

            continue;
        }

        writeATcommand("ATFR");
//        readDevice();

        QTime time;
        time.start();
        QByteArray readArr = readDevice().toUpper();
        if(readArr.isEmpty()){
            writeATcommand("ATCN");
            readDeviceQuick("\r\n", true);
            wasOk4atfr = true;
        }else{
            for(int nn = 0; nn < 1000 && time.elapsed() < 3000 && readArr != "OK\r\nOK\r\n" && !stopAll; nn++)
                readArr.append(readDevice().toUpper());

            wasOk4atfr = readArr.contains("OK");
        }

        if(wasOk4atfr)
            break;


    }

#ifdef IS_ZBYRATOR
    if(modemIsOverRS485)
        return true;
#endif
    if(!wasOk4atfr)
        return false;

    QVariantHash aboutModem;
    for(int i = 0; i < 3 && !breakNow; i++){
        qint32 prosh = 0;
        if(verboseMode)
            qDebug() << "ZbyratorObject::enableDisableApi " << enable << i;

        if(i < 1 && wasOk4atfr){
            readDeviceQuick("\r\n", true);
        }else{
            if(!isCoordinatorFreeWithRead())
                continue;

            if(!isConnectionWorks())
                return false;
        }
        if(!enterCommandMode()){
            emit currentOperation(tr("Can't enter a command mode."));

            if(!isCoordinatorFreeWithRead())
                continue;

            continue;
        }

        int indxLummerLog = 2;
        QStringList listArr;
        if(enable){
            writeATcommand("ATAD");

            if(true){
                const QByteArray readArr = readDevice();
                if(readArr.isEmpty())
                    continue;//priority access error
                if(readArr.toUpper() != "C\r\n"){
                    writeATcommand("ATAD C");
                    readDevice();
                    writeATcommand("ATSM 0");
                    readDevice();
                    ifTrueATCN = false;
                }
            }
            writeATcommand("ATVR");
            bool okkk;
            const QByteArray proshMy = readDevice();
            prosh = proshMy.mid(1,3).toInt(&okkk, 10);
            const bool fuckingEnrgmr = (proshMy.left(1) == "E" && okkk);

            emit currentOperation(tr("Checking the %1 mode.").arg(apiPlus ? "API" : "API+"));

            if(prosh >= 206 && okkk) {

                writeATcommand("ATSM");
                bool ok;
                const int tmpIntVal = readDevice().left(2).toInt(&ok);
                if(tmpIntVal != 0 || !ok){
                    ifTrueATCN = false;
                    writeATcommand("ATSM 0");
                    if(readDevice().toUpper() != "OK\r\n"){
                        emit currentOperation(tr("Can't configure the modem."));
                        lSleep(11111);
                        continue;
                    }
                }
            }

            QList<int> listVal;
            listVal << 1 << 0 << 1 << 0 ;

            listArr = QString("ATAP ATIR ATIU ATC1").split(' ');

            if(apiPlus){
                if(fuckingEnrgmr)
                    listVal.append(6);
                else
                    listVal.append(1);
                listArr.append("ATCP0");
                indxLummerLog++;

            }

            bool allGood = true;
             int retryList = 0;
            for(int la = 0, laMax = listArr.size(), laMax2 = listVal.size(); la < laMax && la < laMax2; la++){
                bool ok;
                writeATcommand(listArr.at(la));
                const QByteArray readArr = readDevice();
                int tmpIntVal = readArr.simplified().trimmed().toInt(&ok, 16);
                if(tmpIntVal != listVal.at(la) || !ok){
                    if(verboseMode)
                        qDebug() << "enableDisableApi checkVal " << ok << tmpIntVal << readArr << la << listArr.at(la) << listVal.at(la);

                    writeATcommand(listArr.at(la) + QString::number(listVal.at(la)));
                    if(readDevice().toUpper() != "OK\r\n"){
                        retryList++;
                        la--;
                        if(retryList > 1){
                            allGood = false;
                            break;
                        }
                    }else{
                        retryList = 0;
                    }
                }

            }
            listArr.clear();
            if(!allGood){
                emit currentOperation(tr("Can't configure the modem."));
                lSleep(6666);
                continue;
            }


            if(!ifTrueATCN){
                listArr.append("ATWR");
                emit currentOperation(tr("Enabling the %1 mode.").arg(apiPlus ? "API +" : "API"));
            }




        }else{
                listArr.append("ATCP0 0");
                listArr.append("ATMD 0");
                indxLummerLog = 1;


            listArr.append("ATAP 0");
            listArr.append("ATWR");
            emit currentOperation(tr("Disabling the %1 mode.").arg(apiPlus ? "API +" : "API"));
        }


        QStringList listAboutModem;

        if(readAboutZigBee){
            listAboutModem = QString("ATAD ATPL ATCH ATID ATSH ATSL ATHV ATVR ATDB").split(" ", QString::SkipEmptyParts);
            if(prosh > 203)
                listAboutModem.append("ATHP");

            listArr.append(listAboutModem);
            ifTrueATCN = false;//after ATDB reset a coordinator
        }

        listArr.append(ifTrueATCN ? "ATCN" : "ATFR");


        int retryList = 0;
        for(int i = 0, iMax = listArr.size(); i < iMax; i++){

            writeATcommand(listArr.at(i));
            QByteArray readArr = readDevice();

            if(!readArr.isEmpty() && listAboutModem.contains(QString(listArr.at(i).left(4)) )){
                if(listArr.at(i) == "ATDB"){
                    QTime time;
                    time.start();
                    for(int v = 0; v < 100 && time.elapsed() < 30000 && !(readArr.contains("LQI:") && readArr.lastIndexOf("\r\n") > readArr.indexOf("LQI:") ); v++){
                        readArr.append(readDevice());
                        if(readArr.contains("OK\r\n") || readArr.contains("ERROR\r\n"))
                            break;
                    }
                    if(readArr.contains("OK\r\n") || readArr.contains("ERROR\r\n"))
                        readArr.clear();

                    if(!(readArr.contains("LQI:") && readArr.lastIndexOf("\r\n") > readArr.indexOf("LQI:")))
                        readArr.clear();
                }
                aboutModem.insert(listArr.at(i), readArr.simplified().trimmed().toUpper());
                retryList = 0;
                continue;
            }

            if(readArr.toUpper() != "OK\r\n"){
                retryList++;
                i--;
            }else
                retryList = 0;

            if(retryList > 1)
                break;

        }

        if(retryList > 0){
            ifTrueATCN = false;
            emit currentOperation(tr("Can't configure the modem."));
        }else{
            if(!ifTrueATCN){ //write ATFR
                QTime time;
                time.start();
                QByteArray readArr = readDevice().toUpper();
                while(time.elapsed() < 11000 && readArr == "OK\r\nOK\r\n" && !stopAll)
                    readArr.append(readDevice().toUpper());

                if(!readArr.contains("OK\r\n")){
                    emit currentOperation( tr("Can't configure the modem."));
                    lSleep(6666);
                    continue;
                }

            }

            emit currentOperation(QString("The %1 mode is %2.").arg(apiPlus ? "API+" : "API").arg(enable ? "enabled" : "disabled"));


            if(readAboutZigBee && !aboutModem.isEmpty()){
                aboutModem = conf2modemHelper::aboutZigBeeModem2humanReadable(aboutModem);
                if(!aboutModem.isEmpty()){
                    aboutModem.insert("Updated", QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
                    emit onAboutZigBee(aboutModem);
                }
            }
            return true;
        }
    }

    return false;
}

bool Conf2modem::isConnectionWorks()
{
    bool r = false;

#ifdef ENABLE_EXTSUPPORT_OF_IFACES

        switch(lastConnectionType){
        case 0: r = serialPort->isOpen(); break;
        case 1: r = isTcpConnectionWorks(socket); break;
        case 2: r = svahaConnector->isOpen(); break;
        }

#else
    r = isTcpConnectionWorks(socket);
#endif
    if(!r && connectionDownCounter < 0xFFF)
        connectionDownCounter++;
    return r;

}

bool Conf2modem::isConnectionWorks(int waitMsec)
{
    if(isConnectionWorks())
        return true;
    if(waitMsec > 0)
        QThread::msleep(waitMsec);
    return false;
}


bool Conf2modem::isTcpConnectionWorks(QTcpSocket *socket)
{
    return (socket->state() == QTcpSocket::ConnectedState || socket->state() == QTcpSocket::ConnectingState);
}

bool Conf2modem::isCoordinatorGood(const bool &forced, const bool &readAboutZigBee)
{

#ifdef ENABLE_EXTSUPPORT_OF_IFACES

        isCoordinatorConfigReady = (isCoordinatorConfigReady || !forced);
#endif
    if(!forced && isCoordinatorConfigReady && qAbs(msecWhenCoordinatorWasGood - QDateTime::currentMSecsSinceEpoch()) < MAX_MSEC_FOR_COORDINATOR_READY )
        return isCoordinatorConfigReady;

    isCoordinatorConfigReady = false;
    if(enableDisableApi(true, readAboutZigBee)){
        emit currentOperation(tr("The API mode is active"));

        emit onApiModeEnabled();
        isCoordinatorConfigReady = true;
        apiErrCounter = 0;
        msecWhenCoordinatorWasGood = QDateTime::currentMSecsSinceEpoch();

    }


    if(uartBlockPrtt || directAccess)
        return isCoordinatorConfigReady;

    if(!isCoordinatorConfigReady)
        incrementApiErrCounter();

    return isCoordinatorConfigReady;

}


QByteArray Conf2modem::readDevice()
{
    return readDevice("\r\n", false);
}

QByteArray Conf2modem::readDevice(const QByteArray &endSymb, const bool &isQuickMode)
{
    QByteArray readArr;
    if(!isConnectionWorks())
        return "";


    QTime globalTime;
    globalTime.start();

    const int globalTimeMax = timeouts.global;
    const qint32 timeOut_c = timeouts.block;

    readArr = readAll();

    QTime time;
    time.start();

    const int endSymbLen = endSymb.length();

    bool readArrHasData = false;
    bool uartAccessChecked = false;

    const int defmsec = isQuickMode ? 10 : 50;
    const int defmsec2 = isQuickMode ? 50 : 1111;


    for(int counter = 0, hasDataCounter = 0; (readArr.isEmpty() || time.elapsed() < timeOut_c) && globalTime.elapsed() < globalTimeMax && counter < MAX_READ_TRYES_FROM_UART; counter++){

        if(!uartAccessChecked && canCheckUartAccess(readArr.isEmpty(), globalTime.elapsed())){
            if((checkUartAccess(readArr, globalTime.elapsed()) || directAccess || uartBlockPrtt))
                break;
            uartAccessChecked = !readArr.isEmpty();
        }

        if(readArrHasData){
            if(readArr.length() > MAX_READ_FROM_UART){
                incrementApiErrCounter();
                break;
            }

            if(readArr.length() > endSymbLen && readArr.right(endSymbLen) == endSymb)
                break;

        }
        if(waitForReadyRead( (uartBlockPrtt) ? defmsec2 : defmsec)){
            readArr.append(readAll());
            hasDataCounter++;
            time.restart();
        }
        if(!readArrHasData && hasDataCounter != 0)
            readArrHasData = !readArr.isEmpty();

//        if(isQuickMode)
//            break;

    }
    emit dataReadWriteReal(readArr, ifaceName, true);
    if(!uartAccessChecked && (directAccess || uartBlockPrtt || checkUartAccess(readArr, globalTime.elapsed()))){
        if(sayGoodByeIfUartIsNFree("readDevice ", 555, readArr.isEmpty())){
            if(activeDbgMessages)  emit appendDbgExtData(dbgExtSrcId, QString("Conf2modem readDevice endSymb=%1 exit, busy, timeg=%2, timeb=%3").arg(QString(endSymb))
                                                         .arg(globalTime.elapsed()).arg(time.elapsed()));
            return QByteArray();
        }
    }

    emit dataRead(readArr);

    if(!readArr.isEmpty())
        emit dataFromDevice();

    if(activeDbgMessages)  emit appendDbgExtData(dbgExtSrcId, QString("Conf2modem readDevice endSymb=%1 exit, readArrIsEmpty=%2, readArr=%3, timeg=%4, timeb=%5")
                                                 .arg(QString(endSymb)).arg(readArr.isEmpty()).arg(QString(readArr.toHex()) + " " + QString(readArr).simplified().trimmed())
                                                 .arg(globalTime.elapsed()).arg(time.elapsed()));




    return readArr;
}

QByteArray Conf2modem::readDeviceQuick(const QByteArray &endSymb, const bool &isclearbufmode)
{
    const DeviceTimeouts timeoutsl = this->timeouts;
    timeouts.global = isclearbufmode ? 60 : 250;
    timeouts.block = isclearbufmode ? 20 : 50;
    const QByteArray r = readDevice(endSymb, true);
    this->timeouts = timeoutsl;

    return r;

}

bool Conf2modem::waitForReadyRead(const int &msecs)
{
#ifdef ENABLE_EXTSUPPORT_OF_IFACES

        switch(lastConnectionType){
        case 0: return serialPort->waitForReadyRead(msecs);
        case 1: return socket->waitForReadyRead(msecs);
        case 2: return svahaConnector->waitForReadyRead(msecs);
        }
        return false ;


#else
    return socket->waitForReadyRead(msecs);
#endif
}



bool Conf2modem::waitForBytesWritten(const int &msecs)
{
#ifdef ENABLE_EXTSUPPORT_OF_IFACES
        switch(lastConnectionType){
        case 0: return serialPort->waitForBytesWritten(msecs);
        case 1: return socket->waitForBytesWritten(msecs);
        case 2: return svahaConnector->waitForBytesWritten(msecs);
        }
        return false ;


#else
    return socket->waitForBytesWritten(msecs);
#endif

}

QByteArray Conf2modem::readAll()//clear a read buffer
{

    if(!isConnectionWorks())
        return "";
#ifdef ENABLE_EXTSUPPORT_OF_IFACES

    emit onReadWriteOperation(true);

        switch(lastConnectionType){
        case 0: return serialPort->readAll();
        case 1: return socket->readAll();
        case 2: return svahaConnector->readAll();
        }
        return "";


#else
    return socket->readAll();
#endif
}


qint64 Conf2modem::writeATcommand(const QString &atcommand)
{
    return write2dev(QByteArray(atcommand.toUtf8()) + "\r\n");
}

qint64 Conf2modem::writeATcommand(const QString &atcommand, const bool &ignoreDaAndPrtt)
{    
    return write2dev(QByteArray(atcommand.toUtf8()) + "\r\n", ignoreDaAndPrtt);
}

qint64 Conf2modem::write2dev(const QByteArray &writeArr)
{
    return write2dev(writeArr, false);
}

qint64 Conf2modem::write2dev(const QByteArray &writeArr, const bool &ignoreDaAndPrtt)
{

    if(verboseMode)
        qDebug() << "ZbyratorObject::write2dev " << directAccess << writeArr.isEmpty() << writePreffix << uartBlockPrtt;

    if(activeDbgMessages)  emit appendDbgExtData(dbgExtSrcId, QString("Conf2modem write2dev directAccess=%1, uartBlockPrtt=%2, myPrtt=%3, writeArr=%4, lastConnectionType=%5").arg(directAccess).arg(uartBlockPrtt).arg(QString(writePreffix)).arg(QString(writeArr.toHex()) + " " + QString(writeArr).simplified().trimmed()).arg((int)lastConnectionType));

    if(!isConnectionWorks())
        return 0;

    emit dataReadWriteReal(writeArr, ifaceName, false);
    if(!ignoreDaAndPrtt && directAccess)
        return 0;

    if(writeArr.isEmpty() || lastConnectionType == 0xFF)
        return 0;


    //<prtt>;<space>;<data>
    //prtt.len = 1byte
#ifdef DISABLE_UART_PRIORITY
    const qint64 len = write(writeArr);
#else
    const qint64 len = write(writePreffix + writeArr);// QByteArray::number(lastPeredavatorPrtt) + "; ;"
#endif
    if(uartBlockPrtt)
        lastCommandWasAtcn = (len > 0 && writeArr == "ATCN\r\n");
    if(verboseMode)
        qDebug() << "ZbyratorObject::write2dev " << len << writePreffix << lastCommandWasAtcn;

    if(activeDbgMessages)  emit appendDbgExtData(dbgExtSrcId, QString("Conf2modem write2dev directAccess=%1, uartBlockPrtt=%2, myPrtt=%3, len=%4").arg(directAccess).arg(uartBlockPrtt).arg(QString(writePreffix)).arg(len));

    waitForBytesWritten(300);
    emit dataWrite(writeArr);

    return len;
}

bool Conf2modem::openTcpConnection(const QStringList &hosts, const QList<quint16> &ports)
{

    for(int i = 0, imax = hosts.size(); i < imax; i++){
        if(openTcpConnection(hosts.at(i), ports.at(i)))
            return true;
    }
    return false;
}

bool Conf2modem::openTcpConnection(const QString &host, const quint16 &port)
{
    onDeviceDestr();//reset params
    lastConnectionType = 1;
    if(verboseMode)
        qDebug() << "Conf2modem::openTcpConnection()";
    if(activeDbgMessages)  emit appendDbgExtData(dbgExtSrcId, QString("Conf2modem openTcpConnection host=%1, port=%2, myPrtt=%3").arg(host).arg(port).arg(QString(writePreffix)));


    emit openingTcpConnection();

    stopAll = false;
    socket->connectToHost(host, port);

    const bool r = socket->waitForConnected(qMax(timeouts.global, 5000));

    if(!r){
        if(activeDbgMessages)  emit appendDbgExtData(dbgExtSrcId, QString("Conf2modem openTcpConnection err=%1").arg(socket->errorString()));
        emit currentOperation(tr("Can't connect to the coordinator. %1").arg(socket->errorString()));//%1 %2, %3").arg(host).arg(port).arg(socket->errorString()));
        closeDevice();
    }else{
        connectionDownCounter = 0;
        ifaceName = host.contains(":") ? QString("[%1]:%2").arg(host).arg(port) : QString("%1:%2").arg(host).arg(port);
        emit currentOperation(tr("Connection to the coordinator was established)"));
    }

//#ifdef DISABLE_UART_PRIORITY
//   if(r){
//        writeATcommand("ATCN");
//        readDeviceQuick("\r\n");

//    }
//#endif

    return r;

}
#ifdef ENABLE_EXTSUPPORT_OF_IFACES

bool Conf2modem::openM2mConnection(const QVariantHash &oneProfile)
{
    onDeviceDestr();//reset params

    lastConnectionType = 2;
    stopAll = false;

    const int timeout = qMax(timeouts.global, 7000);
    svahaConnector->connect2hostViaSvaha(oneProfile, timeout, timeouts.block);
    const bool r = svahaConnector->waitForConnected(timeout);

    if(!r){
        if(activeDbgMessages)  emit appendDbgExtData(dbgExtSrcId, QString("Conf2modem openTcpConnection err=%1").arg(svahaConnector->errorString()));
        emit currentOperation(tr("Can't connect to the coordinator. %1").arg(svahaConnector->errorString()));//%1 %2, %3").arg(host).arg(port).arg(socket->errorString()));
        closeDevice();
    }else{
        ifaceName = svahaConnector->getLastConnDev();
        connectionDownCounter = 0;
        emit currentOperation(tr("Connection to the coordinator was established)"));
    }

//    if(r){
//         writeATcommand("ATCN");
//         readDeviceQuick("\r\n");
//     }
    return r;
}

bool Conf2modem::openSerialPort(const QString &portName, const qint32 &baudRate, const QStringList &uarts)
{
    onDeviceDestr();//reset params
 stopAll = false;
    lastConnectionType = 0;
    QString mess;
    const bool r = (uarts.isEmpty() && !portName.isEmpty()) ?
        findModemOnPort("", baudRate, portName.split("\n"), mess) : //manual mode
        findModemOnPort(portName, baudRate, uarts, mess); //detection mode


    if(!r){
        if(activeDbgMessages)  emit appendDbgExtData(dbgExtSrcId, QString("Conf2modem openTcpConnection err=%1").arg(mess));
        emit currentOperation(tr("Can't connect to the coordinator. %1").arg(mess));//%1 %2, %3").arg(host).arg(port).arg(socket->errorString()));
        closeDevice();
    }else{
        connectionDownCounter = 0;
        ifaceName = serialPort->portName();
        emit currentOperation(tr("Connection to the coordinator was established)"));

    }

    return r;


}

bool Conf2modem::findModemOnPort(QString defPortName, qint32 baudR, QStringList uarts, QString &lastError)
{
    std::sort(uarts.begin(), uarts.end());
    if(!defPortName.isEmpty() && uarts.contains(defPortName)){
        uarts.removeOne(defPortName);
        uarts.prepend(defPortName);
    }

    while(!uarts.isEmpty()){
        const QString portN = uarts.takeFirst();

        emit currentOperation(tr("Trying to open %1").arg(portN));

        serialPort->setPortName(portN);
        if(serialPort->open(QIODevice::ReadWrite)){
            if(serialPort->setBaudRate(baudR) && serialPort->setParity(QSerialPort::NoParity) && serialPort->setStopBits(QSerialPort::OneStop) &&
                    serialPort->setDataBits(QSerialPort::Data8) && serialPort->setFlowControl(QSerialPort::NoFlowControl)){
                serialPort->clear();
                emit onSerialPortOpened(portN);

                if(request2modemOn())
                    return true;

                emit stopCheckCurrPort();

            }
        }
        serialPort->close();
    }
    lastError = tr("Can't find any device");
    return false;
}

bool Conf2modem::request2modemOn()
{

    for(int i = 1; i <= 3 && !stopAll; i++){
        emit currentOperation(tr("Detecting the modem... Sending the AT command to %1.").arg(serialPort->portName()));
        writeATcommand("+++");
        const QByteArray readArr = readDeviceQuick("\r\n", false);

        if(readArr.startsWith("OKERROR") || readArr.startsWith("ERROR"))
            return true;

        if(!readArr.isEmpty() && (readArr.contains("OK") || readArr.contains("O\r\n") || readArr.contains("K\r\n")))
            i--;

    }
    return false;
}


#endif


void Conf2modem::lSleep(const int &msleep)
{
    QTime time;
    time.start();
    if(isConnectionWorks()){
        for(int i = msleep > 10 ? 10 : msleep; i < msleep && isConnectionWorks() && time.elapsed() < msleep; i += 10)
            readDevice();
    }else{
        for(int i = msleep > 10 ? 10 : msleep; i < msleep && time.elapsed() < msleep; i += 10)
            QThread::msleep(10);
    }
}

bool Conf2modem::checkUartAccess(const QByteArray &arr, const int &msecElapsed)
{    
    if(verboseMode)
        qDebug() << "checkUartAccess " << arr.isEmpty() << msecElapsed << uartBlockPrtt << lastCommandWasAtcn << directAccess << arr.toHex();
    if(uartBlockPrtt && lastCommandWasAtcn && arr.isEmpty() && msecElapsed > 200){
        directAccess = uartBlockPrtt = false;
        return false;
    }

    if(!arr.isEmpty()){

        if(arr == PORT_IS_BUSY ){
            if(!directAccess && msecElapsed <= MAX_MSEC_TIME2OPEN_DA){
                directAccess = true;
                uartBlockPrtt = false;
                //            emit onDaStateChanged(true);
                isCoordinatorConfigReady = false;
            }

        }else{
            if(arr == PORT_IS_FREE || arr.endsWith(PORT_IS_FREE)){
                if(directAccess){
                    directAccess = uartBlockPrtt = false;
                    //                emit onDaStateChanged(false);

                }
            }else{
                if(!writePreffix.isEmpty() && arr == PORT_IS_BUSY_LOCAL){
                    uartBlockPrtt = true;
//                    emit onLowPriority2uart();

                }else{
                    if(msecElapsed > 0)
                        directAccess = uartBlockPrtt = false;
                }
            }
        }

    }
    if(verboseMode)
        qDebug() << "arr=" << arr << directAccess << uartBlockPrtt;
    return isUartBusy();
}

bool Conf2modem::canCheckUartAccess(const bool &arrIsEmpty, const int &msecElapsed)
{
    return (!arrIsEmpty || (arrIsEmpty && msecElapsed > 20 && lastCommandWasAtcn));
}

bool Conf2modem::isCoordinatorFree()
{
    const bool da = directAccess;
    const bool busy = uartBlockPrtt;

    QTime time; time.start();

    if(activeDbgMessages)  emit appendDbgExtData(dbgExtSrcId, QString("Conf2modem isCoordinatorFree da directAccess=%1, uartBlockPrtt=%2, myPrtt=%3, time=%4")
                          .arg(directAccess).arg(uartBlockPrtt).arg(QString(writePreffix)).arg(time.elapsed()));

    const bool nfree = directAccess || uartBlockPrtt;
    if(directAccess){
        if(activeDbgMessages)  emit appendDbgExtData(dbgExtSrcId, QString("Conf2modem ondirectaccess directAccess=%1, uartBlockPrtt=%2, myPrtt=%3, da=%4")
                              .arg(directAccess).arg(uartBlockPrtt).arg(QString(writePreffix)).arg(da));
        for(int i = 0; i < 3; i++){
            readDevice();
            if(!directAccess && !uartBlockPrtt)
                return true;
            if(uartBlockPrtt)
                break;
        }

    }
//    directAccess = uartBlockPrtt = false;//reset

    writeATcommand("ATCN", true);//у відповідь може бути нічого, тому важливо почати обмін з мінімальною затримкою
    if(readDeviceQuick("\r\n", false).contains("ERR") || uartBlockPrtt){
        writeATcommand("ATCN", true);
        readDeviceQuick("\r\n", false);//в цій функції уже є перевірка прямого доступу

    }

    if(da != directAccess){
        if(activeDbgMessages)  emit appendDbgExtData(dbgExtSrcId, QString("Conf2modem isCoordinatorFree da directAccess=%1, uartBlockPrtt=%2, myPrtt=%3, da=%4")
                              .arg(directAccess).arg(uartBlockPrtt).arg(QString(writePreffix)).arg(da));
//        emit onDaStateChanged(directAccess);
    }

    if(busy != uartBlockPrtt){

        emit onBusyByPriority(uartBlockPrtt);
    }

    const bool nfree2 = (directAccess || uartBlockPrtt);
    if(nfree == nfree2){
        readDeviceQuick("\r\n", true);

    }

    const bool nfree3 = (directAccess || uartBlockPrtt);
    if(nfree3 != nfree){
        emit onCoordinatorIsBusy(nfree3);

    }


    return !nfree3;
}

bool Conf2modem::isCoordinatorFreeWithRead()
{
    QTime time; time.start();
    if(isCoordinatorFree()){

        return true;
    }

    if(isDirectAccess())
        readDevice();
    else
        readDeviceQuick("\r\n", false);

    const bool r = !isUartBusy();


    return r;
}

bool Conf2modem::sayGoodByeIfUartIsNFree(const QString &mess, const int &msec4blockUart)
{
    return sayGoodByeIfUartIsNFree(mess, msec4blockUart, false);
}

bool Conf2modem::sayGoodByeIfUartIsNFree(const QString &mess, const int &msec4blockUart, const bool &arrIsEmpty)
{
    if(directAccess || uartBlockPrtt){
        if(verboseMode)
            qDebug() << mess << directAccess << uartBlockPrtt;

        if(!directAccess && uartBlockPrtt && arrIsEmpty && lastCommandWasAtcn)
            uartBlockPrtt = false;
        return false;

        if(activeDbgMessages)  emit appendDbgExtData(dbgExtSrcId, mess +  QString("directAccess=%1, uartBlockPrtt=%2, myPrtt=%3, sayGoodByeIfUartIsNFree, msec4blockUart=%4").arg(directAccess).arg(uartBlockPrtt).arg(QString(writePreffix)).arg(msec4blockUart));
        if(msec4blockUart > 0)
            QThread::msleep(msec4blockUart);
        return true;
    }
    return false;
}



void Conf2modem::setWritePreffix(QByteArray preffix)
{

    if(writePreffix != preffix)
        if(activeDbgMessages)  emit appendDbgExtData(dbgExtSrcId, QString("Conf2modem setWritePreffix directAccess=%1, uartBlockPrtt=%2, myPrtt=%3, preffix=%4")
                              .arg(directAccess).arg(uartBlockPrtt).arg(QString(writePreffix)).arg(QString(preffix)));

#ifdef DISABLE_UART_PRIORITY
    writePreffix.clear();
#else

    writePreffix = preffix;
#endif

}

void Conf2modem::setWritePriority(const quint8 &prtt)
{
    writePrtt = prtt;
    setWritePreffix(QByteArray::number(prtt) + "; ;");
}

void Conf2modem::stopAllSlot()
{
    stopAll = true;
}

void Conf2modem::resetStopAllConfModem()
{
    stopAll = false;
}

void Conf2modem::setTimeouts(int global, int block)
{
    timeouts.global = global;
    timeouts.block = block;
}

void Conf2modem::incrementApiErrCounter()
{
    apiErrCounter++;

    if(lastConnectionType != 0xFF && apiErrCounter > MAX_TRIES_FOR_CONFIG){
        uartBlockPrtt = directAccess = false;
        close();
    }

    emit currentOperation(tr("Can't activate the API mode, counter: %1").arg(apiErrCounter));

    if(apiErrCounter > MAX_TRIES_FOR_HARD_RESET){
        if(activeDbgMessages)  emit appendDbgExtData(dbgExtSrcId, QString("Conf2modem incrementApiErrCounter directAccess=%1, uartBlockPrtt=%2, myPrtt=%3, apiErrCounter=%4")
                              .arg(directAccess).arg(uartBlockPrtt).arg(QString(writePreffix)).arg(apiErrCounter));

        apiErrCounter = 0;//обнуляю щоб не дуже часто скидало координатора
        emit resetCoordinatorRequest();
    }else if(apiErrCounter > MAX_TRIES_FOR_CONFIG){
        if(activeDbgMessages)  emit appendDbgExtData(dbgExtSrcId, QString("Conf2modem incrementApiErrCounter directAccess=%1, uartBlockPrtt=%2, myPrtt=%3, apiErrCounter=%4")
                              .arg(directAccess).arg(uartBlockPrtt).arg(QString(writePreffix)).arg(apiErrCounter));
        emit killPeredavatorRequest();
    }
}

void Conf2modem::closeDevice()
{    
    if(isConnectionWorks()){
        close();
        uartBlockPrtt = directAccess = false;
        emit currentOperation(tr("Connection to the coordinator has been closed"));

        emit onConnectionDown();
    }
}

void Conf2modem::resetBlockByPrtt()
{
    uartBlockPrtt = false;
}

void Conf2modem::resetDaState()
{
    directAccess = false;
}

#ifdef ENABLE_EXTSUPPORT_OF_IFACES

void Conf2modem::closeSerialPort()
{
    serialPort->close();
    emit onConnectionClosed();
}


#endif

void Conf2modem::onDeviceDestr()
{
    close();
    if(activeDbgMessages)  emit appendDbgExtData(dbgExtSrcId, QString("Conf2modem onDeviceDestr directAccess=%1, uartBlockPrtt=%2, myPrtt=%3, apiErrCounter=%4")
                          .arg(directAccess).arg(uartBlockPrtt).arg(QString(writePreffix)).arg(apiErrCounter));
    modemIsOverRS485 = isCoordinatorConfigReady = false;
#ifdef ENABLE_EXTSUPPORT_OF_IFACES
    lastConnectionType = 0xFF;
#else
    lastConnectionType = 1;//tcp client
#endif
    directAccess = false;
    uartBlockPrtt = false;
    lastCommandWasAtcn = false;
    stopAllSlot();
}

qint64 Conf2modem::write(const QByteArray &arr)
{
#ifdef ENABLE_EXTSUPPORT_OF_IFACES
    emit onReadWriteOperation(false);

        switch(lastConnectionType){
        case 0: return serialPort->write(arr);
        case 1: return socket->write(arr);
        case 2: return svahaConnector->write(arr);
        }
        return -1;


#else
    return socket->write(arr);
#endif

}

void Conf2modem::close()
{
#ifdef ENABLE_EXTSUPPORT_OF_IFACES

        switch(lastConnectionType){
        case 0: return serialPort->close();
        case 1: return socket->close();
        case 2: return svahaConnector->close();
        }
        return;

#else
    return socket->close();
#endif

}
