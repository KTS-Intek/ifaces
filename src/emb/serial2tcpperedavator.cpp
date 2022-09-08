#include "serial2tcpperedavator.h"
#include <QTime>


#ifdef HASSERIALLIB
///[!] ifaces
#include "src/emb/checkcurrport.h"
#endif

Serial2tcpPeredavator::Serial2tcpPeredavator(QObject *parent) : QObject(parent)
{

}

void Serial2tcpPeredavator::onThreadStarted()
{
#ifdef HASSERIALLIB

    serial = new QSerialPort(this);
    CheckCurrPort *checkPort = new CheckCurrPort(this);

    connect(checkPort, SIGNAL(portDisconnected(bool)), this, SLOT(closeSerialPort()));
    connect(checkPort,SIGNAL(terminateNow()), this, SLOT(closeSerialDirect()), Qt::DirectConnection);

    connect(this, SIGNAL(onConnectionClosed()), checkPort, SLOT(terminate()) );
    connect(this, SIGNAL(stopCheckCurrPort()), checkPort, SLOT(terminate()) );
    connect(this, SIGNAL(onConnectionDown()), checkPort, SLOT(terminate()) );
    connect(this, SIGNAL(onSerialPortOpened(QString)), checkPort, SLOT(zapuskalka(QString)) );
    connect(serial, SIGNAL(readyRead()), this, SLOT(mReadyReadSerial()));
#endif

    socket = new QTcpSocket(this);
    connect(socket, SIGNAL(disconnected()), this, SLOT(onTcpDown()));

    connect(socket, SIGNAL(readyRead()), this, SLOT(mReadyReadTcp()));

    serialTimeout = 50;
    tcpTimeout = 5;

    isSerialOpen = isTcpOpen = false;


    connect(this, SIGNAL(onConnectionClosed()), this, SLOT(closeConnect()) );
}

void Serial2tcpPeredavator::setBlockTimeoutSerial(int msec)
{
    serialTimeout = msec;
}

void Serial2tcpPeredavator::setBlockTimeoutTcp(int msec)
{
    tcpTimeout = msec;
}

void Serial2tcpPeredavator::mReadyReadSerial()
{
#ifdef HASSERIALLIB

    disconnect(serial, SIGNAL(readyRead()), this, SLOT(mReadyReadSerial()));
    const QByteArray a = mReadyReadSerialF();
    connect(serial, SIGNAL(readyRead()), this, SLOT(mReadyReadSerial()));

    socket->write(a);
#endif
}

void Serial2tcpPeredavator::mReadyReadTcp()
{
#ifdef HASSERIALLIB

    disconnect(socket, SIGNAL(readyRead()), this, SLOT(mReadyReadTcp()));
    const QByteArray a = mReadyReadTcpF();
    connect(socket, SIGNAL(readyRead()), this, SLOT(mReadyReadTcp()));
    serial->write(a);
#endif
}

void Serial2tcpPeredavator::openConnection(QString port, qint32 baudRate, QStringList hosts, int tcpPort)
{
    stopAll = false;
    if(openSerial(port, baudRate)){
        if(hosts.isEmpty()){
            hosts.prepend("127.0.0.1");
            hosts.prepend("::1");
        }
        if(tcpPort < 1 || tcpPort > 65534)
            tcpPort = 8989;
        if(openTcp(hosts, tcpPort)){
            emit onConnectionWorks(true);
            return;
        }
    }
    closeConnect();
}

void Serial2tcpPeredavator::closeConnect()
{
    closeSerialPort();
    onTcpDown();
    emit onConnectionWorks(false);
}

void Serial2tcpPeredavator::closeSerialPort()
{
    if(isSerialOpen){
        isSerialOpen = false;
#ifdef HASSERIALLIB
        serial->close();
#endif
        emit stopCheckCurrPort();
        emit onConnectionClosed();
    }
}

void Serial2tcpPeredavator::closeSerialDirect()
{
    stopAll = true;
}

void Serial2tcpPeredavator::onTcpDown()
{
    if(isTcpOpen){
        isTcpOpen = false;
        socket->close();
        emit onConnectionClosed();
    }
}

bool Serial2tcpPeredavator::openSerial(const QString &port, const qint32 &baudRate)
{
#ifdef HASSERIALLIB

    serial->setPortName(port);
    if(serial->open(QIODevice::ReadWrite)){
        if(serial->setBaudRate(baudRate) && serial->setParity(QSerialPort::NoParity) && serial->setStopBits(QSerialPort::OneStop) &&
                serial->setDataBits(QSerialPort::Data8) && serial->setFlowControl(QSerialPort::NoFlowControl)){
            serial->clear();
            emit onSerialPortOpened(port);
            isSerialOpen = true;
            return true;
        }
    }
    serial->close();
    const QString err = serial->errorString();
#else
    const QString err = QString("baud: %1, !HASSERIALLIB").arg(baudRate);
#endif
    emit showMessage(tr("Couldn't open the serial port %1.<br>%2").arg(port).arg(err));
    return false;
}

bool Serial2tcpPeredavator::openTcp(const QStringList &hosts, const int &tcpPort)
{

    for(int i = 0, imax = hosts.size(); i < imax; i++){
        socket->connectToHost(hosts.at(i), tcpPort);
        if(socket->waitForConnected(50)){
            isTcpOpen = true;
            return true;
        }
    }
    return false;
}

QByteArray Serial2tcpPeredavator::mReadyReadSerialF()
{
#ifdef HASSERIALLIB
    return mReadyReadF(serial, serialTimeout);
#endif
}

QByteArray Serial2tcpPeredavator::mReadyReadTcpF()
{
    return mReadyReadF(socket, tcpTimeout);
}

QByteArray Serial2tcpPeredavator::mReadyReadF(QIODevice *d, const int &timeout)
{
    QTime t, gt;
    gt.start();
    QByteArray a = d->readAll();
    if(timeout < 1)
        return a;
    t.start();
    for(int i = 0; i < 100000 && t.elapsed() < timeout && gt.elapsed() < 9000; i++){
        if(d->waitForReadyRead(timeout)){
            a.append(d->readAll());
            t.restart();
        }
        if(!isSerialOpen || !isTcpOpen || stopAll)
            break;
    }
    return a;
}
