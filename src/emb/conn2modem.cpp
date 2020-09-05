#include "conn2modem.h"
#include "embeelimits.h"
#include "matildalimits.h"
#include <QThread>
#include <QTime>
#include <QDebug>




//-------------------------------------------------------------------------------------

Conn2modem::Conn2modem(const int &dbgExtSrcId, const bool &verboseMode, QObject *parent) : QObject(parent), dbgExtSrcId(dbgExtSrcId), verboseMode(verboseMode)
{
    activeDbgMessages = (dbgExtSrcId > 0);
    createDevices();
    onDeviceDestr();

    setMaxDataFromModem(0);//use defined value
    writePrtt = 0;
    lModemState.apiErrCounter = 0;

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
    return lModemState.directAccess;
}

//-------------------------------------------------------------------------------------

bool Conn2modem::isBlockByPrtt() const
{
    return
        #ifdef HASGUI4USR
            false
        #else
            (lModemState.isMainConnectionUsed ? lModemState.uartBlockPrtt : false)
        #endif

            ;
}

//-------------------------------------------------------------------------------------

bool Conn2modem::isUartBusy() const
{
    return (lModemState.directAccess

        #ifndef HASGUI4USR
            || (lModemState.isMainConnectionUsed ? lModemState.uartBlockPrtt : false)
        #endif
            );
}
//-------------------------------------------------------------------------------------
int Conn2modem::getMaxDataFromModem()
{
    return (maxDataFromModem < 1 || maxDataFromModem > 5000) ? MAX_READ_FROM_UART : maxDataFromModem;
}
//-------------------------------------------------------------------------------------
void Conn2modem::setMaxDataFromModem(const int &bytes)
{
    maxDataFromModem = bytes;
}


//-------------------------------------------------------------------------------------


bool Conn2modem::isConnectionWorking()
{
    bool r = false;

#ifdef ENABLE_EXTSUPPORT_OF_IFACES

    switch(lastConnectionType){

#ifndef DISABLE_SERIALPORT_MODE
    case IFACECONNTYPE_UART     : if(need2closeSerial){ emit currentOperation("need2closeSerial isConnectionWorking "); closeSerialPort();} r = serialPort->isOpen(); break;
#endif
#ifndef DISABLE_TCPCLIENT_MODE
    case IFACECONNTYPE_TCPCLNT  : r = isTcpConnectionWorks(socket)  ; break;
#endif
#ifndef DISABLE_M2M_MODULE
    case IFACECONNTYPE_M2MCLNT  : r = svahaConnector->isOpen()      ; break;
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

bool Conn2modem::isConnectionWorking(int waitMsec)
{
    if(isConnectionWorking())
        return true;
    if(waitMsec > 0)
        QThread::msleep(waitMsec);
    return false;
}

//-------------------------------------------------------------------------------------
#ifndef DISABLE_TCPCLIENT_MODE

bool Conn2modem::isTcpConnectionWorks(QTcpSocket *socket)
{
    return (socket->state() == QTcpSocket::ConnectedState || socket->state() == QTcpSocket::ConnectingState);

}
#endif

//-------------------------------------------------------------------------------------

QByteArray Conn2modem::readDevice()
{
    return readDevice("\r\n", false);
}

//-------------------------------------------------------------------------------------

QByteArray Conn2modem::readDevice(const QByteArray &endSymb, const bool &isQuickMode)
{
    QByteArray readArr;
    if(!isConnectionWorking())
        return "";


    QTime globalTime;
    globalTime.start();

    const int timeoutgeneral = timeouts.global;
    const qint32 timeoutblock = timeouts.block;

    readArr = readAll();

    QTime time;
    time.start();

    const int endSymbLen = endSymb.length();

    bool readArrHasData = false;
    bool uartAccessChecked = false;

    const int defmsec = isQuickMode ? 10 : 50;
    const int defmsec2 = isQuickMode ? 50 : 1111;


    for(int counter = 0, hasDataCounter = 0; (readArr.isEmpty() || time.elapsed() < timeoutblock) && globalTime.elapsed() < timeoutgeneral && counter < MAX_READ_TRYES_FROM_UART; counter++){

        if(!uartAccessChecked && canCheckUartAccess(readArr.isEmpty(), globalTime.elapsed())){
            if((checkUartAccess(readArr, globalTime.elapsed()) || lModemState.directAccess || lModemState.uartBlockPrtt))
                break;
            uartAccessChecked = !readArr.isEmpty();
        }

        if(readArrHasData){
            const int len = readArr.length();
            if(len > getMaxDataFromModem()){
                emit bigReadData(len);
                incrementApiErrCounter();
                break;
            }

            if(len > endSymbLen && readArr.right(endSymbLen) == endSymb)
                break;

        }
        if(waitForReadyRead( (lModemState.uartBlockPrtt) ? defmsec2 : defmsec)){
            readArr.append(readAll());
            hasDataCounter++;

#ifdef Q_OS_LINUX
            //            if(verboseMode)
            //        qDebug() << "readDevice wait4readyread " << time.elapsed() << timeouts.block << globalTime.elapsed() << timeouts.global << (readArr.isEmpty() || time.elapsed() < timeoutblock)
            //                 << (globalTime.elapsed() < timeoutgeneral) ;
#endif
            time.restart();

        }
        if(!readArrHasData && hasDataCounter != 0)
            readArrHasData = !readArr.isEmpty();


        //        if(isQuickMode)
        //            break;

    }

#ifdef Q_OS_LINUX
    if(verboseMode)
        qDebug() << "readDevice " << time.elapsed() << timeouts.block << globalTime.elapsed() << timeouts.global << (readArr.isEmpty() || time.elapsed() < timeoutblock)
                 << (globalTime.elapsed() < timeoutgeneral) ;
#endif
    emit dataReadWriteReal(readArr, ifaceName, true);
    if(!uartAccessChecked && (lModemState.directAccess || lModemState.uartBlockPrtt || checkUartAccess(readArr, globalTime.elapsed()))){
        if(sayGoodByeIfUartIsNFree("readDevice ", 555, readArr.isEmpty())){
#ifdef ENABLE_VERBOSE_SERVER
            if(activeDbgMessages)  emit appendDbgExtData(dbgExtSrcId, QString("Conf2modem readDevice endSymb=%1 exit, busy, timeg=%2, timeb=%3").arg(QString(endSymb))
                                                         .arg(globalTime.elapsed()).arg(time.elapsed()));
#endif
            return QByteArray();
        }
    }

    emit dataRead(readArr);//do not move it, it must be here, even if readArr is empty

    if(!readArr.isEmpty())
        emit dataFromDevice();
#ifdef ENABLE_VERBOSE_SERVER
    if(activeDbgMessages)  emit appendDbgExtData(dbgExtSrcId, QString("Conf2modem readDevice endSymb=%1 exit, readArrIsEmpty=%2, readArr=%3, timeg=%4, timeb=%5")
                                                 .arg(QString(endSymb)).arg(readArr.isEmpty()).arg(QString(readArr.toHex()) + " " + QString(readArr).simplified().trimmed())
                                                 .arg(globalTime.elapsed()).arg(time.elapsed()));
#endif



    return readArr;
}

//-------------------------------------------------------------------------------------

QByteArray Conn2modem::readDeviceQuick(const QByteArray &endSymb, const bool &isclearbufmode)
{
    const DeviceTimeouts timeoutsl = this->timeouts;

    timeouts.global = isclearbufmode ? 60 : 300;
    timeouts.block = isclearbufmode ? 20 : 100;



    if(!isclearbufmode && lastConnectionType != IFACECONNTYPE_UART && !lModemState.workWithoutAPI){

        const int global = lModemState.lastWasATCNtest ? 300 : 3000;

        if(lModemState.isMainConnectionUsed){
#ifdef HASGUI4USR
            timeouts.global = qMin(timeoutsl.global, global);
#else
            ;
#endif
        }else{

            timeouts.global = qMin(timeoutsl.global, global);

        }
    }



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

    bool r = false;
    switch(lastConnectionType){
#ifndef DISABLE_SERIALPORT_MODE
    case IFACECONNTYPE_UART     : r = serialPort->waitForReadyRead(msecs)   ; break;
#endif
#ifndef DISABLE_TCPCLIENT_MODE
    case IFACECONNTYPE_TCPCLNT  : r = socket->waitForReadyRead(msecs)       ; break;
#endif
#ifndef DISABLE_M2M_MODULE
    case IFACECONNTYPE_M2MCLNT  : r = svahaConnector->waitForReadyRead(msecs); break;
#endif
    default: r = false; break;
    }
    return r ;


#else
    return socket->waitForReadyRead(msecs);
#endif
}

//-------------------------------------------------------------------------------------

bool Conn2modem::waitForBytesWritten(const int &msecs)
{
#ifdef ENABLE_EXTSUPPORT_OF_IFACES

    bool r = false;
    switch(lastConnectionType){
#ifndef DISABLE_SERIALPORT_MODE
    case IFACECONNTYPE_UART     : r = serialPort->waitForBytesWritten(msecs)    ; break;
#endif
#ifndef DISABLE_TCPCLIENT_MODE
    case IFACECONNTYPE_TCPCLNT  : r = socket->waitForBytesWritten(msecs)        ; break;
#endif
#ifndef DISABLE_M2M_MODULE
    case IFACECONNTYPE_M2MCLNT  : r = svahaConnector->waitForBytesWritten(msecs); break;
#endif
    default: r = false; break;
    }
    return r ;


#else
    return socket->waitForBytesWritten(msecs);
#endif
}

qint64 Conn2modem::bytesAvailable()
{
    if(!isConnectionWorking())
        return -1;
#ifdef ENABLE_EXTSUPPORT_OF_IFACES

    qint64 r = -1;
    switch(lastConnectionType){
#ifndef DISABLE_SERIALPORT_MODE
    case IFACECONNTYPE_UART     : r = serialPort->bytesAvailable()      ; break;
#endif
#ifndef DISABLE_TCPCLIENT_MODE
    case IFACECONNTYPE_TCPCLNT  : r = socket->bytesAvailable()          ; break;
#endif
#ifndef DISABLE_M2M_MODULE
    case IFACECONNTYPE_M2MCLNT  : r = svahaConnector->bytesAvailable()  ; break;
#endif
    default: r = -2; break;
    }
    return r;


#else
    return socket->bytesAvailable();
#endif
}

QByteArray Conn2modem::clearBufferRead()
{
    return (bytesAvailable() > 0) ? readAll() : QByteArray();
}

//-------------------------------------------------------------------------------------

QByteArray Conn2modem::readAll()
{
    if(!isConnectionWorking())
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
    if(lModemState.lastWasATCNtest)
        lModemState.lastWasATCNtest = false;
    if(verboseMode)
        qDebug() << "ZbyratorObject::write2dev " << lModemState.directAccess << writeArr.simplified() << writeArr.isEmpty() << writePreffix << lModemState.uartBlockPrtt;

    const bool isConnOk = isConnectionWorking();
#ifdef ENABLE_VERBOSE_SERVER
    if(activeDbgMessages)  emit appendDbgExtData(dbgExtSrcId, QString("Conf2modem write2dev directAccess=%1, uartBlockPrtt=%2, myPrtt=%3, writeArr=%4, lastConnectionType=%5, isConnOk=%6")
                                                 .arg(lModemState.directAccess)
                                                 .arg(lModemState.uartBlockPrtt)
                                                 .arg(QString(writePreffix))
                                                 .arg(QString(writeArr.toHex()) + " " + QString(writeArr).simplified().trimmed())
                                                 .arg((int)lastConnectionType)
                                                 .arg(int(isConnOk)));
#endif
    if(!isConnOk)
        return 0;

    emit dataReadWriteReal(writeArr, ifaceName, false);
    if(!ignoreDaAndPrtt && lModemState.directAccess)
        return 0;

    if(writeArr.isEmpty() || lastConnectionType == IFACECONNTYPE_UNKN)
        return 0;


    //<prtt>;<space>;<data>
    //prtt.len = 1byte
#ifdef DISABLE_UART_PRIORITY
    const qint64 len = write(writeArr);
#else
    const qint64 len = write( (lModemState.isMainConnectionUsed ? writePreffix : QByteArray()) + writeArr);// QByteArray::number(lastPeredavatorPrtt) + "; ;"
#endif
    const bool wasatcncommand = (len == 6 && writeArr == "ATCN\r\n");
    if(wasatcncommand)
        lModemState.isModemInCommandMode = false;

    if(lModemState.uartBlockPrtt)
        lModemState.lastCommandWasAtcn = wasatcncommand;

    if(ignoreDaAndPrtt && wasatcncommand)
        lModemState.lastWasATCNtest = true;

    if(verboseMode)
        qDebug() << "ZbyratorObject::write2dev len=" << len << writePreffix << lModemState.lastCommandWasAtcn << lModemState.lastWasATCNtest;
#ifdef ENABLE_VERBOSE_SERVER
    if(activeDbgMessages)  emit appendDbgExtData(dbgExtSrcId, QString("Conf2modem write2dev directAccess=%1, uartBlockPrtt=%2, myPrtt=%3, len=%4")
                                                 .arg(lModemState.directAccess).arg(lModemState.uartBlockPrtt).arg(QString(writePreffix)).arg(len));
#endif
    lModemState.lastWriteLen = len;
    waitForBytesWritten(50);
    emit dataWrite(writeArr);

    return len;
}

//-------------------------------------------------------------------------------------
#ifndef DISABLE_TCPCLIENT_MODE

bool Conn2modem::openTcpConnection(const bool &workWithoutAPI, const QStringList &hosts, const QList<quint16> &ports)
{
    for(int i = 0, imax = hosts.size(); i < imax; i++){
        if(openTcpConnection(workWithoutAPI, hosts.at(i), ports.at(i)))
            return true;
    }
    return false;
}

//-------------------------------------------------------------------------------------

bool Conn2modem::openTcpConnection(const bool &workWithoutAPI, const QString &host, const quint16 &port)
{
    onDeviceDestr();//reset params
    lastConnectionType = IFACECONNTYPE_TCPCLNT;
    const int wait4conn = qMax(timeouts.global, 5000);
    if(verboseMode)
        qDebug() << "Conf2modem::openTcpConnection() " << host << port << timeouts.global << wait4conn ;

#ifdef ENABLE_VERBOSE_SERVER
    if(activeDbgMessages)  emit appendDbgExtData(dbgExtSrcId, QString("Conf2modem openTcpConnection host=%1, port=%2, myPrtt=%3").arg(host).arg(port).arg(QString(writePreffix)));
#endif

    emit openingTcpConnection();
    emit openingAconnection();

    stopAll = false;
    socket->connectToHost(host, port);
    lModemState.workWithoutAPI = workWithoutAPI;

    const bool r = socket->waitForConnected(wait4conn);

    if(!r){
#ifdef ENABLE_VERBOSE_SERVER
        if(activeDbgMessages)  emit appendDbgExtData(dbgExtSrcId, QString("Conf2modem openTcpConnection err=%1").arg(socket->errorString()));
#endif
        emit currentOperation(tr("Couldn't connect to the remote device. %1").arg(socket->errorString()));//%1 %2, %3").arg(host).arg(port).arg(socket->errorString()));
        closeDevice();
    }else{
        connectionDownCounter = 0;
        ifaceName = host.contains(":") ? QString("[%1]:%2").arg(host).arg(port) : QString("%1:%2").arg(host).arg(port) + ifacesuffix;
        emit currentOperation(tr("Connection to the remote device was established)"));
    }
    if(verboseMode)
        qDebug() << "Conf2modem::openTcpConnection() " << host << port << r ;


    //#ifdef DISABLE_UART_PRIORITY
    //   if(r){
    //        writeATcommand("ATCN");
    //        readDeviceQuick("\r\n");

    //    }
    //#endif

    return r;
}
#endif
//-------------------------------------------------------------------------------------

#ifdef ENABLE_EXTSUPPORT_OF_IFACES

#ifndef DISABLE_M2M_MODULE
bool Conn2modem::openM2mConnection(const bool &workWithoutAPI, const QVariantHash &oneProfile)
{
    onDeviceDestr();//reset params
    emit openingAconnection();
    lastConnectionType = IFACECONNTYPE_M2MCLNT;
    stopAll = false;

    const int timeout = qMax(timeouts.global, 7000);
    svahaConnector->connect2hostViaSvaha(oneProfile, timeout, timeouts.block);
    lModemState.workWithoutAPI = workWithoutAPI;

    const bool r = svahaConnector->waitForConnected(timeout);
    if(!r){
#ifdef ENABLE_VERBOSE_SERVER
        if(activeDbgMessages)  emit appendDbgExtData(dbgExtSrcId, QString("Conf2modem openTcpConnection err=%1").arg(svahaConnector->errorString()));
#endif
        emit currentOperation(tr("Couldn't connect to the remote device. %1").arg(svahaConnector->errorString()));//%1 %2, %3").arg(host).arg(port).arg(socket->errorString()));
        closeDevice();
    }else{
        ifaceName = svahaConnector->getLastConnDev() + ifacesuffix;
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
bool Conn2modem::openSerialPort(const bool &workWithoutAPI, const QString &portName, const qint32 &baudRate, const QStringList &uarts, const qint8 &databits, const qint8 &stopbits, const qint8 &parity, const qint8 &flowcontrol)
{

    onDeviceDestr();//reset params
    emit openingAconnection();
    stopAll = false;
    lastConnectionType = IFACECONNTYPE_UART;
    QString mess;
    lModemState.workWithoutAPI = workWithoutAPI;


    const bool nhasUarts = (uarts.isEmpty() || uarts.join("").isEmpty());//there was an error here, but now everything is fine
    const bool pnameisembpty = portName.isEmpty();


    const bool r = (nhasUarts && !pnameisembpty) ? // (uarts.isEmpty() && !portName.isEmpty()) ?
                testModemOnPort(portName, baudRate, mess, databits, stopbits, parity, flowcontrol) : //manual mode
                findModemOnPort(portName, baudRate, uarts, mess, databits, stopbits, parity, flowcontrol); //detection mode


    if(!r){
#ifdef ENABLE_VERBOSE_SERVER
        if(activeDbgMessages)  emit appendDbgExtData(dbgExtSrcId, QString("Conf2modem openSerialPort err=%1").arg(mess));
#endif
        if(verboseMode)
            qDebug() << " Conn2modem::openSerialPort " << nhasUarts << pnameisembpty << uarts.size() << uarts;
        emit currentOperation(tr("Couldn't connect to the serial port. %1").arg(mess));//%1 %2, %3").arg(host).arg(port).arg(socket->errorString()));
        closeDevice();
    }else{
        connectionDownCounter = 0;
        ifaceName = serialPort->portName() + ifacesuffix;
        emit currentOperation(tr("Connection to the serial port was established)"));


        emit currentOperation(tr("Serialport settings are %1, %2, %3, %4")
                              .arg(QString::number(databits))
                              .arg(QString("No parity;Even;Odd;Space;Mark").split(";").at(qMin(4, qMax(0, int(parity)))) )
                              .arg(QString::number(stopbits))
                              .arg(QString("No flowcontrol;Hardware RTS/CTS;Software XON/XOFF").split(";").at(qMin(2, qMax(0, int(flowcontrol)))) )
                              );

    }

    return r;
}

bool Conn2modem::testModemOnPort(QString defPortName, qint32 baudR, QString &lastError, const qint8 &databits, const qint8 &stopbits, const qint8 &parity, const qint8 &flowcontrol)
{
    const QStringList uarts = defPortName.split("\n");
    const bool r = findModemOnPort("", baudR, uarts, lastError, databits, stopbits, parity, flowcontrol); //manual mode
    return r;

}

//-------------------------------------------------------------------------------------

bool Conn2modem::findModemOnPort(QString defPortName, qint32 baudR, QStringList uarts, QString &lastError, const qint8 &databits, const qint8 &stopbits, const qint8 &parity, const qint8 &flowcontrol)
{
    std::sort(uarts.begin(), uarts.end());
    if(!defPortName.isEmpty() && uarts.contains(defPortName)){
        uarts.removeOne(defPortName);
        uarts.prepend(defPortName);
    }

    const qint8 paritycorrection = (parity == 0) ? parity : (1 + parity);//see QSerialPort::Parity

    QSerialPort::DataBits dataBits=static_cast<QSerialPort::DataBits>(databits); //currentText().toInt());
    QSerialPort::Parity parityY=static_cast<QSerialPort::Parity>(paritycorrection);
    QSerialPort::StopBits stopBits=static_cast<QSerialPort::StopBits>(stopbits);
    QSerialPort::FlowControl flowControl=static_cast<QSerialPort::FlowControl>(flowcontrol);



    QString lastuarterr;
    for(int i = 0; i < 1000 && !uarts.isEmpty(); i++){

        if(openSerialPortExt(lModemState.workWithoutAPI, uarts.takeFirst(), baudR, dataBits, stopBits, parityY, flowControl, ignoreUartsChecks, lastuarterr)){
            lastError.clear();
            return true;
        }

    }


    lastError = lastError.isEmpty() ? tr("Couldn't find any device") : lastError;
    return false;
}

bool Conn2modem::openSerialPortExt(const bool &workWithoutAPI, const QString &portName, const qint32 &baudRate, const QSerialPort::DataBits &data, const QSerialPort::StopBits &stopbits,
                                   const QSerialPort::Parity &parity, const QSerialPort::FlowControl &flow, const bool &ignoreUartsChecks, QString &lastuarterr)
{


    emit currentOperation(tr("Trying to open %1").arg(portName));
    ifaceName = portName + ifacesuffix;
    serialPort->setPortName(portName);
    lModemState.workWithoutAPI = workWithoutAPI;

    if(serialPort->open(QIODevice::ReadWrite)){
        if(serialPort->setBaudRate(baudRate) && serialPort->setParity(parity) && serialPort->setStopBits(stopbits) &&
                serialPort->setDataBits(data) && serialPort->setFlowControl(flow)){
            lastuarterr.clear();
            serialPort->clear();
            need2closeSerial = false;
            emit onSerialPortOpened(portName);


            if(ignoreUartsChecks){
                emit currentOperation(tr("UART checks are disabled"));
                return true;
            }

            const bool r = request2modemOn();

            if(r)
                return true;

            emit stopCheckCurrPort();

        }else
            lastuarterr = serialPort->errorString();
    }
    lastuarterr = serialPort->errorString();
    serialPort->close();
    return false;
}

//-------------------------------------------------------------------------------------

bool Conn2modem::request2modemOn()
{

    for(int i = 1, j = 0; i <= 3 && !stopAll && j < 30; i++, j++){
        emit currentOperation(tr("Detecting the modem... Sending the AT command to %1.").arg(serialPort->portName()));
        writeATcommand("+++");
        const QByteArray readArr = readDeviceQuick("\r\n", false);

        if(readArr.startsWith("OKERROR") || readArr.startsWith("ERROR")){
            lModemState.isModemInCommandMode = true;
            return true;
        }

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
    if(isConnectionWorking()){
        for(int i = msleep > 10 ? 10 : msleep; i < msleep && isConnectionWorking() && time.elapsed() < msleep; i += 10)
            readDeviceQuick("\r\n", true);
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

//    if(verboseMode)
//        qDebug() << "checkUartAccess " << arr.isEmpty() << msecElapsed << lModemState.uartBlockPrtt << lModemState.lastCommandWasAtcn << lModemState.directAccess << arr.toHex();
    if(lModemState.uartBlockPrtt && lModemState.lastCommandWasAtcn && arr.isEmpty() && msecElapsed > 200){
        lModemState.directAccess = lModemState.uartBlockPrtt = false;
        return false;
    }

    if(!arr.isEmpty()){

        if(arr == PORT_IS_BUSY ){
            if(!lModemState.directAccess && msecElapsed <= MAX_MSEC_TIME2OPEN_DA){
                lModemState.directAccess = true;
                lModemState.uartBlockPrtt = false;
                //            emit onDaStateChanged(true);
                lModemState.isCoordinatorConfigReady = false;
            }

        }else{
            if(arr == PORT_IS_FREE || arr.endsWith(PORT_IS_FREE)){
                if(lModemState.directAccess){
                    lModemState.directAccess = lModemState.uartBlockPrtt = false;
                    //                emit onDaStateChanged(false);

                }
            }else{
                if(!writePreffix.isEmpty() && arr == PORT_IS_BUSY_LOCAL){
                    lModemState.uartBlockPrtt = true;
                    //                    emit onLowPriority2uart();

                }else{
                    if(msecElapsed > 0)
                        lModemState.directAccess = lModemState.uartBlockPrtt = false;
                }
            }
        }

    }
    if(verboseMode)
        qDebug() << "arr=" << arr << lModemState.directAccess << lModemState.uartBlockPrtt;
    return isUartBusy();
}

//-------------------------------------------------------------------------------------

bool Conn2modem::canCheckUartAccess(const bool &arrIsEmpty, const int &msecElapsed)
{
    return (!arrIsEmpty || (arrIsEmpty && msecElapsed > 20 && lModemState.lastCommandWasAtcn));

}
//-------------------------------------------------------------------------------------


bool Conn2modem::isCoordinatorFree()
{
#ifdef ENABLE_EXTSUPPORT_OF_IFACES

    if(lastConnectionType == IFACECONNTYPE_UART){
        lModemState.directAccess = false;
        lModemState.uartBlockPrtt = false;
        //        return true;


    }

    if(ignoreUartsChecks)
        return true;
#endif

    const bool busy = lModemState.uartBlockPrtt;

    QTime time; time.start();
#ifdef ENABLE_VERBOSE_SERVER
    if(activeDbgMessages)  emit appendDbgExtData(dbgExtSrcId, QString("Conf2modem isCoordinatorFree da directAccess=%1, uartBlockPrtt=%2, myPrtt=%3, time=%4")
                                                 .arg(lModemState.directAccess).arg(lModemState.uartBlockPrtt).arg(QString(writePreffix)).arg(time.elapsed()));
    const bool da = lModemState.directAccess;

#endif

    const bool nfree = lModemState.directAccess || lModemState.uartBlockPrtt;
    if(lModemState.directAccess){
#ifdef ENABLE_VERBOSE_SERVER
        if(activeDbgMessages)  emit appendDbgExtData(dbgExtSrcId, QString("Conf2modem ondirectaccess directAccess=%1, uartBlockPrtt=%2, myPrtt=%3, da=%4")
                                                     .arg(lModemState.directAccess).arg(lModemState.uartBlockPrtt).arg(QString(writePreffix)).arg(da));
#endif
        for(int i = 0; i < 3; i++){
            readDevice();
            if(!lModemState.directAccess && !lModemState.uartBlockPrtt)
                return true;
            if(lModemState.uartBlockPrtt)
                break;
        }

    }
    //    directAccess = uartBlockPrtt = false;//reset



    writeATcommand("ATCN", true);//у відповідь може бути нічого, тому важливо почати обмін з мінімальною затримкою
    if(readDeviceQuick("\r\n", false).contains("ERR") || lModemState.uartBlockPrtt){
        writeATcommand("ATCN", true);
        readDeviceQuick("\r\n", false);//в цій функції уже є перевірка прямого доступу

    }
#ifdef ENABLE_VERBOSE_SERVER
    if(da != lModemState.directAccess){
        if(activeDbgMessages)  emit appendDbgExtData(dbgExtSrcId, QString("Conf2modem isCoordinatorFree da directAccess=%1, uartBlockPrtt=%2, myPrtt=%3, da=%4")
                                                     .arg(lModemState.directAccess).arg(lModemState.uartBlockPrtt).arg(QString(writePreffix)).arg(da));
        //        emit onDaStateChanged(directAccess);
    }
#endif

    if(busy != lModemState.uartBlockPrtt){

        emit onBusyByPriority(lModemState.uartBlockPrtt);
    }

    const bool nfree2 = (lModemState.directAccess || lModemState.uartBlockPrtt);
    if(nfree == nfree2){
        readDeviceQuick("\r\n", true);

    }

    const bool nfree3 = (lModemState.directAccess || lModemState.uartBlockPrtt);
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
    if(lModemState.directAccess || lModemState.uartBlockPrtt){
        if(verboseMode)
            qDebug() << mess << lModemState.directAccess << lModemState.uartBlockPrtt;

        if(!lModemState.directAccess && lModemState.uartBlockPrtt && arrIsEmpty && lModemState.lastCommandWasAtcn)
            lModemState.uartBlockPrtt = false;
        return false; //when I did that????
#ifdef ENABLE_VERBOSE_SERVER
        if(activeDbgMessages)  emit appendDbgExtData(dbgExtSrcId, mess +  QString("directAccess=%1, uartBlockPrtt=%2, myPrtt=%3, sayGoodByeIfUartIsNFree, msec4blockUart=%4")
                                                     .arg(lModemState.directAccess).arg(lModemState.uartBlockPrtt).arg(QString(writePreffix)).arg(msec4blockUart));
#endif
        if(msec4blockUart > 0)
            QThread::msleep(msec4blockUart);
        return true;
    }
    return false;
}

void Conn2modem::setIfaceNameSuffix(QString suffix)
{
    ifacesuffix = suffix;
}

//-------------------------------------------------------------------------------------

void Conn2modem::setWritePreffix(QByteArray preffix)
{
#ifdef ENABLE_VERBOSE_SERVER
    if(writePreffix != preffix)
        if(activeDbgMessages)  emit appendDbgExtData(dbgExtSrcId, QString("Conf2modem setWritePreffix directAccess=%1, uartBlockPrtt=%2, myPrtt=%3, preffix=%4")
                                                     .arg(lModemState.directAccess).arg(lModemState.uartBlockPrtt).arg(QString(writePreffix)).arg(QString(preffix)));
#endif
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
    //it is not only for API errors, it is for all communication errors
    //but you have to tell about connection type - main or additional
    lModemState.apiErrCounter++;

    if(lastConnectionType != IFACECONNTYPE_UNKN && lModemState.apiErrCounter > MAX_TRIES_FOR_CONFIG){
        close();
    }

    emit onApiErrorIncremented(lModemState.apiErrCounter);
    emit currentOperation(tr("Exchange error, counter: %1, can reset the coordinator: %2").arg(lModemState.apiErrCounter).arg(int(lModemState.isMainConnectionUsed)));

    if(!lModemState.isMainConnectionUsed){
        return;
    }

    if(lModemState.apiErrCounter > MAX_TRIES_FOR_HARD_RESET){
#ifdef ENABLE_VERBOSE_SERVER
        if(activeDbgMessages)  emit appendDbgExtData(dbgExtSrcId, QString("Conf2modem incrementApiErrCounter directAccess=%1, uartBlockPrtt=%2, myPrtt=%3, apiErrCounter=%4")
                                                     .arg(lModemState.directAccess).arg(lModemState.uartBlockPrtt).arg(QString(writePreffix)).arg(lModemState.apiErrCounter));
#endif
        emit currentOperation(tr("Exchange error, resetting the coordinator"));

        lModemState.apiErrCounter = 0;//обнуляю щоб не дуже часто скидало координатора
        emit resetCoordinatorRequest();
    }else if(lModemState.apiErrCounter > MAX_TRIES_FOR_CONFIG){
#ifdef ENABLE_VERBOSE_SERVER
        if(activeDbgMessages)  emit appendDbgExtData(dbgExtSrcId, QString("Conf2modem incrementApiErrCounter directAccess=%1, uartBlockPrtt=%2, myPrtt=%3, apiErrCounter=%4")
                                                     .arg(lModemState.directAccess).arg(lModemState.uartBlockPrtt).arg(QString(writePreffix)).arg(lModemState.apiErrCounter));
#endif
        emit currentOperation(tr("Exchange error, killing the TcpToEmbee service"));
        emit killPeredavatorRequest();
    }
}

//-------------------------------------------------------------------------------------

void Conn2modem::closeDevice()
{
    if(isConnectionWorking()){
        close();
        emit currentOperation(tr("Connection to the remote device has been closed"));
        lastConnState = true;
    }
    if(lastConnState){
        lastConnState = false;
        emit onConnectionDown();//why does it tell to kill UCon tasks?

    }
}

//-------------------------------------------------------------------------------------

void Conn2modem::resetBlockByPrtt()
{
    lModemState.uartBlockPrtt = false;
}

//-------------------------------------------------------------------------------------

void Conn2modem::resetDaState()
{
    lModemState.directAccess = false;
}
//-------------------------------------------------------------------------------------

#ifdef ENABLE_EXTSUPPORT_OF_IFACES

void Conn2modem::simpleClose()
{
    if(lastConnectionType == IFACECONNTYPE_UART)
        emit stopCheckCurrPort();
    close();
}

//-------------------------------------------------------------------------------------
void Conn2modem::setIgnoreUartChecks(bool ignore)
{
    ignoreUartsChecks = ignore;
}


#ifndef DISABLE_SERIALPORT_MODE

void Conn2modem::closeSerialPort() //only if the port was disconnected not by the app
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

void Conn2modem::activateAsyncModeExt(const quint8 &conntype)
{

#ifndef DISABLE_TCPCLIENT_MODE
    if(conntype == IFACECONNTYPE_TCPCLNT || conntype == IFACECONNTYPE_UNKN)
        connect(socket, SIGNAL(readyRead()), this, SIGNAL(readyRead()) );
#endif

#ifdef ENABLE_EXTSUPPORT_OF_IFACES


#ifndef DISABLE_M2M_MODULE
    if(conntype == IFACECONNTYPE_M2MCLNT || conntype == IFACECONNTYPE_UNKN){
        connect(svahaConnector, SIGNAL(readyRead()), this, SIGNAL(readyRead()) );
        svahaConnector->setAsyncMode(true);
    }
#endif
    if(conntype == IFACECONNTYPE_UART || conntype == IFACECONNTYPE_UNKN){
        connect(this, SIGNAL(detectedDisconnectedSerialPort()), this, SLOT(closeSerialPort()));
        connect(serialPort, SIGNAL(readyRead()), this, SIGNAL(readyRead()) );
    }

#endif

}

//-------------------------------------------------------------------------------------

void Conn2modem::activateAsyncMode()
{
    activateAsyncModeExt(lastConnectionType);

}

//-------------------------------------------------------------------------------------

void Conn2modem::deactivateAsyncMode()
{
#ifndef DISABLE_TCPCLIENT_MODE
    disconnect(socket, SIGNAL(readyRead()), this, SIGNAL(readyRead()) );
#endif

#ifdef ENABLE_EXTSUPPORT_OF_IFACES


#ifndef DISABLE_M2M_MODULE
    disconnect(svahaConnector, SIGNAL(readyRead()), this, SIGNAL(readyRead()) );
    svahaConnector->setAsyncMode(false);
#endif

    disconnect(serialPort, SIGNAL(readyRead()), this, SIGNAL(readyRead()) );
    disconnect(this, SIGNAL(detectedDisconnectedSerialPort()), this, SLOT(closeSerialPort()));

#endif
}

void Conn2modem::exitCommandModeSimple()
{

    writeATcommand("ATCN");
    readDeviceQuick("\r\n", true);


}

//-------------------------------------------------------------------------------------

void Conn2modem::createDevices()
{
    lModemState.isCoordinatorConfigReady = false;
#ifndef DISABLE_TCPCLIENT_MODE

    socket = new QTcpSocket(this);
    connect(socket, SIGNAL(disconnected()), this, SIGNAL(onConnectionClosed()) );
#endif


#ifdef ENABLE_EXTSUPPORT_OF_IFACES

    setIgnoreUartChecks(false);
#ifndef DISABLE_M2M_MODULE
    svahaConnector = new SvahaServiceConnector(this);
    connect(svahaConnector, SIGNAL(disconnected()), this, SIGNAL(onConnectionClosed()) );
#endif

    serialPort = new QSerialPort(this);


    need2closeSerial = false;
    CheckCurrPort *checkPort = new CheckCurrPort(this);
    checkPort->setObjectName("CheckCurrPort");

    connect(checkPort, SIGNAL(portDisconnected(bool)), this, SLOT(closeSerialPortDirect()), Qt::DirectConnection);
    connect(checkPort,SIGNAL(terminateNow()), this, SLOT(closeSerialPortDirect()), Qt::DirectConnection);
    connect(checkPort, SIGNAL(portDisconnected(bool)), this, SIGNAL(detectedDisconnectedSerialPort()));

    connect(this, SIGNAL(onConnectionClosed()), checkPort, SLOT(terminate()) );
    connect(this, SIGNAL(stopCheckCurrPort()), checkPort, SLOT(terminate()) );
    connect(this, SIGNAL(onConnectionDown()), checkPort, SLOT(terminate()) );
    connect(this, SIGNAL(onSerialPortOpened(QString)), checkPort, SLOT(zapuskalka(QString)) );
    //    connect(checkPort, &CheckCurrPort::appendMessage, this, &Conn2modem::currentOperation); //it is only for bugs

#endif
}

//-------------------------------------------------------------------------------------

void Conn2modem::onDeviceDestr()
{
    close();
#ifdef ENABLE_VERBOSE_SERVER
    if(activeDbgMessages)  emit appendDbgExtData(dbgExtSrcId, QString("Conf2modem onDeviceDestr directAccess=%1, uartBlockPrtt=%2, myPrtt=%3, apiErrCounter=%4")
                                                 .arg(lModemState.directAccess).arg(lModemState.uartBlockPrtt).arg(QString(writePreffix)).arg(lModemState.apiErrCounter));
#endif
    lModemState.modemIsOverRS485 = false;

    lModemState.directAccess = lModemState.uartBlockPrtt = false;

    if(qAbs(lModemState.hashMsecWhenCoordinatorWasGood.value(ifaceName, 0) - QDateTime::currentMSecsSinceEpoch()) > MAX_MSEC_FOR_COORDINATOR_READY)
        lModemState.isCoordinatorConfigReady = false;

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

    lModemState.lastCommandWasAtcn = false;
    stopAllSlot();
}

//-------------------------------------------------------------------------------------

qint64 Conn2modem::write(const QByteArray &arr)
{
#ifdef ENABLE_EXTSUPPORT_OF_IFACES
    emit onReadWriteOperation(false);

    qint64 r = -1;
    switch(lastConnectionType){
#ifndef DISABLE_SERIALPORT_MODE
    case IFACECONNTYPE_UART     : r = serialPort->write(arr); break;
#endif
#ifndef DISABLE_TCPCLIENT_MODE
    case IFACECONNTYPE_TCPCLNT  : r = socket->write(arr); break;
#endif
#ifndef DISABLE_M2M_MODULE
    case IFACECONNTYPE_M2MCLNT  : r = svahaConnector->write(arr); break;
#endif
    default: r = -2; break;
    }
    return r;


#else
    return socket->write(arr);
#endif
}

//-------------------------------------------------------------------------------------

void Conn2modem::close()
{
    lModemState.uartBlockPrtt = lModemState.directAccess = false;//reset the states

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
