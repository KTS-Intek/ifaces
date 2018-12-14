#include "checkcurrport.h"
#if QT_VERSION >= 0x050000
#include <QSerialPortInfo>
#else
#include <QtAddOnSerialPort/serialportinfo.h>
#endif

#include <QFileInfo>
#include <QDateTime>

//==================================================================================================
CheckCurrPort::CheckCurrPort(QObject *parent) :
    QThread(parent)
{

}
//==================================================================================================
void CheckCurrPort::zapuskalka(QString currPort)
{
    this->currPort=currPort;   
    this->start();
}
//==================================================================================================
void CheckCurrPort::run()
{
//    qDebug()<<QTime::currentTime().toString()<<"startt";
    quint8 counter;
    counter = 1;

    QDateTime dt = QDateTime::currentDateTime();

    if(true){
        QFileInfo fi(currPort);
        if(fi.exists())
            dt = fi.birthTime();
    }

    while(true){
        msleep(1500/counter);
        bool found=false;
    #if QT_VERSION >= 0x050000
        foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()){
    #else
        foreach (const SerialPortInfo &info, SerialPortInfo::availablePorts()){
    #endif
            if(info.portName() == currPort){
                found = true;


                if(!info.isBusy()){
                    emit portDisconnected(1);
                    counter++;
                    if(counter > 2){
                        counter = 2;
                        emit terminateNow();
                    }
                }
                break;
            }
       }
        if(!found){

            QFileInfo fi(currPort);
            if(fi.exists() && fi.birthTime() == dt && dt.isValid())
                continue;

             emit portDisconnected(true);
            counter++;
            if(counter > 2){
                counter = 2;
                emit terminateNow();
            }
        }
    }
//    QTimer timer;
//    connect(&timer, SIGNAL(timeout()), SLOT(onTimerTimeOut()) );
//    timer.start(1500);
//    exec();
}
//==================================================================================================

//void CheckCurrPort::onTimerTimeOut()
//{
//    bool found=false;
//#if QT_VERSION >= 0x050000
//    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()){
//#else
//    foreach (const SerialPortInfo &info, SerialPortInfo::availablePorts()){
//#endif
//        if(info.portName() == currPort){
//            found = true;

//            if(!info.isBusy()){
//                emit portDisconnected(1);
//                counter++;
//                if(counter > 2){
//                    counter = 2;
//                    emit terminateNow();
//                }
//            }
//            break;
//        }
//   }
//    if(!found){
//         emit portDisconnected(true);
//        counter++;
//        if(counter > 2){
//            counter = 2;
//            emit terminateNow();
//        }
//    }
//}
//==================================================================================================
