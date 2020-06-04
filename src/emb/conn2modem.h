#ifndef CONN2MODEM_H
#define CONN2MODEM_H

#include <QObject>
#include <QIODevice>
#include <QStringList>
#include <QHash>

#ifndef DISABLE_TCPCLIENT_MODE
#include <QTcpSocket>
#endif

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
    bool lastConnState;

    DeviceTimeouts timeouts;


    quint16 connectionDownCounter;


    struct LastModemState
    {
        //-1 do not change/unk value, 0 - disabled, >0 enabled
        int lastAtmdValue;//last value from the ATMD command
        int atmd2write;//write this value during the configuration, 0xFF is the maximum

        bool directAccess;
        bool uartBlockPrtt;

        bool isCoordinatorConfigReady;
        quint8 apiErrCounter;
//        qint64 msecWhenCoordinatorWasGood;
        QHash<QString,qint64> hashMsecWhenCoordinatorWasGood;

        bool lastCommandWasAtcn;

        bool modemIsOverRS485;

        qint64 lastWriteLen;


        bool isMainConnectionUsed; //it indicates that main/additional connection is used
        bool workWithoutAPI;

        bool lastWasATCNtest;//only if ignore daprrtt

        bool isModemInCommandMode;//true in request2modemOn() , it resets in the write2dev();

        LastModemState() :
            lastAtmdValue(-1), atmd2write(-1), directAccess(false), uartBlockPrtt(false), isCoordinatorConfigReady(false),
            apiErrCounter(0), lastCommandWasAtcn(false), modemIsOverRS485(false), lastWriteLen(-1), isMainConnectionUsed(true),
            workWithoutAPI(false), lastWasATCNtest(false), isModemInCommandMode(false) {}
    } lModemState;

#ifndef DISABLE_TCPCLIENT_MODE
    QTcpSocket *socket;
#endif

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

    int getMaxDataFromModem();

    void setMaxDataFromModem(const int &bytes);

    bool isConnectionWorking();

    bool isConnectionWorking(int waitMsec);
#ifndef DISABLE_TCPCLIENT_MODE

    bool isTcpConnectionWorks(QTcpSocket *socket);
#endif
    QByteArray readDevice();

    QByteArray readDevice(const QByteArray &endSymb, const bool &isQuickMode);

    QByteArray readDeviceQuick(const QByteArray &endSymb, const bool &isclearbufmode);

    bool waitForReadyRead(const int &msecs);
    bool waitForBytesWritten(const int &msecs);

    qint64 bytesAvailable();

    QByteArray clearBufferRead();

    QByteArray readAll();

    qint64 writeATcommand(const QString &atcommand);

    qint64 writeATcommand(const QString &atcommand, const bool &ignoreDaAndPrtt);


    qint64 write2dev(const QByteArray &writeArr);

    qint64 write2dev(const QByteArray &writeArr, const bool &ignoreDaAndPrtt);

#ifndef DISABLE_TCPCLIENT_MODE

    bool openTcpConnection(const bool &workWithoutAPI, const QStringList &hosts, const QList<quint16> &ports);

    bool openTcpConnection(const bool &workWithoutAPI, const QString &host, const quint16 &port);
#endif


#ifdef ENABLE_EXTSUPPORT_OF_IFACES


#ifndef DISABLE_M2M_MODULE
    bool openM2mConnection(const bool &workWithoutAPI, const QVariantHash &oneProfile);
#endif

#ifndef DISABLE_SERIALPORT_MODE
    bool openSerialPort(const bool &workWithoutAPI, const QString &portName, const qint32 &baudRate, const QStringList &uarts,
                        const qint8 &databits, const qint8 &stopbits, const qint8 &parity, const qint8 &flowcontrol);

    bool testModemOnPort(QString defPortName, qint32 baudR, QString &lastError, const qint8 &databits, const qint8 &stopbits, const qint8 &parity, const qint8 &flowcontrol);

    bool findModemOnPort(QString defPortName, qint32 baudR, QStringList uarts, QString &lastError, const qint8 &databits, const qint8 &stopbits, const qint8 &parity, const qint8 &flowcontrol);

    bool openSerialPortExt(const bool &workWithoutAPI, const QString &portName, const qint32 &baudRate, const QSerialPort::DataBits &data, const QSerialPort::StopBits &stopbits, const QSerialPort::Parity &parity,
                           const QSerialPort::FlowControl &flow, const bool &ignoreUartsChecks, QString &lastuarterr);
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
#ifdef ENABLE_VERBOSE_SERVER
    void appendDbgExtData(quint32 sourceType, QString data);
#endif
    void dataFromDevice();

    void onReadWriteOperation(bool isRead);

    void openingTcpConnection();

    void currentOperation(QString mess);

    void onCoordinatorIsBusy(bool busy);

    void onBusyByPriority(bool isBusy);

    void killPeredavatorRequest();
    void resetCoordinatorRequest();//hardware

    void onConnectionClosed();//Connection was closed by not this object

    void onConnectionDown();

    void onSerialPortOpened(QString portName);
    void stopCheckCurrPort();

    void need2closeSerialPort();


    void bigReadData(qint64 len);//readArr.length() > MAX_READ_FROM_UART

    void readyRead();


    void detectedDisconnectedSerialPort();

    void openingAconnection();


    void onApiErrorIncremented(int errcounter);


public slots:
    void setIfaceNameSuffix(QString suffix);

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
    void activateAsyncModeExt(const quint8 &conntype);

    void activateAsyncMode();

    void deactivateAsyncMode();

    void exitCommandModeSimple();

private slots:
    void createDevices();

    void onDeviceDestr();



private:
    qint64 write(const QByteArray &arr);

    void close();

    int maxDataFromModem;

    QString ifacesuffix;

};

#endif // CONN2MODEM_H
