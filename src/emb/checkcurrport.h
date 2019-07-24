#ifndef CHECKCURRPORT_H
#define CHECKCURRPORT_H

#include <QThread>
#include <QTimer>
#if QT_VERSION < 0x050000

#include <QtAddOnSerialPort/serialport.h>
#include <QtAddOnSerialPort/serialport-global.h>

QT_BEGIN_NAMESPACE_SERIALPORT
class SerialPort;
QT_END_NAMESPACE_SERIALPORT

QT_USE_NAMESPACE_SERIALPORT

#endif

class CheckCurrPort : public QThread
{

    Q_OBJECT
public:
    explicit CheckCurrPort(QObject *parent = 0);

signals:
    void portDisconnected(bool);
    void terminateNow();

    void appendMessage(QString message);
    
public slots:
    void zapuskalka(QString currPort);

    void sendMessageWithAtimeStamp(QString message);

    
protected:
    void run();
//    void onTimerTimeOut();

private:
    QString currPort;

};

#endif // CHECKCURRPORT_H
