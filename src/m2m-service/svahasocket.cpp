#include "svahasocket.h"
#include "moji_defy.h"
#include "matildalimits.h"



#include <QCryptographicHash>
#include <QtCore>
#include <QHostAddress>
#include "matildausertypes.h"

///[!] type-converter
#include "src/m2m-service/serializeddatacalculation.h"

#include "matildamessages.h"


#include "showmesshelper4wdgtdef.h"


SvahaSocket::SvahaSocket(const quint8 &sesionId, const quint16 &m2mDAchannel, const bool &verboseMode, QObject *parent) : QTcpSocket(parent)
{
    this->sessionId = sesionId;
    this->m2mDAchannel = m2mDAchannel;
    this->verboseMode = verboseMode;

}

//------------------------------------------------------------------------------------------
void SvahaSocket::setConnectionSett(const QString &host, const quint16 &port, const QString &objIdOrMac, const bool &cmMAC, const int &timeOut, const int &timeOutB,
                                    const QString &login, const QString &pas, const bool &useMac)
{
    ignoreDisconn = true;
    isActiveClient = false;
    socketSett.host = host;
    socketSett.port = port;
    socketSett.objIdOrMac = objIdOrMac;
    socketSett.cmMAC = cmMAC;
    socketSett.timeOut = timeOut;
    socketSett.timeOutB = timeOutB;
    socketSett.login = login;
    socketSett.password = pas;
    socketSett.useMac = useMac;
    stopAll = false;
    stopAllDirect = false;
    accessLevel = 0;
    noWriteData = true;
}
//------------------------------------------------------------------------------------------
void SvahaSocket::setDoAfterConn(const int &command, const QVariantHash &s_data)
{
    hashMemoWrite.clear();
    hashMemoWrite.insert(command, s_data);
}

//------------------------------------------------------------------------------------------

QString SvahaSocket::getErrMess(const int &code, const QString &commandName)
{
    QString messageStrr;
    switch(code){
    case ERR_DATABASE_CLOSED    : messageStrr = tr("Command: %1. Database is not opened.").arg(commandName); break;
    case ERR_INCORRECT_REQUEST  : messageStrr = tr("Command: %1. Incorrect request.").arg(commandName); break;
    case ERR_INTERNAL_ERROR     : messageStrr = tr("Command: %1. Internal error.").arg(commandName); break;
    case ERR_NO_DATA            : messageStrr = tr("Command: %1. Data is not found.").arg(commandName); break;

    case ERR_MAX_TABLE_COUNT    : messageStrr = tr("Command: %1. The table count limit was reached").arg(commandName); break;
    case ERR_CORRUPTED_DATA     : messageStrr = tr("Command: %1. Data is corrupted").arg(commandName); break;
    case ERR_DUPLICATE_NI       : messageStrr = tr("Command: %1. Duplicating NI").arg(commandName); break;
    case ERR_DUPLICATE_SN       : messageStrr = tr("Command: %1. Duplicating S/N").arg(commandName); break;
    case ERR_DATE_NOT_VALID     : messageStrr = tr("Command: %1. Date is invalid").arg(commandName); break;
    case ERR_COMMAND_NOT_ALLOWED: messageStrr = tr("Command: %1. Command is not allowed").arg(commandName); break;
    case ERR_ACCESS_DENIED      : messageStrr = tr("Command: %1. Access error").arg(commandName); break;
    case ERR_RESOURCE_BUSY      : messageStrr = tr("Command: %1. Resource is busy").arg(commandName); break;
    case ERR_DA_CLOSED          : messageStrr = tr("Direct access was close");  break;
    case ERR_NO_ERROR           : messageStrr = tr("Command: %1. Done)").arg(commandName); break;
    case ERR_NO_ERROR_DLG       : messageStrr = tr("Command: %1. Done)").arg(commandName); break;

    default: messageStrr = tr("Command: %1. Unknown error. Error code: %2").arg(commandName).arg(code); break;
    }
    return messageStrr;

}
//------------------------------------------------------------------------------------------
void SvahaSocket::startConnection()
{
//    qDebug() << "QThread create SvahaServiceConnector " << QTime::currentTime().toString("hh:mm:ss") << QThread::currentThreadId();

    connect(this, SIGNAL(readyRead()), SLOT(mReadyRead()) );
    connect(this, SIGNAL(disconnected()), SLOT(onDisconn()) );

    matildaLogined = false;

    block4activeClient = false;
    iAmDone = false;
    daOpened = false;
    timeGalmo.restart();
    stopAfter = true;
    zombieRetr = 0;

    dataStreamVersion = QDataStream::Qt_DefaultCompiledVersion;

    if(socketSett.useMac){
        isSvahaService = true;
        ignoreThisDisconn = true;
        disconAfterAnswer = true;

        connectToHost(socketSett.host, socketSett.port);
        if(waitForConnected(socketSett.timeOut)){
            socketStamp = QString("%1\t%2\t%3").arg(peerAddress().toString()).arg(socketDescriptor()).arg(QTime::currentTime().toString("hhmmsszzz"));

            lastMacOrObjId = socketSett.objIdOrMac;
            lastUseMacAdd2conn = socketSett.cmMAC;
#if QT_VERSION >= QT_VERSION_CHECK(5, 9, 0)
            loginPasswd.append(QCryptographicHash::hash( QVariant(socketSett.login.simplified().trimmed()).toByteArray(), QCryptographicHash::Keccak_256));
            loginPasswd.append(QCryptographicHash::hash( QVariant(socketSett.password.simplified().trimmed()).toByteArray(), QCryptographicHash::Keccak_256));
#else
            loginPasswd.append(QCryptographicHash::hash( QVariant(socketSett.login.simplified().trimmed()).toByteArray(), QCryptographicHash::Sha3_256));
            loginPasswd.append(QCryptographicHash::hash( QVariant(socketSett.password.simplified().trimmed()).toByteArray(), QCryptographicHash::Sha3_256));
#endif
            loginPasswd.append("");
            disconAfterAnswer = false;

            if(stopAllDirect)
                return;

            if(true){


                //            lastAdd2list = add2list;
                QJsonObject jObj;
                jObj.insert("version", MATILDA_PROTOCOL_VERSION);
                jObj.insert("useId", !socketSett.cmMAC);
                //
                //mode JSON and QDataStream
                jObj.insert("QDS", QString::number(dataStreamVersion));//активація режиму QDataStream
                jObj.insert("cmmprssn", "zlib");
                jObj.insert("cmprssn", "zlib");
                jObj.insert("remote", socketSett.objIdOrMac);
                timeGalmo.restart();
                stopAll = false;

                mWrite2SocketJSON(jObj, COMMAND_CONNECT_ME_2_THIS_ID_OR_MAC, 2);

                return;
            }
        }else{
            emit add2systemLog(sessionId, tr("Couldn't connect to remote server.<br>Error: %1").arg(errorString()));
        }
    }else{
        isSvahaService = false;
        ignoreThisDisconn = false;
        disconAfterAnswer = false;

        lastMacOrObjId = socketSett.objIdOrMac;
        lastUseMacAdd2conn = socketSett.cmMAC;
#if QT_VERSION >= QT_VERSION_CHECK(5, 9, 0)

        loginPasswd.append(QCryptographicHash::hash( QVariant(socketSett.login.simplified().trimmed()).toByteArray(), QCryptographicHash::Keccak_256));
        loginPasswd.append(QCryptographicHash::hash( QVariant(socketSett.password.simplified().trimmed()).toByteArray(), QCryptographicHash::Keccak_256));
#else
        loginPasswd.append(QCryptographicHash::hash( QVariant(socketSett.login.simplified().trimmed()).toByteArray(), QCryptographicHash::Sha3_256));
        loginPasswd.append(QCryptographicHash::hash( QVariant(socketSett.password.simplified().trimmed()).toByteArray(), QCryptographicHash::Sha3_256));
#endif
        loginPasswd.append("");

        timeGalmo.restart();
        stopAll = false;

        connectToHost(socketSett.host, socketSett.port);
        if(waitForConnected(socketSett.timeOut)){
            socketStamp = QString("%1\t%2\t%3").arg(peerAddress().toString()).arg(socketDescriptor()).arg(QTime::currentTime().toString("hhmmsszzz"));
            if(verboseMode)
                qDebug() << "bool isConnected " << block4activeClient;
            block4activeClient = false;

        }else{
            emit add2systemLog(sessionId, tr("Couldn't connect to the temporary service.<br>Error: %1").arg(errorString()));
            onDisconn();
            return;
        }
    }
}

//------------------------------------------------------------------------------------------
void SvahaSocket::stopAllSlot()
{
    stopAllDirect = true;
}
//------------------------------------------------------------------------------------------
void SvahaSocket::killSocketById(QString ignoreThisStamp)
{
    ignoreThisDisconn = false;
    if(ignoreThisStamp == socketStamp && !socketStamp.isEmpty()){
        isActiveClient = true;

    }else{

        stopAllSlot();
        onDisconn();
    }
    if(verboseMode)
        qDebug() << "killSocketById " << isActiveClient << stopAll << ignoreThisStamp << socketStamp;
}
//------------------------------------------------------------------------------------------
void SvahaSocket::data2coordiantor(QByteArray writeArr)
{
    if(verboseMode)
        qDebug() << "data2coordiantor " << daOpened << writeArr << (state() == QAbstractSocket::ConnectedState);
    if(state() == QAbstractSocket::ConnectedState && daOpened){
        if(noWriteData && writeArr != "ATCN\r\n")
            noWriteData = false;
        mWriteToSocket(writeArr, COMMAND_WRITE_DA_DATA_2_COORDINATOR);
    }
}
//------------------------------------------------------------------------------------------

void SvahaSocket::onDaOpened(bool isDaOpen){  daOpened = isDaOpen;}

//------------------------------------------------------------------------------------------
void SvahaSocket::killOldSockets(quint8 sessionId)
{
    if(sessionId != this->sessionId){
        close();
        deleteLater();
    }
}
//------------------------------------------------------------------------------------------
void SvahaSocket::setHeaders4map(QStringList keys, QVariantList headers)
{
    for(int i = 0, imax = keys.size(); i < imax; i++)
        setHeader4map(keys.at(i), headers.at(i).toStringList());
}
//------------------------------------------------------------------------------------------
void SvahaSocket::setHeader4map(QString key, QStringList header)
{
    key2header.insert(key, header);
}

//------------------------------------------------------------------------------------------
void SvahaSocket::onSocketReady()
{
    if(stopAllDirect){
        onDisconn();
        return;
    }
    QTimer::singleShot(1, this, SLOT(openDirectAccess()) );

}
//------------------------------------------------------------------------------------------
void SvahaSocket::decodeReadData(const QVariant &dataVar, const quint16 &command)
{
    if(stopAllDirect){
        onDisconn();
        return;
    }
    if(command == COMMAND_I_AM_A_ZOMBIE){
        if(verboseMode)
            qDebug() << "zombie echo" << peerAddress();
        zombieRetr = 0;
        return;
    }
    if(!dataVar.isValid()){
        if(verboseMode)
            qDebug() << "!dataVar.isValid";
        return;
    }
    zombieRetr = 0;


    if(stopAll && command != COMMAND_AUTHORIZE && command != COMMAND_I_AM_A_ZOMBIE && command != COMMAND_I_NEED_MORE_TIME && command != COMMAND_WRITE_DA_OPEN_CLOSE
            && command != COMMAND_READ_DA_DATA_FROM_COORDINATOR && command != COMMAND_GET_BACKUP_FILE && command != COMMAND_PUSH_BACKUP_FILE_AND_APPLY && command != COMMAND_READ_METER_LIST_FRAMED
            && command != COMMAND_WRITE_METER_LIST_FRAMED && command != COMMAND_WRITE_METER_LIST_ONE_PART){
        if(disconAfterAnswer)
            onDisconn();
        return;
    }

    bool rezIsGood = true;
    int messCode = MESSAGE_NO_MESSAGE;

    switch(command){

    case COMMAND_AUTHORIZE:{
        if(verboseMode)
            qDebug() << "access = " << dataVar.toHash();

        QVariantHash h = dataVar.toHash();

        if(!(h.value("d").toInt() == DEV_POLL || h.value("d").toInt() == DEV_GATE ||
             h.value("d").toInt() == DEV_POLL_EMULATOR_L1 || h.value("d").toInt() == DEV_POLL_EMULATOR_L2 ||
             h.value("d").toInt() == DEV_POLL_EMULATOR_L3) ||
                //since protocol v13
                h.value("d").toInt() == DEV_M2M_STAFF_SRVS || h.value("d").toInt() == DEV_GATE_V2){
            emit add2systemLog( sessionId, tr("Unknown device type: %1").arg(h.value("d").toInt()));
            onDisconn();
            return;
        }

        accessLevel = h.value("a").toUInt();
        switch(h.value("a").toUInt()){
        case MTD_USER_ADMIN:{
            emit add2systemLog( sessionId, tr("Hello)"));
            onSocketReady();
            break;}

        case MTD_USER_OPER:{
            emit add2systemLog( sessionId, tr("Hello)"));
            onSocketReady();
            break;}

        case MTD_USER_GUEST:{
            if(hashMemoWrite.contains(COMMAND_READ_METER_LIST_FRAMED)){
                emit add2systemLog( sessionId, tr("Hello)"));
                onSocketReady();
            }else{
                emit onSvahaSocketReady(sessionId, socketStamp);
                emit add2systemLog( sessionId, tr("Access error("));
                onDisconn();
            }
            break; }
        default: {
            emit onSvahaSocketReady(sessionId, socketStamp);
            emit add2systemLog( sessionId, tr("Access error("));
            emit showMessage(sessionId, tr("Access error("));
            onDisconn();
            break;}
        }
        return;}

    case COMMAND_WRITE_DA_OPEN_CLOSE:{
        //прямий доступ закрито
        daOpened = false;
        onDisconn();
        return;}

    case COMMAND_READ_DA_DATA_FROM_COORDINATOR:{
        emit dataFromCoordinator(sessionId, socketStamp, dataVar.toByteArray());
        return;}


    case COMMAND_I_NEED_MORE_TIME:{
        stopAfter = false;
        return;}

    case COMMAND_I_AM_A_ZOMBIE:{
        if(verboseMode)
            qDebug() << "zombie echo" << peerAddress();
//        mWriteToSocket("", COMMAND_I_AM_A_ZOMBIE);
        return;}

    case COMMAND_ACCESS_DENIED:{
        if(verboseMode)
            qDebug() << "access denied = " << dataVar.toBool();
        if(!dataVar.toBool()){
            emit add2systemLog( sessionId, tr("Access was denied."));
        }
        return;}

    case COMMAND_ERROR_CODE:{
        bool rezIsGood;
        onCOMMAND_ERROR_CODE(dataVar.toHash(), rezIsGood);

        break;}

    case COMMAND_ERROR_CODE_EXT:{
        bool rezIsGood;
        onCOMMAND_ERROR_CODE_EXT(dataVar.toHash(), rezIsGood);

        break;}

    case COMMAND_READ_METER_LIST_FRAMED     :{ messCode = onCOMMAND_READ_METER_LIST_FRAMED(dataVar.toHash(), rezIsGood)     ; break;}

    case COMMAND_WRITE_METER_LIST_FRAMED    :{ messCode = onCOMMAND_WRITE_METER_LIST_FRAMED(dataVar.toHash(), rezIsGood)    ; break;}


    case COMMAND_READ_LEDLAMPLIST_FRAMED     :{ messCode = onCOMMAND_READ_LEDLAMPLIST_FRAMED(dataVar.toHash(), rezIsGood)     ; break;}

    case COMMAND_WRITE_LEDLAMPLIST_FRAMED    :{ messCode = onCOMMAND_WRITE_LEDLAMPLIST_FRAMED(dataVar.toHash(), rezIsGood)    ; break;}

    default: onDisconn(); break;
    }

    if(disconAfterAnswer)
        onDisconn();
    else{

        if(!hashMemoWrite.isEmpty()){
            if(rezIsGood && messCode == MESSAGE_OPERATION_IN_PROGRESS){
                return;
            }

            if(!(rezIsGood && (command == COMMAND_ERROR_CODE || command == COMMAND_ERROR_CODE_EXT))){

                if(messCode != MESSAGE_NO_MESSAGE && messCode != MESSAGE_OPERATION_IN_PROGRESS)
                    emit add2systemLog(sessionId, (MatildaMessages::messFromCode(messCode)));
                emit add2systemLog(sessionId, MatildaMessages::addRez2endOfLine(MatildaMessages::name4command(command), rezIsGood));
            }
            onDisconn();
        }
    }
}
//------------------------------------------------------------------------------------------
void SvahaSocket::decodeReadDataJSON(const QByteArray &dataArr)
{
    QJsonParseError jErr;
    QJsonDocument jDoc = QJsonDocument::fromJson( dataArr, &jErr);

    QVariantHash hash = jDoc.object().toVariantHash();
    if(verboseMode)
        qDebug() << hash;

    quint16 command = hash.take("cmd").toUInt();

    if(command == COMMAND_COMPRESSED_PACKET){
        if(messHshIsValid(jDoc.object(), dataArr)){
            jDoc = QJsonDocument::fromJson( qUncompress( QByteArray::fromBase64( hash.value("zlib").toByteArray() )  ), &jErr);//qUncompress( <quint32 not compressed data len><comprssed data> )
            hash = hash = jDoc.object().toVariantHash();

            command = hash.take("cmd").toUInt();
        }else{
            onDisconn();
            return;
        }
    }

    if(verboseMode){
        qDebug() << "decodeReadData" << command;
        qDebug()  << jDoc.object();
    }

    if(!messHshIsValid(jDoc.object(), dataArr)){
        onDisconn();
        return;
    }
    zombieRetr = 0;

    switch(command){
    case COMMAND_ZULU:{

        if(!isSvahaService && hash.value("name").toString() == "Matilda" && hash.value("version").toInt() > 0 && hash.value("version").toInt() >= MATILDA_PROTOCOL_VERSION_V1 && QDateTime::fromString(hash.value("UTC").toString(), "yyyy-MM-dd hh:mm:ss").isValid()){
            if(!hash.value("err").toString().isEmpty()){
                if(verboseMode)
                    qDebug() << hash.value("err").toString();
                emit add2systemLog( sessionId, tr("Authorization failed.<br>Your IP in block list.<br><br>Message from device:<br>%1").arg(hash.value("message").toString()));
                onDisconn();
                return;
            }else{

                if(hash.value("QDS").toInt() >= QDataStream::Qt_4_8 && hash.value("QDS").toInt() <= QDataStream::Qt_DefaultCompiledVersion){
                    emit add2systemLog( sessionId, tr("Send authorization request"));

                    matildaLogined = true;
                    dataStreamVersion = hash.value("QDS").toInt();

                    QJsonObject jObj;

                    jObj.insert("version", hash.value("version").toInt());
#if QT_VERSION >= QT_VERSION_CHECK(5, 9, 0)
                    jObj.insert("hsh", QString(QCryptographicHash::hash(loginPasswd.at(0) + "\n" + dataArr + "\n" + loginPasswd.at(1), QCryptographicHash::Keccak_256).toBase64()));
#else
                    jObj.insert("hsh", QString(QCryptographicHash::hash(loginPasswd.at(0) + "\n" + dataArr + "\n" + loginPasswd.at(1), QCryptographicHash::Sha3_256).toBase64()));
#endif
//
                    //mode JSON and QDataStream
                    jObj.insert("QDS", QString::number(dataStreamVersion));//активація режиму QDataStream
                    jObj.insert("cmmprssn", "zlib");

                    jObj.insert("plg", false);

                    timeGalmo.restart();
                    stopAll = false;

                    QTimer *zombieTmr = new QTimer(this);
                    zombieTmr->setInterval(15 * 1000);
                    connect(zombieTmr, SIGNAL(timeout()), this, SLOT(onZombeTo()) );
                    connect(this, SIGNAL(connected()), zombieTmr, SLOT(start()) );
                    connect(this, SIGNAL(readyRead()), zombieTmr, SLOT(start()) );
                    connect(this, SIGNAL(disconnected()), zombieTmr, SLOT(stop()) );

                    mWrite2SocketJSON(jObj, COMMAND_AUTHORIZE, 2);
                    return;
                }

            }
            emit add2systemLog( sessionId, tr("Authorization failed("));

        }

        onDisconn();
        break;}

    case COMMAND_I_AM_A_ZOMBIE:{
        if(verboseMode)
            qDebug() << "zombie echo" << peerAddress();
//        mWriteToSocket("", COMMAND_I_AM_A_ZOMBIE);
        return;}


    case COMMAND_CONNECT_ME_2_THIS_ID_OR_MAC:{

        if(hash.contains("sIp") && hash.contains("sP")){
            block4activeClient = true;
            if(verboseMode)
                qDebug() << "close old connection " ;

            close();

            isSvahaService = false;
            connectToHost(hash.value("sIp").toString(), hash.value("sP").toUInt());

            if(waitForConnected(socketSett.timeOut)){
                if(verboseMode)
                    qDebug() << "bool isConnected " << block4activeClient;
                block4activeClient = false;

            }else{
                emit add2systemLog(sessionId, tr("Couldn't connect to the temporary service.<br>Error: %1").arg(errorString()));
                onDisconn();
                return;
            }
        }else{
            QVariantList l = hash.value("l").toList();
            QStringList list;
            for(int i = 0, iMax = l.size(); i < iMax; i++)
                list.append(l.at(i).toString());

            emit add2systemLog( sessionId, tr("%1 devices were found").arg(list.size()));

            emit onYouCanSelectDevice(sessionId, list);

            onDisconn();
        }
        break;}

//    case COMMAND_ERROR_CODE:{ emit data2gui(command, hash);  close(); break;}
//    case COMMAND_ERROR_CODE_EXT:{ emit data2gui(command, hash); close(); break;}
    }
}
//------------------------------------------------------------------------------------------
void SvahaSocket::mWriteToSocket(const QVariant s_data, const quint16 s_command)
{


    if((s_command >= COMMAND_WRITE_FIRST && accessLevel > 1) || (s_command >= COMMAND_WRITE_FIRST_4_OPERATOR && accessLevel > 2)){
        emit add2systemLog( sessionId, tr("Not allowed("));
        return;
    }

    if(!isOpen()){
        if(verboseMode)
            qDebug() << "matildaclient::mWriteToSocket connIsDown " << QTime::currentTime().toString("hh:mm:ss.zzz") << isOpen() << state();
        return;
    }



    timeGalmo.restart();
    stopAll = false;

    if(s_command == COMMAND_ERROR_CODE || s_command == COMMAND_ERROR_CODE_EXT){
        if(verboseMode)
            qDebug() << "block write " << s_command << s_data;
        return;
    }

    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(dataStreamVersion); //Qt_4_0);
    out << (quint32)0;
    out << s_command << s_data;
    out.device()->seek(0);

//     out << (quint32)(block.size() - sizeof(quint32));
    quint32 blSize = block.size();



    if(blSize < SETT_MAX_UNCOMPRSS_PACkET_SIZE || s_command == COMMAND_WRITE_UPGRADE || s_command == COMMAND_COMPRESSED_STREAM ){
        out << (quint32)(blSize - sizeof(quint32));

    }else{
        block.clear();

        QByteArray blockUncompr;
        QDataStream outUncompr(&blockUncompr, QIODevice::WriteOnly);
        outUncompr.setVersion(dataStreamVersion); //Qt_4_0);
        outUncompr << s_command << s_data;

        QDataStream out(&block, QIODevice::WriteOnly);
        out.setVersion(dataStreamVersion); //Qt_4_0);
        out << (quint32)0;
        out << (quint16)COMMAND_COMPRESSED_PACKET << QVariant(qCompress(blockUncompr,9));
        out.device()->seek(0);
        if(verboseMode)
            qDebug() << "blSize " << blSize;
        quint32 blSize2 = block.size();
        if(verboseMode)
            qDebug() << "blSize2 " << blSize2;
        out << (quint32)(blSize2 - sizeof(quint32));
    }

    qint64 len = write(block);
    if(verboseMode)
        qDebug() << "write " << QTime::currentTime().toString("hh:mm:ss.zzz") << len << s_command;

    waitForBytesWritten(50);

}
//------------------------------------------------------------------------------------------
void SvahaSocket::onDisconn()
{
    if(verboseMode)
        qDebug() << "onDisconn" << ignoreThisDisconn << block4activeClient << socketStamp << sessionId;

    if(stopAllDirect){
        block4activeClient = false;
        ignoreThisDisconn = false;
        stopAll = true;
    }

    if(block4activeClient){
        block4activeClient = false;
        return;
    }

    if(ignoreThisDisconn)
        return;

    if(!iAmDone){
        iAmDone = true;
        emit add2systemLog( sessionId, tr("Connection was closed."));
    }

    iAmDone = true;
    emit onSocketKilled(sessionId, socketStamp);
    close();
    deleteLater();
}
//------------------------------------------------------------------------------------------
void SvahaSocket::mReadyRead()
{
    disconnect(this, SIGNAL(readyRead()), this, SLOT(mReadyRead()) );
    mReadyReadF();
    connect(this, SIGNAL(readyRead()), this, SLOT(mReadyRead()) );

}
//------------------------------------------------------------------------------------------
void SvahaSocket::openDirectAccess()
{
    if(stopAllDirect){
        onDisconn();
        return;
    }

    if(hashMemoWrite.isEmpty()){
    //відкриваю прямий доступ

        //m2mDAchannel
        // 0 - COMMAND_DA_CLOSE close
        // 1 - COMMAND_DA_OPEN - main channel
        //

        if(m2mDAchannel < 1)
            m2mDAchannel = 1;

        mWriteToSocket(QVariant(int(m2mDAchannel)), COMMAND_WRITE_DA_OPEN_CLOSE);
    }else{
        int command = hashMemoWrite.keys().first();
        QVariantHash writeHash = hashMemoWrite.value(command);

        if(writeHash.isEmpty()){
            if(verboseMode)
                qDebug() << "command not valid SvahaSocket::openDirectAccess( " << command << hashMemoWrite.value(command);
            deleteLater();
            return;
        }
        if(!daOpened){
            daOpened = true;
             emit onSvahaSocketReady(sessionId, socketStamp);
        }
        switch(command){

        case COMMAND_READ_LEDLAMPLIST_FRAMED:  break;
        case COMMAND_WRITE_LEDLAMPLIST_FRAMED:{ bool ok; onCOMMAND_WRITE_LEDLAMPLIST_FRAMED(QVariantHash(), ok, true); return;}

        case COMMAND_READ_METER_LIST_FRAMED     : break;
        case COMMAND_WRITE_METER_LIST_ONE_PART  : break;
        case COMMAND_WRITE_METER_LIST_FRAMED    :{ bool ok; onCOMMAND_WRITE_METER_LIST_FRAMED(QVariantHash(), ok, true); return;}
        default:{ if(verboseMode) qDebug() << "unknown command SvahaSocket::openDirectAccess( " << command << hashMemoWrite.keys(); deleteLater(); break;}
        }

        mWriteToSocket(writeHash, quint16 (command));
    }
}
//------------------------------------------------------------------------------------------
void SvahaSocket::onZombeTo()
{
    if(noWriteData)
        return;
    zombieRetr++;
    if(zombieRetr > 2){
        onDisconn();
        return;
    }
    if(matildaLogined)
        mWriteToSocket(QVariant(), COMMAND_I_AM_A_ZOMBIE);
    else
        mWrite2SocketJSON(QJsonObject(), COMMAND_I_AM_A_ZOMBIE, 2);
}
//------------------------------------------------------------------------------------------
void SvahaSocket::mReadyReadF()
{
    if(stopAllDirect){
        onDisconn();
        return;
    }
    if(!matildaLogined){
        QByteArray readarr = readAll();
        QElapsedTimer time;
        time.start();
        int razivDuzkaL = readarr.count('{'), razivDuzkaR = readarr.count('}');

        QElapsedTimer timeG;
        timeG.start();

        while(( razivDuzkaR < razivDuzkaL) && readarr.size() < MAX_PACKET_LEN && time.elapsed() < socketSett.timeOutB && timeG.elapsed() < socketSett.timeOut && razivDuzkaL > 0){
            if(waitForReadyRead(socketSett.timeOutB)){
                time.restart();
                readarr.append(readAll());
                razivDuzkaL = readarr.count('{');
                razivDuzkaR = readarr.count('}');
    //            qDebug() << "razivDuzkaL razivDuzkaR " << razivDuzkaL << razivDuzkaR;
            }
        }

        if(razivDuzkaL != razivDuzkaR || razivDuzkaL < 1){
            if(verboseMode)
                qDebug()<< "readServer:"<< readarr;
            return ;
        }else{
            int duzkaIndx = readarr.indexOf("}");
            int lastIndx = 0;

            int len = readarr.length();
            while(duzkaIndx > 1 && lastIndx < len){
                decodeReadDataJSON(readarr.mid(lastIndx, duzkaIndx + 1));
                duzkaIndx = readarr.indexOf("}", lastIndx);
                lastIndx = duzkaIndx + 1;

            }
        }
        readarr = readAll();
        if(verboseMode)
            qDebug()<< "readServer2:"<< readarr;
        return;


    }

    while(timeGalmo.elapsed() < 35){
        this->waitForBytesWritten(7);
        this->waitForReadyRead(7);
    }

    quint32 blockSize;
    QDataStream inStrm(this);

    inStrm.setVersion(dataStreamVersion);

    if(this->bytesAvailable() < (int)sizeof(quint32))
        return ;

    inStrm >> blockSize;

    QElapsedTimer timeG;
    timeG.start();
    while(bytesAvailable() < blockSize && timeG.elapsed() < socketSett.timeOut){

        waitForReadyRead(socketSett.timeOutB);

    }
    if(verboseMode)
        qDebug() << "time2read " << bytesAvailable() << blockSize << timeG.elapsed();
    if(bytesAvailable() < blockSize){
//        qDebug()<<"clientConnection->bytesAvailable() != blockSize";

        const QByteArray readArr = readAll();
        if(verboseMode)
            qDebug()<< "readServer:"<< bytesAvailable() << blockSize << readArr.toHex();

        return ;
    }

    stopAfter = true;

    if(bytesAvailable() == blockSize){

        if(verboseMode)
            qDebug() << "good byte " << bytesAvailable() << blockSize ;
        quint16 serverCommand;
        QVariant readVar;

        inStrm >> serverCommand >> readVar;
        if(serverCommand == COMMAND_COMPRESSED_PACKET)
            decodeReadData(uncompressRead(readVar.toByteArray(), serverCommand, blockSize) , serverCommand);
        else{
            decodeReadData(readVar, serverCommand);
        }
        stopAfter = false;
    }else{
        if(!inStrm.atEnd()){
            quint16 serverCommand;
            QVariant readVar;
            if(verboseMode)
                qDebug() << "not good byte " << bytesAvailable() << blockSize ;

            inStrm >> serverCommand >> readVar;
            if(serverCommand == COMMAND_COMPRESSED_PACKET)
                decodeReadData(uncompressRead(readVar.toByteArray(), serverCommand, bytesAvailable()) , serverCommand);
            else{
                decodeReadData(readVar, serverCommand);
            }
            stopAfter = false;
            QTimer::singleShot(1, this, SLOT(mReadyRead()) );
        }
    }
    stopAll = stopAfter;
}
//------------------------------------------------------------------------------------------
int SvahaSocket::onCOMMAND_ERROR_CODE(const QVariantHash &h, bool &rezIsGood)
{
    rezIsGood = !h.isEmpty();
    if(!rezIsGood)
        return MESSAGE_CORRUPTED_DATA;

//    QString commandName;// = ShowMessageHelperCore::name4command(h.value("lcmd").toInt());

    const QString messageStrr = getErrMess(h.value("e").toInt(), QString::number(h.value("lcmd").toInt()));

    if(h.value("lcmd").toInt() == COMMAND_WRITE_DA_OPEN_CLOSE){
        if(h.value("e").toInt() == ERR_NO_ERROR || h.value("e").toInt() == ERR_NO_ERROR_DLG){
            if(stopAllDirect)
                onDisconn();
            else
                emit onSvahaSocketReady(sessionId, socketStamp);
            daOpened = true;
            return MESSAGE_NO_MESSAGE;
        }
        rezIsGood = false;
    }

    switch(h.value("lcmd").toInt()){
    case COMMAND_READ_METER_LIST_FRAMED   :
    case COMMAND_WRITE_METER_LIST_FRAMED  :
    case COMMAND_WRITE_METER_LIST_ONE_PART: emit showMessage(sessionId, messageStrr);emit add2systemLog(sessionId, messageStrr); break;
    }

    if((h.value("e").toInt() == ERR_NO_ERROR || h.value("e").toInt() == ERR_NO_DATA || h.value("e").toInt() == ERR_NO_ERROR_DLG)
            && h.value("lcmd").toInt() == COMMAND_WRITE_METER_LIST_ONE_PART){
        emit uploadProgress(sessionId, 100, messageStrr);
    }
    stopAllSlot();
    onDisconn();
    QTimer::singleShot(555, this, SLOT(deleteLater()) );
    return MESSAGE_NO_MESSAGE;
}
//------------------------------------------------------------------------------------------
int SvahaSocket::onCOMMAND_ERROR_CODE_EXT(const QVariantHash &h, bool &rezIsGood)
{
    rezIsGood = !h.isEmpty();
    if(!rezIsGood)
        return MESSAGE_CORRUPTED_DATA;

    bool addRedLine = true;

    QString messageStrr = getErrMess(h.value("e").toInt(), QString::number(h.value("lcmd").toInt()));

    if(h.value("lcmd").toInt() == COMMAND_WRITE_DA_OPEN_CLOSE){
        if(h.value("e").toInt() == ERR_NO_ERROR || h.value("e").toInt() == ERR_NO_ERROR_DLG){
            if(stopAllDirect)
                onDisconn();
            else
                emit onSvahaSocketReady(sessionId, socketStamp);
            daOpened = true;
            return MESSAGE_NO_MESSAGE;
        }
        rezIsGood = false;
    }

    if(addRedLine){
//        messageStrr = ShowMessageHelperCore::addWithFontColorRed(messageStrr);
        if(!h.value("em").toString().isEmpty())
            messageStrr.append(tr(". Device: %1").arg(h.value("em").toString()) + "(");
    }

    if(!h.value("em").toString().isEmpty())
        messageStrr.append(tr(". Device: %1").arg(h.value("em").toString()));

    if(addRedLine && !h.value("em").toString().isEmpty())
        messageStrr.append("(");

    switch(h.value("lcmd").toInt()){
    case COMMAND_READ_METER_LIST_FRAMED   :
    case COMMAND_WRITE_METER_LIST_FRAMED  :
    case COMMAND_WRITE_METER_LIST_ONE_PART: emit showMessage(sessionId, messageStrr);emit add2systemLog(sessionId, messageStrr); break;
    }

    if((h.value("e").toInt() == ERR_NO_ERROR || h.value("e").toInt() == ERR_NO_DATA || h.value("e").toInt() == ERR_NO_ERROR_DLG)
            && h.value("lcmd").toInt() == COMMAND_WRITE_METER_LIST_ONE_PART){
        emit uploadProgress(sessionId, 100, messageStrr);
    }

    stopAllSlot();
    onDisconn();
    QTimer::singleShot(555, this, SLOT(deleteLater()) );

    return MESSAGE_NO_MESSAGE;
}
//------------------------------------------------------------------------------------------
int SvahaSocket::onCOMMAND_READ_METER_LIST_FRAMED(const QVariantHash &h, bool &rezIsGood)
{


    rezIsGood = !h.isEmpty();
    if(!rezIsGood)
        return MESSAGE_CORRUPTED_DATA;

    const bool ignoreEmptyList = h.contains("t");
    QStringList lHeader;

    if(ignoreEmptyList){
//#ifdef IS4ANDROID
//        lHeader = tr("Model,Serial Number,NI,Memo,Password,On/Off,Physical values,Tariff Count,Coordinates,Meter Version,Powering,Transformer,Time").split(",");
        lHeader = key2header.value("getColNamesMeterList");// TableHeaders::getColNamesMeterList().split(",");
        totalTables = h.value("t").toInt();
        doneTables = 0;
    }

    const QVariantList l = h.value("m").toList();
    const int lastIndx = h.value("i").toInt();

    if((l.isEmpty() || !h.contains("i")) && !ignoreEmptyList){
        rezIsGood = false;
        return MESSAGE_CORRUPTED_DATA;
    }

    const QStringList k = key2header.value("getKeysMeterList");// MatildaDataKeys::getKeysMeterList().split(',');//"model,SN,NI,memo,passwd,on,politic,trff,crdnts,vrsn, grp, pwrn,tc,dta"
    QVariantList meters;
    const int iMax = l.size();

    int counterF = 0;
    if(verboseMode)
        qDebug() << "meterSvahaKeys= " << k;
    for(int i = 0, colMax = k.size(); i < iMax; i++){
        const QVariantHash h = l.at(i).toHash();

        QStringList m;
        for(int col = 0; col < colMax; col++)
            m.append(h.value(k.at(col)).toString());

        m.insert(10, "");//grp
        if(verboseMode)
            qDebug() << "meterSvaha=" << i << m;
        meters.append(m);
        counterF++;

    }


    if(!lHeader.isEmpty() || !meters.isEmpty()){
        QVariantHash hash;
        hash.insert("lHeader", lHeader);
        hash.insert("meters", meters);
        hash.insert("hasHeader", ignoreEmptyList);
        emit onCOMMAND2GUI(sessionId, COMMAND_READ_METER_LIST_FRAMED, hash);
    }


    if(lastIndx >= 0 && totalTables > 0){

        blockDone = 0;
        doneTables += iMax;
            emit uploadProgress(sessionId, ((doneTables * 100) / totalTables), tr("Total count: %1 meters, Downloaded: %2 meters")
                             .arg(totalTables).arg(doneTables) );
        QVariantHash hash;
        hash.insert("i", lastIndx);
        mWriteToSocket(hash, COMMAND_READ_METER_LIST_FRAMED);
        return MESSAGE_OPERATION_IN_PROGRESS;

    }else{ //-1 read done
        doneTables = totalTables;
         emit uploadProgress(sessionId, ((doneTables * 100) / totalTables), tr("Total count: %1 meters, Downloaded: %2 meters")
                             .arg(totalTables).arg(totalTables) );
        QTimer::singleShot(111, this, SLOT(deleteLater()) );
    }



    return MESSAGE_NO_MESSAGE;
}
//------------------------------------------------------------------------------------------
int SvahaSocket::onCOMMAND_WRITE_METER_LIST_FRAMED(const QVariantHash &h, bool &rezIsGood, const bool firstWrite)
{
    if(firstWrite){
        rezIsGood = true;
    }else{
        rezIsGood = !h.isEmpty();
        if(!rezIsGood)
            return MESSAGE_CORRUPTED_DATA;
    }

    const QVariantList listCache = hashMemoWrite.value(COMMAND_WRITE_METER_LIST_FRAMED).value("lv").toList();

    const int rowCount = listCache.size();

    QVariantList l;
    QStringList k = key2header.value("getKeysMeterList");// MatildaDataKeys::getKeysMeterList().split(' ', QString::SkipEmptyPart);

    int lastIndx = h.value("i").toInt();
    if(firstWrite){
        lastIndx = -1;
    }else{
        if(lastIndx >= rowCount || lastIndx < 0){
            rezIsGood = false;
            return MESSAGE_CORRUPTED_DATA;
        }
    }

    QVariantHash hash;

    qint64 dataLen = 50;
    qint64 maxDataLen = 11000;
    int newLastIndx = -1;

    for(int j = 0, row = lastIndx + 1, colMax = k.size(); row < rowCount; row++, j++){

        QVariantHash hh = listCache.at(row).toHash();
        for(int col = 0; col < colMax; col++){
            if(!hh.contains(k.at(col)))
                hh.insert(k.at(col), QString(""));
        }

        dataLen += SerializedDataCalculation::chkMessageSize(hh);
        if(j > 0 && dataLen >= maxDataLen){
            if(row < rowCount)
                newLastIndx = row - 1;
            break;
        }
        l.append(hh);
    }
    if(lastIndx < 0)
        hash.insert("t", rowCount);
    hash.insert("m", l);
    hash.insert("i", newLastIndx);

//        emit uploadProgress( ((newLastIndx * 100) / rowCount),
    if(newLastIndx < 0){
        emit uploadProgress(sessionId, 100, tr("Total count: %1 Meters, Uploaded: %2 Meters")
                         .arg(rowCount).arg(rowCount) );
        QTimer::singleShot(55555, this, SLOT(deleteLater()) );
    }else
        emit uploadProgress(sessionId, ((newLastIndx * 100) / rowCount), tr("Total count: %1 Meters, Uploaded: %2 Meters")
                         .arg(rowCount).arg(newLastIndx) );

    if(firstWrite)
        lastWriteVarZero = hash;
    mWriteToSocket(hash, COMMAND_WRITE_METER_LIST_FRAMED);

    return MESSAGE_OPERATION_IN_PROGRESS;
}

int SvahaSocket::onCOMMAND_READ_LEDLAMPLIST_FRAMED(const QVariantHash &h, bool &rezIsGood)
{
    //Model,NI,Group,Last Exchange,Power,Start Power,NA Power,Tna,Coordinate,Poll On/Off,Street,Memo
    return readFramedList(h, rezIsGood, key2header.value("getColNamesLedLamp"), key2header.value("getKeysLedLampWriteOnly"), // TableHeaders::getColNamesLedLampV2(), FireflyGlobal::getKeysLedLampWriteOnly(),
                          COMMAND_READ_LEDLAMPLIST_FRAMED, tr("lamps"));

}

int SvahaSocket::onCOMMAND_WRITE_LEDLAMPLIST_FRAMED(const QVariantHash &h, bool &rezIsGood, const bool firstWrite)
{
    if(firstWrite){
        rezIsGood = true;
    }else{
        rezIsGood = !h.isEmpty();
        if(!rezIsGood)
            return MESSAGE_CORRUPTED_DATA;
    }

    QVariantList listCache = hashMemoWrite.value(COMMAND_WRITE_LEDLAMPLIST_FRAMED).value("lv").toList();

    int rowCount = listCache.size();

    QVariantList l;
    QStringList k = key2header.value("getKeysLedLampWriteOnlyList");// FireflyGlobal::getKeysLedLampWriteOnlyList();

    int lastIndx = h.value("i").toInt();
    if(firstWrite){
        lastIndx = -1;
    }else{
        if(lastIndx >= rowCount || lastIndx < 0){
            rezIsGood = false;
            return MESSAGE_CORRUPTED_DATA;
        }
    }

    QVariantHash hash;

    qint64 dataLen = 50;
    qint64 maxDataLen = 11000;
    int newLastIndx = -1;

    for(int j = 0, row = lastIndx + 1, colMax = k.size(); row < rowCount; row++, j++){

        QVariantHash hh = listCache.at(row).toHash();
        for(int col = 0; col < colMax; col++){
            if(!hh.contains(k.at(col)))
                hh.insert(k.at(col), QString(""));
        }

        dataLen += SerializedDataCalculation::chkMessageSize(hh);
        if(j > 0 && dataLen >= maxDataLen){
            if(row < rowCount)
                newLastIndx = row - 1;
            break;
        }
        l.append(hh);
    }
    if(lastIndx < 0)
        hash.insert("t", rowCount);
    hash.insert("m", l);
    hash.insert("i", newLastIndx);

//        emit uploadProgress( ((newLastIndx * 100) / rowCount),
    if(newLastIndx < 0){
        emit uploadProgress(sessionId,  100, tr("Total count: %1 lamps, Uploaded: %2 lamps")
                         .arg(rowCount).arg(rowCount) );



    }else
        emit uploadProgress(sessionId,  ((newLastIndx * 100) / rowCount), tr("Total count: %1 lamps, Uploaded: %2 lamps")
                         .arg(rowCount).arg(newLastIndx) );

    if(firstWrite)
        lastWriteVarZero = hash;
    mWriteToSocket(hash, COMMAND_WRITE_LEDLAMPLIST_FRAMED);

    return MESSAGE_OPERATION_IN_PROGRESS;
}

int SvahaSocket::readFramedList(const QVariantHash &h, bool &rezIsGood, const QStringList &humanHeaders, const QStringList &lk, const quint16 &command, const QString &listName)
{
    rezIsGood = !h.isEmpty();
    if(!rezIsGood)
        return MESSAGE_CORRUPTED_DATA;

    bool ignoreEmptyList = h.contains("t");
    QStringList lHeader;

    if(ignoreEmptyList){
        lHeader = humanHeaders;//.split(",");
        totalTables = h.value("t").toInt();
        doneTables = 0;

    }

    QVariantList l = h.value("m").toList();
    int lastIndx = h.value("i").toInt();



    if((l.isEmpty() || !h.contains("i")) && !ignoreEmptyList){
        rezIsGood = false;
        return MESSAGE_CORRUPTED_DATA;
    }

    if(command == COMMAND_READ_LEDLAMPLIST_FRAMED){
        emit appendLedLampListVar(sessionId, l);
    }

    const QStringList k = lk;//.split(' ', QString::SkipEmptyPart);
    QVariantList meters;
    int iMax = l.size();

    int counterF = 0;
    QStringList lkLedList;


    const QDateTime currDt = QDateTime::currentDateTimeUtc();
    QStringList icos;

    for(int i = 0, colMax = k.size(); i < iMax; i++){
        const QVariantHash h = l.at(i).toHash();
        QStringList m;


            for(int col = 0; col < colMax; col++){
                QString s = h.value(k.at(col)).toString();
                if(k.at(col).startsWith("dt")){
                    const QDateTime dt = QDateTime( QDate::fromString(s.left(10), "yyyy-MM-dd"), QTime::fromString(s.right(8), "hh:mm:ss"), Qt::UTC);
                    if(dt.isValid())
                        s = dt.toLocalTime().toString("yyyy-MM-dd hh:mm:ss");
                }

                m.append(s);
            }




        meters.append(m);
        counterF++;


    }

    if(verboseMode)
        qDebug() << "command "  << command << lastIndx << iMax << counterF;

    if(meters.isEmpty() && lHeader.isEmpty() && lastIndx < 1){
        lHeader = humanHeaders;//.split(",");
    }
    if(!lHeader.isEmpty() || !meters.isEmpty()){
        QVariantHash hash;
        hash.insert("lHeader", lHeader);
        hash.insert("meters", meters);
        hash.insert("hasHeader", ignoreEmptyList);
        hash.insert("thisistheend", (lastIndx < 0 || lastIndx >= totalTables));

        emit onCOMMAND2GUI(sessionId, command, hash);
    }


    if(lastIndx >= 0 && totalTables > 0){

        blockDone = 0;
        doneTables += iMax;
            emit uploadProgress(sessionId,  ((doneTables * 100) / totalTables), tr("Total count: %1 %3, Downloaded: %2 %4")
                             .arg(totalTables).arg(doneTables).arg(listName).arg(listName) );
        QVariantHash hash;
        hash.insert("i", lastIndx);
        mWriteToSocket(hash, command);
        return MESSAGE_OPERATION_IN_PROGRESS;

    }else{ //-1 read done
        doneTables = totalTables;
         emit uploadProgress(sessionId,  100, tr("Total count: %1 %3, Downloaded: %2 %4")
                             .arg(totalTables).arg(totalTables).arg(listName).arg(listName) );
    }



    return MESSAGE_NO_MESSAGE;
}
//------------------------------------------------------------------------------------------
QVariantHash SvahaSocket::errCodeLastOperation(const quint16 &command, const int &errCode) const
{
    QVariantHash h;
    h.insert("lcmd", command);
    h.insert("e", errCode);
    return h;
}
//------------------------------------------------------------------------------------------
bool SvahaSocket::messHshIsValid(const QJsonObject &jObj, QByteArray readArr)
{
    QStringList lh = getHshNames();
    int hshIndx = -1;
    int lastHashSumm = 1;
    QString hshName("");
    for(int i = 0, iMax = getHshNames().size(); i < iMax && hshIndx < 0; i++){
        if(jObj.contains(lh.at(i))){
            hshIndx = i;
            hshName = lh.at(i);
            if(verboseMode)
                qDebug() << "hash is " << hshName;
        }
    }


    if(hshIndx < 0){
        if(verboseMode)
            qDebug() << "if(hshIndx < 0 " << hshIndx;
        return false;
    }else{
        lastHashSumm = hshIndx;
        int startIndx = readArr.indexOf(QString("\"%1\":").arg(hshName).toLocal8Bit());
        QByteArray hshBase64;
        if(startIndx > 0){
            startIndx += hshName.length() + 4;
            if(verboseMode)
                qDebug() << "hshName " << hshName << startIndx << readArr.mid(startIndx);

            int endIndx = readArr.indexOf("\"", startIndx + 1);
            if(verboseMode)
                qDebug() << "endIndx " << endIndx << readArr.mid(endIndx);

            hshBase64 = readArr.mid(startIndx , endIndx - startIndx);
            if(verboseMode)
                qDebug() << hshBase64;
            readArr = readArr.left(startIndx ) + "0" + readArr.mid(endIndx);
            if(verboseMode)
                qDebug() << readArr;

        }
        if(hshBase64.isEmpty())
            return false;

        hshBase64 = QByteArray::fromBase64(hshBase64);

        QByteArray myHash = QCryptographicHash::hash(readArr, static_cast<QCryptographicHash::Algorithm>(lastHashSumm));
        if(myHash == hshBase64){
            return true;
        }else{
            if(verboseMode)
                qDebug() << "if(myHash != hshBase64 " << myHash.toBase64() << hshBase64.toBase64();
            return false;
        }
    }
}

//------------------------------------------------------------------------------------------

QStringList SvahaSocket::getHshNames() const{  return QString("Md4,Md5,Sha1,Sha224,Sha256,Sha384,Sha512,Sha3_224,Sha3_256,Sha3_384,Sha3_512").split(","); }

//------------------------------------------------------------------------------------------
qint64 SvahaSocket::mWrite2SocketJSON(QJsonObject jObj, const quint16 s_command, int lastHashSumm)
{
    jObj.insert("cmd", s_command);
    stopAll = false;
    bool allowCompress = true;


    QByteArray writeArr ;
    if(true){
        QJsonDocument jDoc2DST(jObj);
        writeArr = jDoc2DST.toJson(QJsonDocument::Compact);
        writeArr.chop(1);// remove }
    }


if(verboseMode)
    qDebug() << "writeArr0 " << writeArr;
    switch(lastHashSumm){
    case 0: { writeArr.append(", \"Md4\":\""      + QCryptographicHash::hash( writeArr + ", \"Md4\":\"0\"}"     , QCryptographicHash::Md4     ).toBase64() + "\"}" ); break;}
    case 2: { writeArr.append(", \"Sha1\":\""     + QCryptographicHash::hash( writeArr + ", \"Sha1\":\"0\"}"    , QCryptographicHash::Sha1    ).toBase64() + "\"}" ); break;}
    case 3: { writeArr.append(", \"Sha224\":\""   + QCryptographicHash::hash( writeArr + ", \"Sha224\":\"0\"}"  , QCryptographicHash::Sha224  ).toBase64() + "\"}" ); break;}
    case 4: { writeArr.append(", \"Sha256\":\""   + QCryptographicHash::hash( writeArr + ", \"Sha256\":\"0\"}"  , QCryptographicHash::Sha256  ).toBase64() + "\"}" ); break;}
    case 5: { writeArr.append(", \"Sha384\":\""   + QCryptographicHash::hash( writeArr + ", \"Sha384\":\"0\"}"  , QCryptographicHash::Sha384  ).toBase64() + "\"}" ); break;}
    case 6: { writeArr.append(", \"Sha512\":\""   + QCryptographicHash::hash( writeArr + ", \"Sha512\":\"0\"}"  , QCryptographicHash::Sha512  ).toBase64() + "\"}" ); break;}
#if QT_VERSION >= QT_VERSION_CHECK(5, 9, 0)
    case 7: { writeArr.append(", \"Sha3_224\":\"" + QCryptographicHash::hash( writeArr + ", \"Sha3_224\":\"0\"}", QCryptographicHash::Keccak_224).toBase64() + "\"}" ); break;}
    case 8: { writeArr.append(", \"Sha3_256\":\"" + QCryptographicHash::hash( writeArr + ", \"Sha3_256\":\"0\"}", QCryptographicHash::Keccak_256).toBase64() + "\"}" ); break;}
    case 9: { writeArr.append(", \"Sha3_384\":\"" + QCryptographicHash::hash( writeArr + ", \"Sha3_384\":\"0\"}", QCryptographicHash::Keccak_384).toBase64() + "\"}" ); break;}
    case 10:{ writeArr.append(", \"Sha3_512\":\"" + QCryptographicHash::hash( writeArr + ", \"Sha3_512\":\"0\"}", QCryptographicHash::Keccak_512).toBase64() + "\"}" ); break;}
#else
    case 7: { writeArr.append(", \"Sha3_224\":\"" + QCryptographicHash::hash( writeArr + ", \"Sha3_224\":\"0\"}", QCryptographicHash::Sha3_224).toBase64() + "\"}" ); break;}
    case 8: { writeArr.append(", \"Sha3_256\":\"" + QCryptographicHash::hash( writeArr + ", \"Sha3_256\":\"0\"}", QCryptographicHash::Sha3_256).toBase64() + "\"}" ); break;}
    case 9: { writeArr.append(", \"Sha3_384\":\"" + QCryptographicHash::hash( writeArr + ", \"Sha3_384\":\"0\"}", QCryptographicHash::Sha3_384).toBase64() + "\"}" ); break;}
    case 10:{ writeArr.append(", \"Sha3_512\":\"" + QCryptographicHash::hash( writeArr + ", \"Sha3_512\":\"0\"}", QCryptographicHash::Sha3_512).toBase64() + "\"}" ); break;}
#endif
    default:{ writeArr.append(", \"Md5\":\""      + QCryptographicHash::hash( writeArr + ", \"Md5\":\"0\"}"     , QCryptographicHash::Md5     ).toBase64() + "\"}" ); break;}
    }

    qint64 blSize = writeArr.length();

    if(blSize >= SETT_MAX_UNCOMPRSS_PACkET_SIZE && allowCompress){

        if(true){

            QJsonObject jObjCpmrr;

            jObjCpmrr.insert("cmd", QString::number(COMMAND_COMPRESSED_PACKET));
            jObjCpmrr.insert("zlib", QString(qCompress(writeArr, 9).toBase64()));

            QJsonDocument jDoc2DST(jObjCpmrr);
            writeArr = jDoc2DST.toJson(QJsonDocument::Compact);
            writeArr.chop(1);// remove }
        }

        if(verboseMode)
            qDebug() << "writeArr1 comprs " << writeArr;
        switch(lastHashSumm){
        case 0: { writeArr.append(", \"Md4\":\""      + QCryptographicHash::hash( writeArr + ", \"Md4\":\"0\"}"     , QCryptographicHash::Md4     ).toBase64() + "\"}" ); break;}
        case 2: { writeArr.append(", \"Sha1\":\""     + QCryptographicHash::hash( writeArr + ", \"Sha1\":\"0\"}"    , QCryptographicHash::Sha1    ).toBase64() + "\"}" ); break;}
        case 3: { writeArr.append(", \"Sha224\":\""   + QCryptographicHash::hash( writeArr + ", \"Sha224\":\"0\"}"  , QCryptographicHash::Sha224  ).toBase64() + "\"}" ); break;}
        case 4: { writeArr.append(", \"Sha256\":\""   + QCryptographicHash::hash( writeArr + ", \"Sha256\":\"0\"}"  , QCryptographicHash::Sha256  ).toBase64() + "\"}" ); break;}
        case 5: { writeArr.append(", \"Sha384\":\""   + QCryptographicHash::hash( writeArr + ", \"Sha384\":\"0\"}"  , QCryptographicHash::Sha384  ).toBase64() + "\"}" ); break;}
        case 6: { writeArr.append(", \"Sha512\":\""   + QCryptographicHash::hash( writeArr + ", \"Sha512\":\"0\"}"  , QCryptographicHash::Sha512  ).toBase64() + "\"}" ); break;}
#if QT_VERSION >= QT_VERSION_CHECK(5, 9, 0)

        case 7: { writeArr.append(", \"Sha3_224\":\"" + QCryptographicHash::hash( writeArr + ", \"Sha3_224\":\"0\"}", QCryptographicHash::Keccak_224).toBase64() + "\"}" ); break;}
        case 8: { writeArr.append(", \"Sha3_256\":\"" + QCryptographicHash::hash( writeArr + ", \"Sha3_256\":\"0\"}", QCryptographicHash::Keccak_256).toBase64() + "\"}" ); break;}
        case 9: { writeArr.append(", \"Sha3_384\":\"" + QCryptographicHash::hash( writeArr + ", \"Sha3_384\":\"0\"}", QCryptographicHash::Keccak_384).toBase64() + "\"}" ); break;}
        case 10:{ writeArr.append(", \"Sha3_512\":\"" + QCryptographicHash::hash( writeArr + ", \"Sha3_512\":\"0\"}", QCryptographicHash::Keccak_512).toBase64() + "\"}" ); break;}
#else
        case 7: { writeArr.append(", \"Sha3_224\":\"" + QCryptographicHash::hash( writeArr + ", \"Sha3_224\":\"0\"}", QCryptographicHash::Sha3_224).toBase64() + "\"}" ); break;}
        case 8: { writeArr.append(", \"Sha3_256\":\"" + QCryptographicHash::hash( writeArr + ", \"Sha3_256\":\"0\"}", QCryptographicHash::Sha3_256).toBase64() + "\"}" ); break;}
        case 9: { writeArr.append(", \"Sha3_384\":\"" + QCryptographicHash::hash( writeArr + ", \"Sha3_384\":\"0\"}", QCryptographicHash::Sha3_384).toBase64() + "\"}" ); break;}
        case 10:{ writeArr.append(", \"Sha3_512\":\"" + QCryptographicHash::hash( writeArr + ", \"Sha3_512\":\"0\"}", QCryptographicHash::Sha3_512).toBase64() + "\"}" ); break;}
#endif
        default:{ writeArr.append(", \"Md5\":\""      + QCryptographicHash::hash( writeArr + ", \"Md5\":\"0\"}"     , QCryptographicHash::Md5     ).toBase64() + "\"}" ); break;}
        }
    }else{
       blSize = -1;
    }

    readAll();
    const qint64 len = write(writeArr);
    if(verboseMode){
        qDebug() << "writeJSON " << QTime::currentTime().toString("hh:mm:ss.zzz");
        qDebug() << writeArr;
    }


    waitForBytesWritten(50);
//    if(s_command != COMMAND_I_AM_A_ZOMBIE && s_command != COMMAND_WRITE_DA_DATA_2_COORDINATOR)

    return len;
}
//------------------------------------------------------------------------------------------
QVariant SvahaSocket::uncompressRead(QByteArray readArr, quint16 &command, qint64 lenBefore)
{
    if(verboseMode)
        qDebug() << "uncompress command=" << command << readArr.size() << lenBefore;

    readArr = qUncompress(readArr);
    QVariant readVar;
    QDataStream outUncompr(readArr);
    outUncompr.setVersion(dataStreamVersion); //Qt_4_0);


    outUncompr >> command >> readVar;
    if(verboseMode)
        qDebug() << "uncompress command2=" << command << readArr.size();
    return readVar;
}
//------------------------------------------------------------------------------------------
QByteArray SvahaSocket::varHash2arr(const QVariantHash &hash)
{
    //було для ПЗ
//    QByteArray block;
//    QDataStream out(&block, QIODevice::WriteOnly);
//    out.setVersion(QDataStream::Qt_5_6); //Qt_4_0);
//    out << (quint32)0;
//    out << hash;
//    out.device()->seek(0);
//    out << (quint32)(block.size() - sizeof(quint32));

//    QByteArray arr = qCompress(block,9);
//    qDebug() << "block.size " << block.size() << arr.size() << arr.left(4).toHex() << arr.left(4).toHex().toUInt(0, 16);

    //варіант для резервних копій, те саме тепер для ПЗ
    QByteArray writeArr;
    QDataStream out(&writeArr,QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_6);
    out << (quint64)0 << hash;
    out.device()->seek(0);
    quint64 blSize = writeArr.size();
    out << (quint64)(blSize );
    writeArr = qCompress(writeArr,9);


    return writeArr;
}
//------------------------------------------------------------------------------------------
