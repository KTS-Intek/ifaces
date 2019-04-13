#include "conn2modem.h"
#include "embeelimits.h"
#include "matildalimits.h"
#include <QThread>
#include <QTime>

//-------------------------------------------------------------------------------------

Conn2modem::Conn2modem(const int &dbgExtSrcId, const bool &verboseMode, QObject *parent) : QObject(parent), dbgExtSrcId(dbgExtSrcId), verboseMode(verboseMode)
{
    activeDbgMessages = (dbgExtSrcId > 0);
    createDevices();
    onDeviceDestr();

    writePrtt = 0;
    apiErrCounter = 0;
    msecWhenCoordinatorWasGood = 0;

}


//-------------------------------------------------------------------------------------

quint16 Conn2modem::getConnectionDownCounter() const
{
    return connectionDownCounter;

}

//-------------------------------------------------------------------------------------

quint8 Conn2modem::getWritePrtt() const
{
    return writePrtt;
}

//-------------------------------------------------------------------------------------

QString Conn2modem::getLastIfaceName() const
{
    return ifaceName;
}

//-------------------------------------------------------------------------------------

bool Conn2modem::isDirectAccess() const
{
    return directAccess;
}

//-------------------------------------------------------------------------------------

bool Conn2modem::isBlockByPrtt() const
{
    return uartBlockPrtt;
}

//-------------------------------------------------------------------------------------

bool Conn2modem::isUartBusy() const
{
    return (directAccess || uartBlockPrtt);
}


//-------------------------------------------------------------------------------------


bool Conn2modem::isConnectionWorks()
{
    bool r = false;

#ifdef ENABLE_EXTSUPPORT_OF_IFACES

    switch(lastConnectionType){

#ifndef DISABLE_SERIALPORT_MODE
    case IFACECONNTYPE_UART     : if(need2closeSerial) closeSerialPort(); r = serialPort->isOpen(); break;
#endif
#ifndef DISABLE_TCPCLIENT_MODE
    case IFACECONNTYPE_TCPCLNT  : r = isTcpConnectionWorks(socket); break;
#endif
#ifndef DISABLE_M2M_MODULE
    case IFACECONNTYPE_M2MCLNT  : r = svahaConnector->isOpen(); break;
#endif

    }

#else
    r = isTcpConnectionWorks(socket);
#endif
    if(!r && connectionDownCounter < 0xFFF)
        connectionDownCounter++;
    return r;
}

//-------------------------------------------------------------------------------------

bool Conn2modem::isConnectionWorks(int waitMsec)
{
    if(isConnectionWorks())
        return true;
    if(waitMsec > 0)
        QThread::msleep(waitMsec);
    return false;
}

//-------------------------------------------------------------------------------------

bool Conn2modem::isTcpConnectionWorks(QTcpSocket *socket)
{
    return (socket->state() == QTcpSocket::ConnectedState || socket->state() == QTcpSocket::ConnectingState);

}

//-------------------------------------------------------------------------------------

QByteArray Conn2modem::readDevice()
{
    return readDevice("\r\n", false);
}

//-------------------------------------------------------------------------------------

QByteArray Conn2modem::readDevice(const QByteArray &endSymb, const bool &isQuickMode)
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


//    qDebug() << "readDevice " << time.elapsed() << timeouts.block << globalTime.elapsed() << timeouts.global << (readArr.isEmpty() || time.elapsed() < timeOut_c) << (globalTime.elapsed() < globalTimeMax) ;

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

//-------------------------------------------------------------------------------------

QByteArray Conn2modem::readDeviceQuick(const QByteArray &endSymb, const bool &isclearbufmode)
{
    const DeviceTimeouts timeoutsl = this->timeouts;

    timeouts.global = isclearbufmode ? 60 : 250;
    timeouts.block = isclearbufmode ? 20 : 50;

#ifdef DISABLE_QUICK_READMODE
     if(!isclearbufmode){
         timeouts.global = qMin(11000, timeoutsl.global);
         timeouts.block = qMin(500, timeoutsl.block);
     }
#endif

#ifdef ENABLE_BIGGER_BLOCKTIMEOUT
    if(!isclearbufmode){
        timeouts.global = ENABLE_BIGGER_BLOCKTIMEOUT * 5;
        timeouts.block = ENABLE_BIGGER_BLOCKTIMEOUT;
    }
#endif

    const QByteArray r = readDevice(endSymb, true);
    this->timeouts = timeoutsl;

    return r;
}

//-------------------------------------------------------------------------------------

bool Conn2modem::waitForReadyRead(const int &msecs)
{
#ifdef ENABLE_EXTSUPPORT_OF_IFACES

        switch(lastConnectionType){
#ifndef DISABLE_SERIALPORT_MODE
        case IFACECONNTYPE_UART     : return serialPort->waitForReadyRead(msecs);
#endif
#ifndef DISABLE_TCPCLIENT_MODE
        case IFACECONNTYPE_TCPCLNT  : return socket->waitForReadyRead(msecs);
#endif
#ifndef DISABLE_M2M_MODULE
        case IFACECONNTYPE_M2MCLNT  : return svahaConnector->waitForReadyRead(msecs);
#endif
        }
        return false ;


#else
    return socket->waitForReadyRead(msecs);
#endif
}

//-------------------------------------------------------------------------------------

bool Conn2modem::waitForBytesWritten(const int &msecs)
{
#ifdef ENABLE_EXTSUPPORT_OF_IFACES
        switch(lastConnectionType){
#ifndef DISABLE_SERIALPORT_MODE
        case IFACECONNTYPE_UART     : return serialPort->waitForBytesWritten(msecs);
#endif
#ifndef DISABLE_TCPCLIENT_MODE
        case IFACECONNTYPE_TCPCLNT  : return socket->waitForBytesWritten(msecs);
#endif
#ifndef DISABLE_M2M_MODULE
        case IFACECONNTYPE_M2MCLNT  : return svahaConnector->waitForBytesWritten(msecs);
#endif
        }
        return false ;


#else
    return socket->waitForBytesWritten(msecs);
#endif
}

//-------------------------------------------------------------------------------------

QByteArray Conn2modem::readAll()
{
    if(!isConnectionWorks())
        return "";
#ifdef ENABLE_EXTSUPPORT_OF_IFACES

    emit onReadWriteOperation(true);

    switch(lastConnectionType){
#ifndef DISABLE_SERIALPORT_MODE
    case IFACECONNTYPE_UART     : return serialPort->readAll();
#endif
#ifndef DISABLE_TCPCLIENT_MODE
    case IFACECONNTYPE_TCPCLNT  : return socket->readAll();
#endif
#ifndef DISABLE_M2M_MODULE
    case IFACECONNTYPE_M2MCLNT  : return svahaConnector->readAll();
#endif
    }
    return "";


#else
    return socket->readAll();
#endif
}

//-------------------------------------------------------------------------------------

qint64 Conn2modem::writeATcommand(const QString &atcommand)
{
    return writeATcommand(atcommand, false);
}

//-------------------------------------------------------------------------------------

qint64 Conn2modem::writeATcommand(const QString &atcommand, const bool &ignoreDaAndPrtt)
{
    const QByteArray writearr = QByteArray(atcommand.toUtf8()) + "\r\n";
    if(writearr.length() > 14){
        for(int i = 0, imax = writearr.length(), j = 0; i < imax && j < 10; j++, i += 12){
            if(write2dev(writearr.mid(i, 12), ignoreDaAndPrtt) < 1)
                return false;
            if((i+12) < imax)
                QThread::msleep(50);
        }
        return true;
    }

    return write2dev(writearr, ignoreDaAndPrtt);

}

//-------------------------------------------------------------------------------------

qint64 Conn2modem::write2dev(const QByteArray &writeArr)
{
    return write2dev(writeArr, false);
}

//-------------------------------------------------------------------------------------

qint64 Conn2modem::write2dev(const QByteArray &writeArr, const bool &ignoreDaAndPrtt)
{
    if(verboseMode)
        qDebug() << "ZbyratorObject::write2dev " << directAccess << writeArr.isEmpty() << writePreffix << uartBlockPrtt;

    if(activeDbgMessages)  emit appendDbgExtData(dbgExtSrcId, QString("Conf2modem write2dev directAccess=%1, uartBlockPrtt=%2, myPrtt=%3, writeArr=%4, lastConnectionType=%5").arg(directAccess).arg(uartBlockPrtt).arg(QString(writePreffix)).arg(QString(writeArr.toHex()) + " " + QString(writeArr).simplified().trimmed()).arg((int)lastConnectionType));

    if(!isConnectionWorks())
        return 0;

    emit dataReadWriteReal(writeArr, ifaceName, false);
    if(!ignoreDaAndPrtt && directAccess)
        return 0;

    if(writeArr.isEmpty() || lastConnectionType == IFACECONNTYPE_UNKN)
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
        qDebug() << "ZbyratorObject::write2dev len=" << len << writePreffix << lastCommandWasAtcn;

    if(activeDbgMessages)  emit appendDbgExtData(dbgExtSrcId, QString("Conf2modem write2dev directAccess=%1, uartBlockPrtt=%2, myPrtt=%3, len=%4").arg(directAccess).arg(uartBlockPrtt).arg(QString(writePreffix)).arg(len));

    waitForBytesWritten(300);
    emit dataWrite(writeArr);

    return len;
}

//-------------------------------------------------------------------------------------

bool Conn2modem::openTcpConnection(const QStringList &hosts, const QList<quint16> &ports)
{
    for(int i = 0, imax = hosts.size(); i < imax; i++){
        if(openTcpConnection(hosts.at(i), ports.at(i)))
            return true;
    }
    return false;
}

//-------------------------------------------------------------------------------------

bool Conn2modem::openTcpConnection(const QString &host, const quint16 &port)
{
    onDeviceDestr();//reset params
    lastConnectionType = IFACECONNTYPE_TCPCLNT;
    if(verboseMode)
        qDebug() << "Conf2modem::openTcpConnection()";
    if(activeDbgMessages)  emit appendDbgExtData(dbgExtSrcId, QString("Conf2modem openTcpConnection host=%1, port=%2, myPrtt=%3").arg(host).arg(port).arg(QString(writePreffix)));


    emit openingTcpConnection();

    stopAll = false;
    socket->connectToHost(host, port);

    const bool r = socket->waitForConnected(qMax(timeouts.global, 5000));

    if(!r){
        if(activeDbgMessages)  emit appendDbgExtData(dbgExtSrcId, QString("Conf2modem openTcpConnection err=%1").arg(socket->errorString()));
        emit currentOperation(tr("Can't connect to the remote device. %1").arg(socket->errorString()));//%1 %2, %3").arg(host).arg(port).arg(socket->errorString()));
        closeDevice();
    }else{
        connectionDownCounter = 0;
        ifaceName = host.contains(":") ? QString("[%1]:%2").arg(host).arg(port) : QString("%1:%2").arg(host).arg(port);
        emit currentOperation(tr("Connection to the remote device was established)"));
    }

//#ifdef DISABLE_UART_PRIORITY
//   if(r){
//        writeATcommand("ATCN");
//        readDeviceQuick("\r\n");

//    }
//#endif

    return r;
}

//-------------------------------------------------------------------------------------

#ifdef ENABLE_EXTSUPPORT_OF_IFACES

#ifndef DISABLE_M2M_MODULE
bool Conn2modem::openM2mConnection(const QVariantHash &oneProfile)
{
    onDeviceDestr();//reset params

    lastConnectionType = IFACECONNTYPE_M2MCLNT;
    stopAll = false;

    const int timeout = qMax(timeouts.global, 7000);
    svahaConnector->connect2hostViaSvaha(oneProfile, timeout, timeouts.block);
    const bool r = svahaConnector->waitForConnected(timeout);

    if(!r){
        if(activeDbgMessages)  emit appendDbgExtData(dbgExtSrcId, QString("Conf2modem openTcpConnection err=%1").arg(svahaConnector->errorString()));
        emit currentOperation(tr("Can't connect to the remote device. %1").arg(svahaConnector->errorString()));//%1 %2, %3").arg(host).arg(port).arg(socket->errorString()));
        closeDevice();
    }else{
        ifaceName = svahaConnector->getLastConnDev();
        connectionDownCounter = 0;
        emit currentOperation(tr("Connection to the remote device was established)"));
    }

//    if(r){
//         writeATcommand("ATCN");
//         readDeviceQuick("\r\n");
//     }
    return r;
}

#endif

//-------------------------------------------------------------------------------------

#ifndef DISABLE_SERIALPORT_MODE
bool Conn2modem::openSerialPort(const QString &portName, const qint32 &baudRate, const QStringList &uarts)
{
    onDeviceDestr();//reset params
 stopAll = false;
    lastConnectionType = IFACECONNTYPE_UART;
    QString mess;
    const bool r = (uarts.isEmpty() && !portName.isEmpty()) ?
        findModemOnPort("", baudRate, portName.split("\n"), mess) : //manual mode
        findModemOnPort(portName, baudRate, uarts, mess); //detection mode


    if(!r){
        if(activeDbgMessages)  emit appendDbgExtData(dbgExtSrcId, QString("Conf2modem openTcpConnection err=%1").arg(mess));
        emit currentOperation(tr("Can't connect to the remote device. %1").arg(mess));//%1 %2, %3").arg(host).arg(port).arg(socket->errorString()));
        closeDevice();
    }else{
        connectionDownCounter = 0;
        ifaceName = serialPort->portName();
        emit currentOperation(tr("Connection to the remote device was established)"));

    }

    return r;
}

//-------------------------------------------------------------------------------------

bool Conn2modem::findModemOnPort(QString defPortName, qint32 baudR, QStringList uarts, QString &lastError)
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
                need2closeSerial = false;
                emit onSerialPortOpened(portN);


                if(ignoreUartsChecks){
                    emit currentOperation(tr("UART checks are disabled"));
                    return true;
                }

                 const bool r = request2modemOn();

                if(r)
                    return true;

                emit stopCheckCurrPort();

            }
        }
        serialPort->close();
    }
    lastError = tr("Can't find any device");
    return false;
}

//-------------------------------------------------------------------------------------

bool Conn2modem::request2modemOn()
{

    for(int i = 1, j = 0; i <= 3 && !stopAll && j < 30; i++, j++){
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

#endif

//-------------------------------------------------------------------------------------

void Conn2modem::lSleep(const int &msleep)
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

//-------------------------------------------------------------------------------------

bool Conn2modem::checkUartAccess(const QByteArray &arr, const int &msecElapsed)
{
    if(lastConnectionType == IFACECONNTYPE_UART)
        return false;//It is a direct connection

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

//-------------------------------------------------------------------------------------

bool Conn2modem::canCheckUartAccess(const bool &arrIsEmpty, const int &msecElapsed)
{
    return (!arrIsEmpty || (arrIsEmpty && msecElapsed > 20 && lastCommandWasAtcn));

}
//-------------------------------------------------------------------------------------


bool Conn2modem::isCoordinatorFree()
{
#ifdef ENABLE_EXTSUPPORT_OF_IFACES

    if(lastConnectionType == IFACECONNTYPE_UART){
        directAccess = false;
        uartBlockPrtt = false;
//        return true;


    }

    if(ignoreUartsChecks)
        return true;
#endif

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

//-------------------------------------------------------------------------------------

bool Conn2modem::isCoordinatorFreeWithRead()
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

//-------------------------------------------------------------------------------------

bool Conn2modem::sayGoodByeIfUartIsNFree(const QString &mess, const int &msec4blockUart)
{
    return sayGoodByeIfUartIsNFree(mess, msec4blockUart, false);

}

//-------------------------------------------------------------------------------------

bool Conn2modem::sayGoodByeIfUartIsNFree(const QString &mess, const int &msec4blockUart, const bool &arrIsEmpty)
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

//-------------------------------------------------------------------------------------

void Conn2modem::setWritePreffix(QByteArray preffix)
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

//-------------------------------------------------------------------------------------

void Conn2modem::setWritePriority(const quint8 &prtt)
{
    writePrtt = prtt;
    setWritePreffix(QByteArray::number(prtt) + "; ;");
}

//-------------------------------------------------------------------------------------

void Conn2modem::stopAllSlot()
{
    stopAll = true;
}

//-------------------------------------------------------------------------------------

void Conn2modem::resetStopAllConfModem()
{//setNewTask4aDev
    stopAll = false;
}

//-------------------------------------------------------------------------------------

void Conn2modem::setTimeouts(int global, int block)
{
    timeouts.global = global;
    timeouts.block = block;
}

//-------------------------------------------------------------------------------------

void Conn2modem::incrementApiErrCounter()
{
    apiErrCounter++;

    if(lastConnectionType != IFACECONNTYPE_UNKN && apiErrCounter > MAX_TRIES_FOR_CONFIG){
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

//-------------------------------------------------------------------------------------

void Conn2modem::closeDevice()
{
    if(isConnectionWorks()){
        close();
        uartBlockPrtt = directAccess = false;
        emit currentOperation(tr("Connection to the remote device has been closed"));

        emit onConnectionDown();
    }
}

//-------------------------------------------------------------------------------------

void Conn2modem::resetBlockByPrtt()
{
    uartBlockPrtt = false;
}

//-------------------------------------------------------------------------------------

void Conn2modem::resetDaState()
{
    directAccess = false;
}

//-------------------------------------------------------------------------------------

#ifdef ENABLE_EXTSUPPORT_OF_IFACES

void Conn2modem::setIgnoreUartChecks(bool ignore)
{
    ignoreUartsChecks = ignore;
}


#ifndef DISABLE_SERIALPORT_MODE

void Conn2modem::closeSerialPort()
{
    serialPort->close();    
    emit onConnectionClosed();
}

void Conn2modem::closeSerialPortDirect()
{
    need2closeSerial = true;
    emit need2closeSerialPort();
}
#endif
#endif

//-------------------------------------------------------------------------------------

void Conn2modem::createDevices()
{
    isCoordinatorConfigReady = false;
    socket = new QTcpSocket(this);
    connect(socket, SIGNAL(disconnected()), this, SIGNAL(onConnectionClosed()) );

#ifdef ENABLE_EXTSUPPORT_OF_IFACES

    setIgnoreUartChecks(false);
    svahaConnector = new SvahaServiceConnector(this);
    connect(svahaConnector, SIGNAL(disconnected()), this, SIGNAL(onConnectionClosed()) );

    serialPort = new QSerialPort(this);
    CheckCurrPort *checkPort = new CheckCurrPort(this);

    connect(checkPort, SIGNAL(portDisconnected(bool)), this, SLOT(closeSerialPortDirect()), Qt::DirectConnection);
    connect(checkPort,SIGNAL(terminateNow()), this, SLOT(closeSerialPortDirect()), Qt::DirectConnection);

    connect(this, SIGNAL(onConnectionClosed()), checkPort, SLOT(terminate()) );
    connect(this, SIGNAL(stopCheckCurrPort()), checkPort, SLOT(terminate()) );
    connect(this, SIGNAL(onConnectionDown()), checkPort, SLOT(terminate()) );
    connect(this, SIGNAL(onSerialPortOpened(QString)), checkPort, SLOT(zapuskalka(QString)) );

#endif
}

//-------------------------------------------------------------------------------------

void Conn2modem::onDeviceDestr()
{
    close();
    if(activeDbgMessages)  emit appendDbgExtData(dbgExtSrcId, QString("Conf2modem onDeviceDestr directAccess=%1, uartBlockPrtt=%2, myPrtt=%3, apiErrCounter=%4")
                          .arg(directAccess).arg(uartBlockPrtt).arg(QString(writePreffix)).arg(apiErrCounter));
    modemIsOverRS485 = false;


    if(qAbs(msecWhenCoordinatorWasGood - QDateTime::currentMSecsSinceEpoch()) > MAX_MSEC_FOR_COORDINATOR_READY)
        isCoordinatorConfigReady = false;

#ifdef ENABLE_EXTSUPPORT_OF_IFACES


    lastConnectionType = 0xFF;
#ifndef DISABLE_M2M_MODULE

#else

#ifndef DISABLE_SERIALPORT_MODE
    lastConnectionType = IFACECONNTYPE_UART;
#else
    lastConnectionType = 0xFF;
#endif

#endif



#else
    lastConnectionType = IFACECONNTYPE_TCPCLNT;//tcp client
#endif
    directAccess = false;
    uartBlockPrtt = false;
    lastCommandWasAtcn = false;
    stopAllSlot();
}

//-------------------------------------------------------------------------------------

qint64 Conn2modem::write(const QByteArray &arr)
{
#ifdef ENABLE_EXTSUPPORT_OF_IFACES
    emit onReadWriteOperation(false);

        switch(lastConnectionType){
#ifndef DISABLE_SERIALPORT_MODE
        case IFACECONNTYPE_UART     : return serialPort->write(arr);
#endif
#ifndef DISABLE_TCPCLIENT_MODE
        case IFACECONNTYPE_TCPCLNT  : return socket->write(arr);
#endif
#ifndef DISABLE_M2M_MODULE
        case IFACECONNTYPE_M2MCLNT  : return svahaConnector->write(arr);
#endif
        }
        return -1;


#else
    return socket->write(arr);
#endif
}

//-------------------------------------------------------------------------------------

void Conn2modem::close()
{
#ifdef ENABLE_EXTSUPPORT_OF_IFACES

        switch(lastConnectionType){
#ifndef DISABLE_SERIALPORT_MODE
        case IFACECONNTYPE_UART     : return serialPort->close();
#endif
#ifndef DISABLE_TCPCLIENT_MODE
        case IFACECONNTYPE_TCPCLNT  : return socket->close();
#endif
#ifndef DISABLE_M2M_MODULE
        case IFACECONNTYPE_M2MCLNT  : return svahaConnector->close();
#endif
        }
        return;

#else
    return socket->close();
#endif
}

//-------------------------------------------------------------------------------------
