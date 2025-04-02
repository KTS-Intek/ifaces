#ifndef M2MTCPSOCKETTYPES_H
#define M2MTCPSOCKETTYPES_H

#include <QString>
#include <QByteArray>
#include <QVariantHash>
#include <QTime>


#include "m2mconnectiondefines.h"
#include "ifaceexchangetypesdefs.h"

struct M2MConnSett{
    QString host;
    quint16 port;
    QString objIdOrMac;
    bool cmMAC;//MAC or Object ID
    int timeOutMsecG;
    int timeOutMsecB;
    QString login;
    QString password;
    bool useMac;//use M2M service

    //v2, for convinience
    int daChannelNumber;//it is better to check access leven in onCOMMAND_AUTHORIZE
    M2MConnSett() : port(65000), cmMAC(false), timeOutMsecG(15000), timeOutMsecB(500), useMac(false), daChannelNumber(COMMAND_DA_CLOSE) {}
};


//the next structures must be checked, I think they have manu useless variables
struct UcConnectionState
{


    //v1
    bool iAmDone;
    bool ignoreThisDisconn;
    bool disconAfterAnswer;
    bool stopAll;
    bool stopAfter;
    bool daOpened;

    bool lastUseMacAdd2conn;

    bool isM2Mservice;//isSvahaService;
    int lastMaxDataLen;
    int dataStreamVersion;
    QString lastMacOrObjId;
    bool block4activeClient;
//    int lastProtocolVersion; moved to lastbaseconninfo.zulu

    bool isIgnoringTheVersionOfTheProtocol;//used
    bool enableTestFeatures; //used

    QTime timeread;
    QTime timewrite;
    qint64 lastBytesWritten;

    quint16 sessionid;
    quint8 lastConnectionState;

    UcConnectionState() : iAmDone(false), ignoreThisDisconn(false), disconAfterAnswer(false), stopAll(false), stopAfter(false), daOpened(false),
        enableTestFeatures(false), sessionid(0), lastConnectionState(0xFF) {}
} ;

struct UcDeviceAccess
{
//    quint8 accessLevel; use authorize.accessLevel
    bool isJSONmode;//use this instead of the next two
    QByteArray zuluArray;//used
//    bool matildaLogined; //use class states to determine the current state of the process
//    bool matildaLogining;
    QList<QByteArray> loginPasswd;

    UcDeviceAccess() : isJSONmode(true) {}

};

struct UcZombieState
{
    quint8 zombieRetr;

    bool isWait4zombie;
    bool ignoreAnser4zombie;

    UcZombieState() : zombieRetr(0), isWait4zombie(false), ignoreAnser4zombie(false) {}
};
struct ClientCachedData
{
    quint16 lastServerCommand;
    QVariant lastReadData;

    quint16 lastWriteCommand;

     QString lastSaveFileName;

     QByteArray saveFileCacheArr;
     qint32 lastSaveFileSize;

     QString lastLogin, lastObjId, lastMac, lastinfoAboutObj;
//     QString lastMemoFromDev; moved to lastbaseconninfo.zulu

     QHash<quint16,QByteArray> hashWriteCache;
     QVariantHash hashCache;

    ClientCachedData() {}
} ;


struct M2MreadMethod{
    QList<QByteArray> readArrList;
    QVariant readVar;
    bool hasErr;
    bool hasMoreData;
    quint16 serverCommand ;
    quint32 blockSize;//addLen
    M2MreadMethod() : hasErr(true), hasMoreData(false), serverCommand(0), blockSize(0) {}
};


struct M2MreadResult{
    bool messageFailed;
    QVariant var;
    quint16 command;
    M2MreadResult() : messageFailed(true), command(0xFFFF) {}
};




#endif // M2MTCPSOCKETTYPES_H
