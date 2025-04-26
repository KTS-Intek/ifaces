#include "checkcurrport.h"
#include <QSerialPortInfo>


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

void CheckCurrPort::sendMessageWithAtimeStamp(QString message)
{
    emit appendMessage(QString("%1 %2").arg(QTime::currentTime().toString("hh:mm:ss.zzz")).arg(message));
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


    sendMessageWithAtimeStamp(tr("Start checking %1").arg(currPort));
    while(true){

        sendMessageWithAtimeStamp(tr("New loop %1").arg(currPort));

        msleep(1500/counter);
        bool found=false;
        foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()){

            if(info.portName() == currPort){
                found = true;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)


                if(!info.isBusy()){
                    sendMessageWithAtimeStamp(tr("!isBusy %1, info.isBusy='%2', description='%3'").arg(currPort).arg(int(info.isBusy())).arg( info.description()));

                    emit portDisconnected(1);
                    counter++;
                    if(counter > 2){
                        sendMessageWithAtimeStamp(tr("!isBusy %1, info.isBusy='%2', description='%3', terminateNow").arg(currPort).arg(int(info.isBusy())).arg( info.description()));


                        counter = 2;
                        emit terminateNow();
                    }
                }
#endif
                break;
            }
       }
        if(!found){
            sendMessageWithAtimeStamp(tr("!found %1").arg(currPort));

            QFileInfo fi(currPort);
            if(fi.exists() && (!dt.isValid() || (fi.birthTime() == dt && dt.isValid())))
                continue;

            sendMessageWithAtimeStamp(tr("!found %1, fi.exists='%2', created='%3'").arg(currPort).arg(int(fi.exists())).arg( fi.birthTime().toString("hh:mm:ss")));

             emit portDisconnected(true);
            counter++;
            if(counter > 2){
                counter = 2;
                sendMessageWithAtimeStamp(tr("!found %1, fi.exists='%2', created='%3', terminateNow").arg(currPort).arg(int(fi.exists())).arg( fi.birthTime().toString("hh:mm:ss")));

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
