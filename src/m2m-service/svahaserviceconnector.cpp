#include "svahaserviceconnector.h"
#include "svahasocket.h"
#include <QThread>
#include <QtCore>
#include <QMutexLocker>

//-----------------------------------------------------------------------
SvahaServiceConnector::SvahaServiceConnector(QObject *parent) : QObject(parent)
{
    sessionId = 0;
    needInit = true;
}

SvahaServiceConnector::~SvahaServiceConnector()
{

}
//-----------------------------------------------------------------------
QVariantHash SvahaServiceConnector::defSvahaIps()
{
    QVariantHash h;
    h.insert("kts-m2m.ddns.net", "65000");
    h.insert("kts2-m2m.ddns.net", "65000");
    return h;
}
//-----------------------------------------------------------------------

QString SvahaServiceConnector::errorString() const{ return lastErrorStr;}

QStringList SvahaServiceConnector::getLastSelDevList() const{  return selectList; }

QString SvahaServiceConnector::getLastConnDev() const{   return lastConnDev; }


//-----------------------------------------------------------------------
void SvahaServiceConnector::connect2hostViaSvaha(QVariantHash oneProfile, const int &timeOut, const int &timeOutB)
{
    if(needInit){
        needInit = false;

    }
    useAsynMode = false;    
    quint8 lastS = sessionId;
    sessionId++;
    emit killOldSockets(lastS + 10);

    if(oneProfile.value("useMac", true).toBool())
        lastConnDev = oneProfile.value("cmMAC", false).toBool() ? oneProfile.value("mac").toString() : oneProfile.value("objN").toString();
    else
        lastConnDev = QString("[%1]:%2").arg(oneProfile.value("a").toString()).arg(oneProfile.value("p", 9090).toInt());

    QVariantList lconn;
    if(oneProfile.value("useMac", true).toBool() && oneProfile.value("useCustomSvaha", false).toBool()){
        QVariantHash h = defSvahaIps();
        QList<QString> lh = h.keys();
        for(int i = 0, iMax = lh.size(); i < iMax ; i++){
            oneProfile.insert("a", lh.at(i));
            oneProfile.insert("p", h.value(lh.at(i)).toString());

            QTcpSocket socket;
            socket.connectToHost(oneProfile.value("a").toString(), oneProfile.value("p").toUInt());
            if(socket.waitForConnected(50))
                i += iMax;

            socket.close();

        }
        lconn.append(oneProfile);

    }else{
        lconn.append(oneProfile);
    }
    lastSocketStamp.clear();
    stopAll = false;
    socketKilled = false;
    if(true){
        QMutexLocker locker(&mutex);
        readArr.clear();
    }
    oneConnReady = false;
    socketCounter = 0;
    selectList.clear();

    for(int i = 0, iMax = lconn.size(); i < iMax; i++){
        QVariantHash h = lconn.at(i).toHash();
        if(h.isEmpty())
            continue;

        if(oneConnReady)
            break;

        SvahaSocket *socket = new SvahaSocket(sessionId);
        if(!hashMemoWrite.isEmpty())
            socket->setDoAfterConn(hashMemoWrite.keys().first(), hashMemoWrite.value(hashMemoWrite.keys().first()));

        socket->key2header = this->key2header;

        QThread *t = new QThread;
        socket->moveToThread(t);

        connect(t, SIGNAL(started()), socket, SLOT(startConnection()) );
        connect(socket, SIGNAL(destroyed(QObject*)), t, SLOT(quit()) );
        connect(t, SIGNAL(finished()), t, SLOT(deleteLater()) );

        connect(this, SIGNAL(kickOffThread()), socket, SLOT(stopAllSlot()) , Qt::DirectConnection);
        connect(this, SIGNAL(kickOffThread()), socket, SLOT(onDisconn()) );

        if(h.value("p").toString().isEmpty() || h.value("p").toInt() < 1 || h.value("p").toInt() >= 0xFFFF)
            h.insert("p", 65000);
        socket->setConnectionSett(h.value("a").toString(), h.value("p").toUInt(),
                                  h.value("cmMAC", false).toBool() ? h.value("mac").toString() : h.value("objN").toString(),
                                  h.value("cmMAC", false).toBool(), timeOut, timeOutB, h.value("l").toString(), h.value("pas").toString(), h.value("useMac", true).toBool());

        connect(this, SIGNAL(stopAllSignal()), socket, SLOT(stopAllSlot()), Qt::DirectConnection );
        connect(this, SIGNAL(killOldSockets(quint8)), socket, SLOT(killOldSockets(quint8)) );


        connect(this, SIGNAL(killSocketById(QString)), socket, SLOT(killSocketById(QString)) );
        connect(this, SIGNAL(write2svahaSocket(QByteArray)), socket, SLOT(data2coordiantor(QByteArray)) );

        connect(socket, SIGNAL(dataFromCoordinator(quint8, QString, QByteArray)), this, SLOT(onSvahaSocketRead(quint8, QString,QByteArray)), Qt::DirectConnection );
        connect(socket, SIGNAL(onSocketKilled(quint8, QString)), this, SLOT(onSvahaSocketDisconn(quint8,QString)), Qt::DirectConnection  );
        connect(socket, SIGNAL(add2systemLog(quint8,QString)), this, SLOT(add2systemLogSlot(quint8,QString)) );
        connect(socket, SIGNAL(onSvahaSocketReady(quint8,QString)), this, SLOT(onSvahaSocketReady(quint8,QString)), Qt::DirectConnection );
        connect(socket, SIGNAL(onYouCanSelectDevice(quint8,QStringList)), this, SLOT(onYouCanSelectDevice(quint8,QStringList)) );
        connect(socket, SIGNAL(showMess(quint8,QString)), this, SLOT(showMessS(quint8,QString)) );


        if(!hashMemoWrite.isEmpty()){
            connect(socket, SIGNAL(uploadProgress(quint8,int,QString)), this, SLOT(uploadProgress(quint8,int,QString)) );
            connect(socket, SIGNAL(onCOMMAND2GUI(quint8,quint16,QVariantHash)), this, SLOT(onCOMMAND2GUI(quint8,quint16,QVariantHash)) );
            connect(socket, SIGNAL(appendLedLampListVar(quint8, QVariantList)), this, SLOT(appendLedLampListVar(quint8, QVariantList)));

        }
        socketCounter++;
        t->start();


        if(!h.value("useMac",true).toBool())
            break;
        /*

    void dataFromCoordinator(quint8 sessionId, QByteArray arr);

    void onYouCanSelectDevice(quint8 sessionId, QStringList list);

    void add2systemLog(quint8 sessionId, QString);

//    void startWait4AnswerTimer(int);
//    void stopWait4AnswerTimer();

    void onSvahaSocketReady(quint8 sessionId, QString socketStamp);

    void onSocketKilled(quint8 sessionId, QString socketStamp);


*/

//setConnectionSett(const QString &host, const quint16 &port, const QString &objIdOrMac, const bool &cmMAC, const int &timeOut, const int &timeOutB,
//        const QString &login, const QString &pas)
    }

}
//-----------------------------------------------------------------------
void SvahaServiceConnector::setDoAfterConn(const int &command, const QVariantHash &s_data)
{
    hashMemoWrite.clear();
    if(command < 0)
        return;
    hashMemoWrite.insert(command, s_data);
}
//-----------------------------------------------------------------------
bool SvahaServiceConnector::waitForConnected(const int &msec)
{

    QTime time;
    time.start();
    for(int i = 0, t = 10; !oneConnReady && time.elapsed() < msec && i <= msec; i += t){

        QThread::msleep(t);

        if(socketCounter == 0 || socketKilled || stopAll)
            break;
    }

    if(oneConnReady){
        lastErrorStr = tr("No error)");
    }else{
        if(selectList.isEmpty())
            lastErrorStr = tr("No device");
        else
            lastErrorStr = tr("%1 devices were found").arg(selectList.size());
    }

    return oneConnReady;
}
//-----------------------------------------------------------------------
bool SvahaServiceConnector::waitForReadyRead(const int &msec)
{
    QTime time;
    time.start();
    bool hasData = false;
    for(int i = -1, t = 10; time.elapsed() < msec && i < time.elapsed(); i += t){
        if(!hasData){
            QMutexLocker locker(&mutex);
            hasData = !readArr.isEmpty();
        }
        if(hasData || stopAll)
            break;
        QThread::msleep(quint32(t));
    }

    if(!hasData){
        QMutexLocker locker(&mutex);
        hasData = !readArr.isEmpty();
    }
    return hasData;
}
//-----------------------------------------------------------------------
bool SvahaServiceConnector::waitForBytesWritten(const int &msec)
{
    QThread::msleep(msec/10);
    return true;
}
//-----------------------------------------------------------------------
qint64 SvahaServiceConnector::bytesAvailable()
{
    if(!isOpen())
        return -1L;
    QMutexLocker locker(&mutex);
    return qint64(readArr.length());
}
//-----------------------------------------------------------------------

qint64 SvahaServiceConnector::write(const QByteArray &arr){  emit write2svahaSocket(arr); return arr.length(); }

void SvahaServiceConnector::clear(){ QMutexLocker locker(&mutex); readArr.clear();  }

void SvahaServiceConnector::close(){ stopAllSlot(); }

bool SvahaServiceConnector::isOpen() const{ return (!stopAll && !socketKilled); }

bool SvahaServiceConnector::isReadable() const
{
    return isOpen();
}

void SvahaServiceConnector::setAsyncMode(bool isAsyn){ useAsynMode = isAsyn;   }
//-----------------------------------------------------------------------

QByteArray SvahaServiceConnector::readAll()
{
    QByteArray arr;
    QMutexLocker locker(&mutex);
    arr = readArr;
    readArr.clear();
    return arr;
}

//-----------------------------------------------------------------------
void SvahaServiceConnector::stopAllSlot()
{
    stopAll = true;
    useAsynMode = false;
    emit stopAllSignal();
    emit killOldSockets(sessionId + 10);

}
//-----------------------------------------------------------------------
void SvahaServiceConnector::stopAllSlotDirect()
{
    stopAll = true;
}
//-----------------------------------------------------------------------

void SvahaServiceConnector::onSvahaSocketRead(quint8 sessionId, QString socketStamp, QByteArray arr)
{

    if(socketStamp.isEmpty() || sessionId != this->sessionId)
        return;

    if(true){
        QMutexLocker locker(&mutex);        
        readArr.append(arr);
    }
    if(useAsynMode)
        emit readyRead();
}
//-----------------------------------------------------------------------

void SvahaServiceConnector::onSvahaSocketDisconn(quint8 sessionId, QString socketStamp)
{
    if(socketStamp.isEmpty() || sessionId != this->sessionId)
        return;


    QMutexLocker locker(&mutex4stamp);   
    if(socketStamp == lastSocketStamp && !lastSocketStamp.isEmpty()){
        socketKilled = true;
        emit disconnected();
    }else{
        if(socketCounter > 0)
            socketCounter--;
    }
}

//-----------------------------------------------------------------------
void SvahaServiceConnector::onSvahaSocketReady(quint8 sessionId, QString socketStamp)
{
    if(stopAll){
        emit killSocketById(socketStamp);
        return;
    }
    QMutexLocker locker(&mutex4stamp);
    if(sessionId != this->sessionId )
        return;

    if(oneConnReady)
        return;
    oneConnReady = true;
    lastSocketStamp = socketStamp;
    emit killSocketById(lastSocketStamp);

}
//-----------------------------------------------------------------------
void SvahaServiceConnector::add2systemLogSlot(quint8 sessionId, QString line)
{
    if(sessionId != this->sessionId )
        return;
    emit add2systemLog(line);
}
//-----------------------------------------------------------------------
void SvahaServiceConnector::onYouCanSelectDevice(quint8 sessionId, QStringList list)
{
    if(sessionId != this->sessionId || oneConnReady)
        return;

    selectList.append(list);
    selectList.removeDuplicates();
}
//-----------------------------------------------------------------------
void SvahaServiceConnector::uploadProgress(quint8 sessionId, int val, QString txt)
{
    if(sessionId != this->sessionId  || stopAll)
        return;
    emit uploadProgressS(val, txt);
}
//-----------------------------------------------------------------------
void SvahaServiceConnector::onCOMMAND2GUI(quint8 sessionId, quint16 command, QVariantHash varHash)
{
    if(sessionId != this->sessionId || stopAll)
        return;
    emit onCOMMAND2GUIS(command, varHash);
}

void SvahaServiceConnector::appendLedLampListVar(quint8 sessionId, QVariantList listvar)
{
    if(sessionId != this->sessionId || stopAll)
        return;
    emit appendLedLampListVar(listvar);
}
//-----------------------------------------------------------------------
void SvahaServiceConnector::startWait4connInThread()
{

    connect(this, SIGNAL(destroyed(QObject*)), this, SIGNAL(kickOffThread()) );
    if(!waitForConnected(43000)){
        if(stopAll){
            emit onConnectionWorks(false);
            close();            
            return;
        }
        if(getLastSelDevList().isEmpty()){
            emit showMess(tr("Couldn't connect to the device. %1. Error: %2.").arg(getLastConnDev()).arg(errorString()));
            emit add2systemLog(tr("Couldn't connect to the device. %1. Error: %2.").arg(getLastConnDev()).arg(errorString()));

        }else{
            emit showMess(tr("Couldn't connect to the device. %1. Error: %2.").arg(getLastConnDev()).arg(errorString()));
            emit add2systemLog(tr("Couldn't connect to the device. %1. Error: %2.").arg(getLastConnDev()).arg(errorString()));

        }
        emit onConnectionWorks(false);
        close();
        //        deleteLater();
    }else{
        emit onConnectionWorks(true);
    }

}
//-----------------------------------------------------------------------
void SvahaServiceConnector::softDelete()
{
    stopAll = true;
 QTimer::singleShot(55555, this, SLOT(deleteLater()) );
}
//-----------------------------------------------------------------------
void SvahaServiceConnector::showMessS(quint8 sessionId, QString mess)
{
    if(sessionId != this->sessionId )
        return;
    emit showMess(mess);
}

void SvahaServiceConnector::setHeader4map(QString key, QStringList header)
{
    key2header.insert(key, header);
}
//-----------------------------------------------------------------------

