#include "m2mtcpsocket.h"

//-------------------------------------------------------------------------------------------------------------------

M2MTcpSocket::M2MTcpSocket(const bool &enableTestFeatures, const bool &ignoreProtocolVersion, const quint16 &sessionId, QObject *parent)
    : M2MTcpSocketBase(enableTestFeatures, ignoreProtocolVersion, sessionId, parent)
{

}

//-------------------------------------------------------------------------------------------------------------------

bool M2MTcpSocket::isWritten2SocketJSON(const QJsonObject &json, const quint16 &s_command)
{
    const qint64 len = mWrite2SocketJSON(json, s_command, 2);//Sha1
    return (len > 0);
}

//-------------------------------------------------------------------------------------------------------------------

qint64 M2MTcpSocket::mWrite2SocketJSON(QJsonObject json, const quint16 &s_command, int lastHashSumm)
{
    json.insert("cmd", s_command);
     mystate.stopAll = false;
    //    bool allowCompress = true; it is useless

    QByteArray writeArr = getWriteArrFromJSON(json, lastHashSumm) ;

    const qint64 arrlen = writeArr.length();

    if(arrlen >= MAX_UNCOMPRSS_PACKET_SIZE /*&& allowCompress*/){
        writeArr = getWriteArrFromJSONcompressed(writeArr, lastHashSumm);
    }

    return mWrite2socket(writeArr, s_command, arrlen);
}

//-------------------------------------------------------------------------------------------------------------------

bool M2MTcpSocket::isWritten2SocketQDS(const QVariant &var, const quint16 &s_command)
{
    const qint64 len = mWrite2SocketQDS(var, s_command);
    return (len > 0);
}

//-------------------------------------------------------------------------------------------------------------------

qint64 M2MTcpSocket::mWrite2SocketQDS(const QVariant &var, const quint16 &s_command)
{
    const qint64 arrlen = ReadWriteIODevice::preparyMessage(var, s_command, verboseMode, NET_SPEED_UFS_1).size();//do not compress by speed, and ignore size

    const QByteArray writeArr = ReadWriteIODevice::preparyMessage(var, s_command, verboseMode, NET_SPEED_NORMAL);//do not compress by speed, only by size

    return mWrite2socket(writeArr, s_command, arrlen);


}

//-------------------------------------------------------------------------------------------------------------------

M2MreadResult M2MTcpSocket::mReadyReadF()
{
    M2MreadResult result;
    const M2MreadMethod readval = mDeviceAccess.isJSONmode ? mReadyReadFJSON() : mReadyReadFQDS();
    if(readval.hasErr ||
            (mDeviceAccess.isJSONmode && readval.readArrList.isEmpty()) ||
            (!mDeviceAccess.isJSONmode && readval.readVar.isNull()) ){


        result.messageFailed = true;
        //        readval.hasErr = true;//to make sure

    }
    //check uncompress
    result = (mDeviceAccess.isJSONmode) ?
                processJSONread(readval) :
                processQDSread(readval);

    if(result.messageFailed)
        closeTheConnection();//to reset all buffers

    return result;
}

//-------------------------------------------------------------------------------------------------------------------

void M2MTcpSocket::conn2thisDev(QString objN, QString login, QString passwd, QString add, quint16 port, int timeoutMsec,
                                bool add2list, bool useMac, QString macAddr, bool useMacAddr2conn, int daChannelNumber)
{//UCon style

    setConnectionSett(add, port, useMacAddr2conn ? macAddr : objN, useMacAddr2conn, timeoutMsec, qMax(timeoutMsec/10, 500), login, passwd, useMac, daChannelNumber);
    startConnection();

    if(isConnectionUp() && add2list){

    }

}

//-------------------------------------------------------------------------------------------------------------------

void M2MTcpSocket::startConnection()
{
    disconnect(this, SIGNAL(readyRead()), this, SLOT(mReadyRead()));
    const bool isConnectionCreated = createAconnection();
    const bool isConnectionUpNow = isConnectionUp();

    if(!isConnectionCreated || mystate.stopAll || !isConnectionUpNow){
        closeTheConnection();
        return;
    }

}

//-------------------------------------------------------------------------------------------------------------------

void M2MTcpSocket::openDirectAccessConnection()
{

    if(!isConnectionUp())
        startConnection();
    if(isConnectionUp() && myconnsett.daChannelNumber > COMMAND_DA_CLOSE){
        mWrite2SocketQDS(myconnsett.daChannelNumber, COMMAND_WRITE_DA_OPEN_CLOSE);
        //0 COMMAND_DA_CLOSE
        //1 COMMAND_DA_OPEN - main
        //1000+ COMMAND_DA_OPEN_TCP_SERVER_MEDIUM_ID_F ...
        //2002+ COMMAND_DA_OPEN_CHANNEL_ID - the second channel ...


        const M2MreadResult rqds = mReadyReadF();//procress
        if(rqds.messageFailed)
            return;// false;




        if(wasLastWriteGood(rqds)){

            connect(this, SIGNAL(readyRead()), this, SLOT(mReadyRead()));

            mCachedData.lastWriteCommand = COMMAND_READ_DA_DATA_FROM_COORDINATOR;//read this only
//connect readyRead to mReadyReadSlot

            return;// true;
        }
    }
    closeTheConnection();
}

//-------------------------------------------------------------------------------------------------------------------

void M2MTcpSocket::mReadyRead()
{
    disconnect(this, SIGNAL(readyRead()), this, SLOT(mReadyRead()));
     const M2MreadResult rqds = mReadyReadF();

    connect(this, SIGNAL(readyRead()), this, SLOT(mReadyRead()));

    if(rqds.messageFailed)
       return;

    qDebug() << "M2MTcpSocket::mReadyRead() " << rqds.var;//make some magic here

}

//-------------------------------------------------------------------------------------------------------------------

bool M2MTcpSocket::createAconnection()
{


    changeConnectionState(M2M_CONN_STATE_CONNECTING);
    disconnect(this, SIGNAL(disconnected()), this, SLOT(onDisconnected())) ;

    if(isConnectionUp()){
        closeTheConnection();
    }
    //reset variables
    prepary4anewconnection();




    bool isconnected = makeAconnection(myconnsett.host,myconnsett.port);


    if(!mystate.stopAll && isconnected && myconnsett.useMac && !isAble2connectViaM2Mservice()){
        isconnected = false;//the device is not found or something else
    }

    if(!mystate.stopAll && isconnected){
        connect(this, SIGNAL(disconnected()), this, SLOT(onDisconnected())) ;

        //authorize
        changeConnectionState(M2M_CONN_STATE_CONNECTED);

        if(isAble2authorize() && !mystate.stopAll){


            changeConnectionState(M2M_CONN_STATE_LOGGINING);

            if(!isWritten2SocketJSON(getAuthorizationMessage(mDeviceAccess.zuluArray), COMMAND_AUTHORIZE))
                return false;
            mDeviceAccess.zuluArray.clear();
            mDeviceAccess.isJSONmode = false;//it must be in QDS
            const M2MreadResult rqds = mReadyReadF();//procress
            if(rqds.messageFailed)
                return false;



            if(rqds.command == COMMAND_AUTHORIZE){
                return onCOMMAND_AUTHORIZE(rqds);
            }
            return false;

        }
    }

    closeTheConnection();
    return false;//terminate everything, but do not call stop method
}

//-------------------------------------------------------------------------------------------------------------------

bool M2MTcpSocket::isAble2connectViaM2Mservice()
{
    changeConnectionState(M2M_CONN_STATE_SEARCHINGM2M);

    if(!isWritten2SocketJSON(getM2MsearchMessage(), COMMAND_CONNECT_ME_2_THIS_ID_OR_MAC))
        return false;

    const M2MreadResult rjson = mReadyReadF();//procress
    if(rjson.messageFailed)
        return false;



    if(rjson.command == COMMAND_CONNECT_ME_2_THIS_ID_OR_MAC){
        return onCOMMAND_CONNECT_ME_2_THIS_ID_OR_MAC(rjson);
    }
    return false;


}

//-------------------------------------------------------------------------------------------------------------------

bool M2MTcpSocket::isAble2authorize()
{
    //wait for COMMAND_ZULU



    onDataIsWritten(COMMAND_ZULU, 0,0);//it must wait 4 this command

    const M2MreadResult rjson = mReadyReadF();//procress
    if(rjson.messageFailed)
        return false;

    if(rjson.command == COMMAND_ZULU){
        return onCOMMAND_ZULU(rjson);
    }

     return false;

}



//-------------------------------------------------------------------------------------------------------------------

M2MreadResult M2MTcpSocket::processJSONread(const M2MreadMethod &readval)
{
    M2MreadResult result;
    for(int i = 0, imax = readval.readArrList.size(); i < imax; i++){
        QJsonObject json = QJsonDocument::fromJson( readval.readArrList.at(i)).object();

        result.command = quint16(json.value("cmd").toInt());

        if(result.command == COMMAND_COMPRESSED_PACKET){
            if(!isMessageHashValid(json, readval.readArr)){
                result.messageFailed = true;
                return result;
            }
            json = QJsonDocument::fromJson( qUncompress( QByteArray::fromBase64( json.value("zlib").toString().toLocal8Bit() )  )).object();
            result.command = quint16(json.value("cmd").toInt());
        }

        if(!isMessageHashValid(json, readval.readArr)){
            result.messageFailed = true;
            return result;
        }

        const bool itWasWrittenCommand = isWrittenCommand(result.command);

        if(!itWasWrittenCommand && isSystemCommand(result.command)){
            continue;//ignore it
        }

        if(result.command == COMMAND_ZULU){
            //remember array
            mDeviceAccess.zuluArray = readval.readArrList;
        }


        json.remove("cmd");
        result.messageFailed = false;
        result.var = json.toVariantMap();


        if(itWasWrittenCommand)
            break;
    }
     return result;
}

//-------------------------------------------------------------------------------------------------------------------

M2MreadResult M2MTcpSocket::processQDSread(M2MreadMethod readval)
{
    M2MreadResult result;


    for(int i = 0, imax = 10; i <= 10; i++){//read more if more data is available

        result.command = readval.serverCommand;

        const QVariant var = (readval.serverCommand == COMMAND_COMPRESSED_PACKET) ?
                    uncompressRead(readval.readVar, result.command) :
                    readval.readVar;




        const bool itWasWrittenCommand = isWrittenCommand(result.command);

        if(!itWasWrittenCommand && isSystemCommand(result.command)){
            continue;//ignore it
        }

        result.messageFailed = false;
        result.var = var;

        if(itWasWrittenCommand)
            break;

        if(readval.hasMoreData && i < imax)
            readval = mReadyReadFQDS();
    }
    return result;
}

//-------------------------------------------------------------------------------------------------------------------

