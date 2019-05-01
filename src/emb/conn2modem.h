#ifndef CONN2MODEM_H
#define CONN2MODEM_H

#include <QObject>
#include <QIODevice>
#include <QStringList>
#include <QTcpSocket>

#ifdef ENABLE_EXTSUPPORT_OF_IFACES
#ifndef DISABLE_SERIALPORT_MODE
#include <QSerialPort>
///[!] ifaces
#include "checkcurrport.h"
#endif

#ifndef DISABLE_M2M_MODULE
///[!] m2m-connector
#include "src/m2m-service/svahaserviceconnector.h"
#endif

#endif


#include "ifaceconnectiondefs.h"



struct DeviceTimeouts
{
    int block;
    int global;
    DeviceTimeouts() : block(77), global(7777) {}
};



class Conn2modem : public QObject
{
    Q_OBJECT
public:
    explicit Conn2modem(const int &dbgExtSrcId, const bool &verboseMode, QObject *parent = nullptr);



    quint8 lastConnectionType;
    QString ifaceName;

    quint8 writePrtt;

    QByteArray writePreffix;
    int dbgExtSrcId;
    bool activeDbgMessages;

    bool verboseMode;
    bool stopAll;

    DeviceTimeouts timeouts;

    bool directAccess;
    bool uartBlockPrtt;

    bool isCoordinatorConfigReady;
    quint8 apiErrCounter;
    qint64 msecWhenCoordinatorWasGood;

    bool lastCommandWasAtcn;

    quint16 connectionDownCounter;

    bool modemIsOverRS485;

    QTcpSocket *socket;

    bool need2closeSerial;
#ifdef ENABLE_EXTSUPPORT_OF_IFACES
    bool ignoreUartsChecks;

#ifndef DISABLE_M2M_MODULE
    SvahaServiceConnector *svahaConnector;
#endif

#ifndef DISABLE_SERIALPORT_MODE
    QSerialPort *serialPort;
#endif

#endif

    quint16 getConnectionDownCounter() const ;

    quint8 getWritePrtt() const;

    QString getLastIfaceName() const;

    bool isDirectAccess() const;

    bool isBlockByPrtt() const;

    bool isUartBusy() const;



    bool isConnectionWorks();

    bool isConnectionWorks(int waitMsec);

    bool isTcpConnectionWorks(QTcpSocket *socket);

    QByteArray readDevice();

    QByteArray readDevice(const QByteArray &endSymb, const bool &isQuickMode);

    QByteArray readDeviceQuick(const QByteArray &endSymb, const bool &isclearbufmode);

    bool waitForReadyRead(const int &msecs);
    bool waitForBytesWritten(const int &msecs);

    QByteArray readAll();

    qint64 writeATcommand(const QString &atcommand);

    qint64 writeATcommand(const QString &atcommand, const bool &ignoreDaAndPrtt);


    qint64 write2dev(const QByteArray &writeArr);

    qint64 write2dev(const QByteArray &writeArr, const bool &ignoreDaAndPrtt);


    bool openTcpConnection(const QStringList &hosts, const QList<quint16> &ports);

    bool openTcpConnection(const QString &host, const quint16 &port);



#ifdef ENABLE_EXTSUPPORT_OF_IFACES


#ifndef DISABLE_M2M_MODULE
    bool openM2mConnection(const QVariantHash &oneProfile);
#endif

#ifndef DISABLE_SERIALPORT_MODE
    bool openSerialPort(const QString &portName, const qint32 &baudRate, const QStringList &uarts);

    bool findModemOnPort(QString defPortName, qint32 baudR, QStringList uarts, QString &lastError);

    bool request2modemOn();
#endif

#endif

    void lSleep(const int &msleep);

    bool checkUartAccess(const QByteArray &arr, const int &msecElapsed);

    bool canCheckUartAccess(const bool &arrIsEmpty, const int &msecElapsed);

    bool isCoordinatorFree();

    bool isCoordinatorFreeWithRead();

    bool sayGoodByeIfUartIsNFree(const QString &mess, const int &msec4blockUart);

    bool sayGoodByeIfUartIsNFree(const QString &mess, const int &msec4blockUart, const bool &arrIsEmpty);



signals:
    void dataReadWriteReal(QByteArray arr, QString ifaceName, bool isRead);

    void dataReadReal(QByteArray readArr);


    void dataRead(QByteArray readArr);

    void dataWrite(QByteArray writeArr);

    void appendDbgExtData(quint32 sourceType, QString data);

    void dataFromDevice();

    void onReadWriteOperation(bool isRead);

    void openingTcpConnection();

    void currentOperation(QString mess);

    void onCoordinatorIsBusy(bool busy);

    void onBusyByPriority(bool isBusy);

    void killPeredavatorRequest();
    void resetCoordinatorRequest();//hardware

    void onConnectionClosed();

    void onConnectionDown();

    void onSerialPortOpened(QString portName);
    void stopCheckCurrPort();

    void need2closeSerialPort();

public slots:
    void setWritePreffix(QByteArray preffix);

    void setWritePriority(const quint8 &prtt);

    void stopAllSlot();
    void resetStopAllConfModem();

    void setTimeouts(int global, int block);

    void incrementApiErrCounter();

    void closeDevice();

    void resetBlockByPrtt();

    void resetDaState();

#ifdef ENABLE_EXTSUPPORT_OF_IFACES
    void simpleClose();

    void setIgnoreUartChecks(bool ignore);

#ifndef DISABLE_SERIALPORT_MODE
    void closeSerialPort();

    void closeSerialPortDirect();
#endif
#endif

private slots:
    void createDevices();

    void onDeviceDestr();



private:
    qint64 write(const QByteArray &arr);

    void close();

};

#endif // CONN2MODEM_H
