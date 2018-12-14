#ifndef CONF2MODEM_H
#define CONF2MODEM_H

#include <QObject>
#include <QIODevice>
#include <QStringList>
#include <QTcpSocket>

#ifdef ENABLE_EXTSUPPORT_OF_IFACES
#include "src/m2m-service/svahaserviceconnector.h"
#include "checkcurrport.h"
#include <QSerialPort>
#endif

#define PORT_IS_BUSY        "\r\nUartIsBusy\r\n"
#define PORT_IS_FREE        "\r\nUartIsFree\r\n"
#define PORT_IS_BUSY_LOCAL  "\r\nUartIsBusyPrtt\r\n"
#define PORT_IS_NFREE       "\r\nUartIs"


struct DeviceTimeouts
{
    int block;
    int global;
    DeviceTimeouts() : block(77), global(7777) {}
};

//class Conf2modem : public QObject
//{

//#include "conf2modem_global.h"

class Conf2modem : public QObject
{
    Q_OBJECT
public:
    explicit Conf2modem(const int &dbgExtSrcId, const bool &verboseMode, QObject *parent = Q_NULLPTR);

    QTcpSocket *socket;

#ifdef ENABLE_EXTSUPPORT_OF_IFACES
    SvahaServiceConnector *svahaConnector;
    QSerialPort *serialPort;
#endif

    quint16 getConnectionDownCounter() const ;

    void createDevices();

    quint8 getWritePrtt() const;

    QString getLastIfaceName() const;

    bool isDirectAccess() const;

    bool isBlockByPrtt() const;

    bool isUartBusy() const;

    bool enterCommandMode();

    bool networkReset(QString &errStr);

    bool enableDisableApi(const bool &enable, const bool &readAboutZigBee);

    bool isConnectionWorks();

    bool isConnectionWorks(int waitMsec);

    bool isTcpConnectionWorks(QTcpSocket *socket);

    bool isCoordinatorGood(const bool &forced, const bool &readAboutZigBee);


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
    bool openM2mConnection(const QVariantHash &oneProfile);

    bool openSerialPort(const QString &portName, const qint32 &baudRate, const QStringList &uarts);

    bool findModemOnPort(QString defPortName, qint32 baudR, QStringList uarts, QString &lastError);

    bool request2modemOn();


#endif

    void lSleep(const int &msleep);

    bool checkUartAccess(const QByteArray &arr, const int &msecElapsed);

    bool canCheckUartAccess(const bool &arrIsEmpty, const int &msecElapsed);

    bool isCoordinatorFree();

    bool isCoordinatorFreeWithRead();

    bool sayGoodByeIfUartIsNFree(const QString &mess, const int &msec4blockUart);

    bool sayGoodByeIfUartIsNFree(const QString &mess, const int &msec4blockUart, const bool &arrIsEmpty);


signals:
    void currentOperation(QString mess);

    void dataFromDevice();

    void openingTcpConnection();

//    void onDaStateChanged(bool isdamodenow);

//    void onLowPriority2uart();


    void dataReadReal(QByteArray readArr);


    void dataRead(QByteArray readArr);

    void dataWrite(QByteArray writeArr);

    void dataReadWriteReal(QByteArray arr, QString ifaceName, bool isRead);

    void onAboutZigBee(QVariantHash hash);


    void onCoordinatorIsBusy(bool busy);


    void onBusyByPriority(bool isBusy);

    void onApiModeEnabled();
    void killPeredavatorRequest();
    void resetCoordinatorRequest();//hardware

    void appendDbgExtData(quint32 sourceType, QString data);


    void onConnectionClosed();

    void onConnectionDown();

    void onSerialPortOpened(QString portName);
    void stopCheckCurrPort();
    void onReadWriteOperation(bool isRead);

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
    void closeSerialPort();


#endif
private slots:
    void onDeviceDestr();

private:
    qint64 write(const QByteArray &arr);

    void close();

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

//    bool enExtIface;
//    bool disableUartPrtt;

};

#endif // CONF2MODEM_H
