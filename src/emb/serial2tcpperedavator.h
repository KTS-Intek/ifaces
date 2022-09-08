#ifndef SERIAL2TCPPEREDAVATOR_H
#define SERIAL2TCPPEREDAVATOR_H

#include <QObject>
#include <QTcpSocket>
#ifdef HASSERIALLIB
#include <QSerialPort>
#endif

class Serial2tcpPeredavator : public QObject
{
    Q_OBJECT
public:
    explicit Serial2tcpPeredavator(QObject *parent = nullptr);

signals:
    void onConnectionClosed();

    void onConnectionDown();

    void stopCheckCurrPort();

    void onSerialPortOpened(QString portName);

    void showMessage(QString messageStrr);

    void onConnectionWorks(bool works);

public slots:
    void onThreadStarted();


    void setBlockTimeoutSerial(int msec);

    void setBlockTimeoutTcp(int msec);


    void mReadyReadSerial();

    void mReadyReadTcp();

    void openConnection(QString port, qint32 baudRate, QStringList hosts, int tcpPort);

    void closeConnect();

    void closeSerialPort();

    void closeSerialDirect();

    void onTcpDown();

private:
    bool openSerial(const QString &port, const qint32 &baudRate);

    bool openTcp(const QStringList &hosts, const int &tcpPort);

    QByteArray mReadyReadSerialF();

    QByteArray mReadyReadTcpF();

    QByteArray mReadyReadF(QIODevice *d, const int &timeout);

    int serialTimeout;
    int tcpTimeout;
#ifdef HASSERIALLIB
    QSerialPort *serial;
#endif
    QTcpSocket *socket;

    bool isSerialOpen;
    bool isTcpOpen;

    bool stopAll;


};

#endif // SERIAL2TCPPEREDAVATOR_H
