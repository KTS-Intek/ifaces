#include "m2mtcpsocketbase.h"




//-----------------------------------------------------------------------------

M2MTcpSocketBase::M2MTcpSocketBase(const bool &enableTestFeatures, const bool &ignoreProtocolVersion, const quint16 &sesionId, QObject *parent) : QTcpSocket(parent)
{
    mystate.enableTestFeatures = enableTestFeatures;
    mystate.isIgnoringTheVersionOfTheProtocol = ignoreProtocolVersion;
    mystate.sessionid = sesionId;
    verboseMode = false;
}

//-----------------------------------------------------------------------------

bool M2MTcpSocketBase::isConnectionUp()
{
    return ReadWriteIODevice::isConnOpen(this);
}

//-----------------------------------------------------------------------------

bool M2MTcpSocketBase::isBackupsCommand(const quint16 &command)
{
    bool rez = false;
    switch(command){
    case COMMAND_GET_BACKUP_FILE            : rez = true; break;
    case COMMAND_PUSH_BACKUP_FILE_AND_APPLY : rez = true; break;
    case COMMAND_WRITE_GET_BACKUPFILE       : rez = true; break;
    }
    return rez;
}

//-----------------------------------------------------------------------------

QJsonObject M2MTcpSocketBase::getM2MsearchMessage()
{
    QJsonObject jObj;
    jObj.insert("version", mystate.enableTestFeatures ? MATILDA_PROTOCOL_VERSION_TEST : MATILDA_PROTOCOL_VERSION);
    jObj.insert("useId", !myconnsett.objIdOrMac);
    //
    //mode JSON and QDataStream
    jObj.insert("QDS", QString::number( mystate.dataStreamVersion));//активація режиму QDataStream
    jObj.insert("pos", true);
    jObj.insert("cmmprssn", "zlib");
    jObj.insert("cmprssn", "zlib");
    jObj.insert("remote", myconnsett.objIdOrMac);// (useMacAddr2conn) ? macAddr : objN);
    return jObj;
}

//-----------------------------------------------------------------------------

QJsonObject M2MTcpSocketBase::getAuthorizationMessage(const QByteArray &dataArr)
{
    QJsonObject jObj;

    jObj.insert("version", hash.value("version").toInt());

#if QT_VERSION >= 0x050902
    jObj.insert("hsh", QString(QCryptographicHash::hash(mDeviceAccess.loginPasswd.at(0) + "\n" + dataArr + "\n" + mDeviceAccess.loginPasswd.at(1), QCryptographicHash::Keccak_256).toBase64()));
#else
    jObj.insert("hsh", QString(QCryptographicHash::hash(mDeviceAccess.loginPasswd.at(0) + "\n" + dataArr + "\n" + mDeviceAccess.loginPasswd.at(1), QCryptographicHash::Sha3_256).toBase64()));

#endif

    //mode JSON and QDataStream
    jObj.insert("QDS", QString::number(mystate.dataStreamVersion));//активація режиму QDataStream
    jObj.insert("cmmprssn", lastbaseconninfo.zulu.cmprssn.split(" ").first());

    //if the version of the protocol is ignored, than do not read supported plugins

    const bool itisgoing2openda = (myconnsett.daChannelNumber > COMMAND_DA_CLOSE);

    if(!itisgoing2openda){

        if(!mystate.isIgnoringTheVersionOfTheProtocol)//
            jObj.insert("plg", true);

        jObj.insert("pos", true);


    }



    return jObj;
}

//-----------------------------------------------------------------------------

M2MreadMethod M2MTcpSocketBase::mReadyReadFJSON()
{
    M2MreadMethod r;
    r.readArrList = ReadWriteIODevice::readIODevice(this, myconnsett.timeOutMsecB, myconnsett.timeOutMsecG, verboseMode, r.blockSize, r.hasErr);

    return r;

}

//-----------------------------------------------------------------------------

M2MreadMethod M2MTcpSocketBase::mReadyReadFQDS()
{
    M2MreadMethod r;
    r.readVar =  ReadWriteIODevice::readIODevice(this, myconnsett.timeOutMsecB, myconnsett.timeOutMsecG, r.hasErr, r.hasMoreData, r.serverCommand, r.blockSize);
    return r;
}

//-----------------------------------------------------------------------------

QByteArray M2MTcpSocketBase::getWriteArrFromJSON(const QJsonObject &json, const int &lastHashSumm)
{
    return ReadWriteIODevice::getArrWithHashSummFromJSON(json, lastHashSumm);
}

//-----------------------------------------------------------------------------

QByteArray M2MTcpSocketBase::getWriteArrFromJSONcompressed(const QByteArray &writeArr, const int &lastHashSumm)
{
    QJsonObject jObjCpmrr;
    jObjCpmrr.insert("cmd", COMMAND_COMPRESSED_PACKET);
    jObjCpmrr.insert("zlib", QString(qCompress(writeArr, 9).toBase64(QByteArray::OmitTrailingEquals)));

    return getWriteArrFromJSON(jObjCpmrr, lastHashSumm);

}

//-----------------------------------------------------------------------------

bool M2MTcpSocketBase::isMessageHashValid(const QJsonObject &json, QByteArray readArr)
{
    int alg;
    return ReadWriteIODevice::isMessageHashValid(json, readArr, alg);
}

//-----------------------------------------------------------------------------

QVariant M2MTcpSocketBase::uncompressRead(QByteArray readArr, quint16 &command)
{
    qint64 lenuncompressed;
    return ReadWriteIODevice::uncompressRead(readArr, command, lenuncompressed, verboseMode);
}

//-----------------------------------------------------------------------------

bool M2MTcpSocketBase::isSystemCommand(const quint16 &command)
{
    return (command == COMMAND_I_AM_A_ZOMBIE);//these commands have no answer
}

//-----------------------------------------------------------------------------

bool M2MTcpSocketBase::isWrittenCommand(const quint16 &command)
{
    return (command == mCachedData.lastWriteCommand || isErrorCommand(command));
}

//-----------------------------------------------------------------------------

bool M2MTcpSocketBase::isErrorCommand(const quint16 &command)
{
    return (command == COMMAND_ERROR_CODE || command == COMMAND_ERROR_CODE_EXT);

}

//-----------------------------------------------------------------------------

bool M2MTcpSocketBase::makeAconnection(const QString &host, const quint16 &port)
{
    connectToHost(host, port);
    changeConnectionState(M2M_CONN_STATE_CONNECTING);

    QTime connectiontime;
    connectiontime.restart();

    for(int i = 0; i < 100000 && connectiontime.elapsed() < myconnsett.timeOutMsecG && !mystate.stopAll; i++){
        if(waitForConnected(myconnsett.timeOutMsecB))
            return true;

        if(!isConnectionUp())
            break;
    }
    return false;
}

//-----------------------------------------------------------------------------

bool M2MTcpSocketBase::onCOMMAND_CONNECT_ME_2_THIS_ID_OR_MAC(const M2MreadResult &rjson)
{
    const QVariantMap map = rjson.var.toMap();

    if(map.contains("sIp") && map.contains("sP")){
        //TCP medium service
        mystate.block4activeClient = true;
        close();
        mystate.isM2Mservice = false;

        return makeAconnection(map.value("sIp").toString(), map.value("sP").toUInt());
    }

    const QVariantList devlist = map.value("l").toList();
    QStringList list;
    for(int i = 0, iMax = devlist.size(); i < iMax; i++)
        list.append(devlist.at(i).toString());

    if(!list.isEmpty())
        emit onYouCanSelectDevice(list);

    return false;
}

//-----------------------------------------------------------------------------

bool M2MTcpSocketBase::onCOMMAND_ZULU(const M2MreadResult &rjson)
{
//process about device

    if(rjson.messageFailed)
        return false;

    const COMMAND_ZULU_STRUCT zulu = getZuluFromJSON(rjson);
    lastbaseconninfo.zulu = zulu;


    const int deviceprotocolversion = zulu.version;
    const int currentprotocolversion = mystate.enableTestFeatures ? MATILDA_PROTOCOL_VERSION_TEST : MATILDA_PROTOCOL_VERSION;
    const bool isProtocolGood = (mystate.isIgnoringTheVersionOfTheProtocol || (deviceprotocolversion <= currentprotocolversion));

    const QDateTime devicedt = QDateTime(
                QDate::fromString( zulu.UTC.left(10), "yyyy-MM-dd"),
                QTime(zulu.UTC.right(8), "hh:mm:ss"), Qt::UTC ).addSecs(zulu.UOFT);

    if(zulu.name == "Matilda" && zulu.version > 0 && devicedt.isValid()){

        if(!isProtocolGood){
            sendMessageCritical(M2M_ERR_OLDPROTOCOLVERSION, QString("%1\n%2").arg(zulu.version).arg(currentprotocolversion));
            return false;
        }

        if(!zulu.message.isEmpty() || (zulu.err > 0 && zulu.err != ERR_NO_ERROR)){
            //do not use zulu.err because error codes must be from uc-connect, not from matilda protocol
            sendMessageCritical(M2M_ERR_IP_IN_BLOCKLIST, zulu.message);
            return false;
        }


        if(zulu.QDS >= QDataStream::Qt_4_8 && zulu.QDS <= QDataStream::Qt_DefaultCompiledVersion){ //QDS is ok,
    //in case the selected UC doesn't have a position, it calculates
            lastbaseconninfo.authorize.positionsn = DEVICE_POSITION_SN(QPointF( ((qreal)zulu.UOFT/(qreal)240.0 ), 49), true);//reset
            mystate.dataStreamVersion = zulu.QDS;

            return true;//it is ready to make athorization
        }

    }


    sendMessageCritical(M2M_ERR_UNKNOWN_DEVICE, "");
    return false;

}

//-----------------------------------------------------------------------------

bool M2MTcpSocketBase::onCOMMAND_AUTHORIZE(const M2MreadResult &rqds)
{
    if(rqds.messageFailed)
        return false;
//it is more convinient to use local variables for debuging
    const COMMAND_AUTHORIZE_STRUCT authorize = getAuthorizeFromQDS(rqds);

    lastbaseconninfo.authorize = authorize;

    if(authorize.accessLevel == MTD_USER_NOAUTH){
        sendMessageCritical(M2M_ERR_ACCESS_DENIED, "");
        return false;

    }

    const bool isAble2write = (authorize.accessLevel == MTD_USER_ADMIN || authorize.accessLevel == MTD_USER_OPER);
//check direct access
    if(myconnsett.daChannelNumber != COMMAND_DA_CLOSE && isAble2write)
        return true;

    if(isAble2write || authorize.accessLevel == MTD_USER_GUEST)
        return true;


    sendMessageCritical(M2M_ERR_ACCESS_DENIED, "");
    return false;

}

//-----------------------------------------------------------------------------


bool M2MTcpSocketBase::wasLastWriteGood(const M2MreadResult &rqds)
{
    if(!isErrorCommand(rqds.command))
        return false;


    const COMMAND_ERR_CODE_STRUCT ercode = getErrCodeFromQDS(rqds);

    if(ercode.lcmd != mCachedData.lastWriteCommand)
        return false;

    if(ercode.errcode == ERR_NO_ERROR)
        return true;

    sendMessageCritical(M2M_ERR_WRITE_ERROR, ercode.message);
    return false;

}

//-----------------------------------------------------------------------------

COMMAND_ZULU_STRUCT M2MTcpSocketBase::getZuluFromJSON(const M2MreadResult &rjson)
{
    const QVariantMap map = rjson.var.toMap();

    COMMAND_ZULU_STRUCT zulu;

    zulu.name       = map.value("name").toString();
    zulu.version    = map.value("version").toInt();

    zulu.UTC        = map.value("UTC").toString();
    zulu.UOFT       = map.value("UOFT").toInt();

    zulu.memo       = map.value("memo").toString();
    zulu.QDS        = map.value("QDS").toInt();

    zulu.BLC        = map.value("BLC").toInt();
    zulu.CTCT       = map.value("CTCT").toInt();
    zulu.CNTR       = map.value("CNTR").toInt();

    zulu.cmprssn    = map.value("cmprssn").toString();

    zulu.RND        = map.value("RND").toUInt();

    //error happend
    zulu.err        = map.value("err", ERR_NO_ERROR).toInt();//ERR_IP_BLOCKED
    zulu.message    = map.value("message").toString();
    return zulu;

}

//-----------------------------------------------------------------------------

COMMAND_AUTHORIZE_STRUCT M2MTcpSocketBase::getAuthorizeFromQDS(const M2MreadResult &rqds)
{

    const QVariantHash readhash = rqds.var.toHash();
    COMMAND_AUTHORIZE_STRUCT authorize;

    if(!readhash.value("v").toString().isEmpty())
        authorize.buildversion = readhash.value("v").toString();


    if(readhash.contains("d")){
        authorize.devtype = readhashh.value("d").toInt();
        if(authorize.devtype != DEV_POLL && authorize.devtype != DEV_SVAHA)
            mZombieState.ignoreAnser4zombie = true;//в емуляторах зомбі є помилка із зомбі таймером, поки-що
    }

    authorize.positionsn.hasSn = readhash.contains("sn");

    if(authorize.positionsn.hasSn)
        authorize.positionsn.devicesn = readhash.value("sn").toString();


    if(readhash.contains("pos")){
        bool ok;
        const QPointF p = ConvertAtype::coordinatesFromStr(readhash.value("pos").toString(), ok);
        if(ok){
            authorize.positionsn.point = p;
            authorize.positionsn.isDefault = false;
        }

    }

    authorize.accessLevel = readhash.value("a", MTD_USER_NOAUTH).toUInt();

    //these are optional, but very useful
    const QStringList lkmeterkeys = QString("b bw bext bwext").split(" ", QString::SkipEmptyParts);
    for(int i = 0, imax = lkmeterkeys.size(); i < imax; i++){
        const QString key = lkmeterkeys.at(i);
        if(readhash.contains(key))
            authorize.hashMeterModelRules.insert(key, readhash.value(key));
    }

    return authorize;
}

//-----------------------------------------------------------------------------

COMMAND_ERR_CODE_STRUCT M2MTcpSocketBase::getErrCodeFromQDS(const M2MreadResult &rqds)
{

    const QVariantHash readhash = rqds.var.toHash();

    COMMAND_ERR_CODE_STRUCT errcode;

    errcode.lcmd = readhash.value("lcmd").toUInt();
    errcode.message = readhash.value("em").toString();
    errcode.errcode = readhash.value("e").toInt();
    return errcode;
}

//-----------------------------------------------------------------------------

qint64 M2MTcpSocketBase::mWrite2socket(const QByteArray &writearr, const quint16 &scommand, const qint64 &srclen)
{

    const qint64 writelen = write(writearr);
    onDataIsWritten(scommand, srclen, writelen);
    waitForBytesWritten(50);
    return writelen;
}

//-----------------------------------------------------------------------------

void M2MTcpSocketBase::addData2cache(const quint16 &command, const QVariant &dataVar)
{
    mCachedData.hashCache.clear();

    if(command == COMMAND_WRITE_UPGRADE){

        QHash<QString,QVariant> h = dataVar.toHash();
        QList<QString> l = h.keys();
        QStringList ll;
        for(int i = 0, iMax = l.size(); i < iMax; i++){
            if(l.at(i).left(2) == "@_")
                ll.append(h.value(l.at(i)).toString());
        }

        mCachedData.hashWriteCache.insert(command, qCompress(ll.join("\n").toLocal8Bit() , 9));
        mCachedData.hashCache = dataVar.toHash();
    }else{
        mCachedData.hashWriteCache.insert(command, varHash2arr(dataVar.toHash()));
    }
}

//-----------------------------------------------------------------------------

void M2MTcpSocketBase::changeConnectionState(const quint8 &connectionstate)
{
    if(connectionstate != mystate.lastConnectionState){
        mystate.lastConnectionState = connectionstate;
        emit connectionStateChanged(mystate.sessionid, mystate.lastConnectionState);
    }
}

//-----------------------------------------------------------------------------

void M2MTcpSocketBase::sendMessage(const int &errorcode, const QString &comment)
{    
    emit showMessage(mystate.sessionid, errorcode, comment);
}
//-----------------------------------------------------------------------------
void M2MTcpSocketBase::sendMessageCritical(const int &errorcode, const QString &comment)
{
    emit showMessageCritical(mystate.sessionid, errorcode, comment);

}

//-----------------------------------------------------------------------------

void M2MTcpSocketBase::setConnectionSett(const QString &host, const quint16 &port, const QString &objIdOrMac, const bool &cmMAC, const int &timeOut,
                                         const int &timeOutB, const QString &login, const QString &pas, const bool &useMac, const int &daChannelNumber)
{
    //quick-collect style

    myconnsett.host = host;
    myconnsett.port = port;
    myconnsett.objIdOrMac = objIdOrMac;
    myconnsett.cmMAC = cmMAC;
    myconnsett.timeOutMsecB = timeOutB;
    myconnsett.timeOutMsecG = timeOut;
    myconnsett.login = login;
    myconnsett.password = pas;
    myconnsett.useMac = useMac;

    myconnsett.daChannelNumber = daChannelNumber;


#if QT_VERSION >= 0x050902
    mDeviceAccess.loginPasswd.append(QCryptographicHash::hash(login.simplified().trimmed().toUtf8(), QCryptographicHash::Keccak_256));
    mDeviceAccess.loginPasswd.append(QCryptographicHash::hash( passwd.simplified().trimmed().toUtf8(), QCryptographicHash::Keccak_256));
#else
    mDeviceAccess.loginPasswd.append(QCryptographicHash::hash(login.simplified().trimmed().toUtf8(), QCryptographicHash::Sha3_256));
    mDeviceAccess.loginPasswd.append(QCryptographicHash::hash( passwd.simplified().trimmed().toUtf8(), QCryptographicHash::Sha3_256));

#endif

    mDeviceAccess.loginPasswd.append("");
    mDeviceAccess.isJSONmode = true;

}

//-----------------------------------------------------------------------------

void M2MTcpSocketBase::closeTheConnection()
{
    changeConnectionState(M2M_CONN_STATE_DISCONNECTING);
    emit onConnectionDown(mystate.sessionid);//this or onConnectionClosed can emit
    close();
}

//-----------------------------------------------------------------------------

void M2MTcpSocketBase::onDisconnected()
{
    changeConnectionState(M2M_CONN_STATE_DISCONNECTED);

    if(mystate.block4activeClient){
        mystate.block4activeClient = false;//m2m connection
        return;
    }
    if(mystate.ignoreThisDisconn)
        return;

    if(!mystate.iAmDone){
        mystate.iAmDone = true;

        emit onConnectionClosed(mystate.sessionid);
        emit onSocketKilled(mystate.sessionid, );
    }
    close();
}

//-----------------------------------------------------------------------------

void M2MTcpSocketBase::stopAllSlotDirect()
{//directly from other thread
    mystate.stopAll = true;
}
//-----------------------------------------------------------------------------


void M2MTcpSocketBase::stopAllAndDestroy()
{//do not use Qt::DirectConnection
    mystate.stopAll = true;
    mystate.block4activeClient = false;
    closeTheConnection();
    QTimer::singleShot(11, this, SLOT(deleteLater()));
}

//-----------------------------------------------------------------------------

void M2MTcpSocketBase::prepary4anewconnection()
{
    //reset some variables
    mystate.stopAll = false;
    mystate.dataStreamVersion = QDataStream::Qt_DefaultCompiledVersion;

    mystate.lastMaxDataLen = MAX_PACKET_LEN_RECOMENDATION;
    mystate.daOpened = false;
    mystate.iAmDone = false;
    mystate.block4activeClient = true;
    mystate.ignoreThisDisconn = false;
    mystate.block4activeClient = false;
    mystate.isM2Mservice = myconnsett.useMac;//useMac;
    mystate.lastMacOrObjId = myconnsett.objIdOrMac;// useMacAddr2conn ? macAddr : objN;
    mystate.lastUseMacAdd2conn = myconnsett.cmMAC;// useMacAddr2conn;//caching
    mystate.disconAfterAnswer = false;


    mCachedData.hashWriteCache.clear();
    mCachedData.saveFileCacheArr.clear();

    mZombieState.zombieRetr = 0;
    mZombieState.isWait4zombie = false;
    mZombieState.ignoreAnser4zombie = false;

}

//-----------------------------------------------------------------------------

void M2MTcpSocketBase::onDataIsWritten(quint16 command, qint64 srclen, qint64 outlen)
{
    mystate.lastBytesWritten = outlen;
    mystate.timewrite.restart();
    mCachedData.lastWriteCommand = command;

    emit commandWasWritten(command, srclen, outlen);

}

//-----------------------------------------------------------------------------

qint32 M2MTcpSocketBase::calculatePacketSize()//calcPacketSize(int minusThisLen)
{

    const quint64 unlimData = 0;
//speedStat.lastMaxDataLen is not useful, it is better to use lastByteWrt
//    speedStat.lastMaxDataLen;
    qint64 lastMaxDataLen = quint64(mystate.lastBytesWritten);
     bool buvIneedMoreTime = (mystate.timewrite.elapsed() > 1000);

    const qint64 maxDataLen = ReadWriteIODevice::calcMaxDataLenSigned(unlimData, lastMaxDataLen, buvIneedMoreTime, mConnState.timewrite.elapsed(), 20);

    const int timeelpsd = mystate.timewrite.elapsed();

    if(maxDataLen > 1400 && mystate.lastProtocolVersion < MATILDA_PROTOCOL_VERSION_V8){//there was a bug with timeouts (only M2M connections)
        //upload data len
       if(mystate.lastProtocolVersion < MATILDA_PROTOCOL_VERSION_V7)
           return 1400;

        if(timeelpsd < 100)
            return (maxDataLen > 8000) ? 8000 : maxDataLen;
        if(timeelpsd < 150)
            return (maxDataLen > 4000) ? 4000 : maxDataLen;

        if(timeelpsd < 200)
            return (maxDataLen > 2500) ? 2500 : maxDataLen;

        return 1400;
    }

    if(maxDataLen > 8000){
        if(timeelpsd < 111)
            return (maxDataLen > 100000) ? 100000 : maxDataLen;
        if(timeelpsd < 222)
            return (maxDataLen > 50000) ? 50000 : maxDataLen;
        if(timeelpsd < 444)
            return (maxDataLen > 8000) ? 8000 : maxDataLen;

        return 8000;
    }

    return maxDataLen;//qMin(qint64(8000), maxDataLen);
}

//-----------------------------------------------------------------------------

