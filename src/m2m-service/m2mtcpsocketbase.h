#ifndef M2MTCPSOCKETBASE_H
#define M2MTCPSOCKETBASE_H

#include <QTcpSocket>
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>


///[!] MatildaIO
#include "src/shared/readwriteiodevice.h"


///[!] type-converter
#include "src/base/convertatype.h"


#include "m2mtcpsockettypes.h"
#include "m2mconnectiondefines.h"
#include "m2mconnectionbasetypes.h"

#include "matildalimits.h"
#include "matildaprotocolcommands.h"

//this class is for boring methods

class M2MTcpSocketBase : public QTcpSocket
{
    Q_OBJECT
public:
    explicit M2MTcpSocketBase(const bool &enableTestFeatures, const bool &ignoreProtocolVersion, const quint16 &sesionId, QObject *parent = nullptr);


    BASE_CONNECTION_INFO lastbaseconninfo;

    bool verboseMode;

    UcConnectionState mystate;
    M2MConnSett myconnsett;
    UcZombieState mZombieState;

    UcDeviceAccess mDeviceAccess;

    ClientCachedData mCachedData;//check this structure


    bool isConnectionUp();


    qint32 calculatePacketSize(const int &lenminus);

    bool isBackupsCommand(const quint16 &command);


    QJsonObject getM2MsearchMessage();

    QJsonObject getAuthorizationMessage(const QByteArray &dataArr);


    M2MreadMethod mReadyReadFJSON();

    M2MreadMethod mReadyReadFQDS();

    QByteArray getWriteArrFromJSON(const QJsonObject &json, const int &lastHashSumm);

    QByteArray getWriteArrFromJSONcompressed(const QByteArray &writeArr, const int &lastHashSumm);


    bool isMessageHashValid(const QJsonObject &json, QByteArray readArr);//messHshIsValid

    QVariant uncompressRead(QByteArray readArr, quint16 &command);

    bool isSystemCommand(const quint16 &command);
    bool isWrittenCommand(const quint16 &command);
    bool isErrorCommand(const quint16 &command);

    bool makeAconnection(const QString &host, const quint16 &port);

    //low level commands, base commands ,
    bool onCOMMAND_CONNECT_ME_2_THIS_ID_OR_MAC(const M2MreadResult &rjson);

    bool onCOMMAND_ZULU(const M2MreadResult &rjson);

    bool onCOMMAND_AUTHORIZE(const M2MreadResult &rqds);



    bool wasLastWriteGood(const M2MreadResult &rqds);//only COMMAND_ERROR_CODE_EXT and COMMAND_ERROR_CODE

    COMMAND_ZULU_STRUCT getZuluFromJSON(const M2MreadResult &rjson);

    COMMAND_AUTHORIZE_STRUCT getAuthorizeFromQDS(const M2MreadResult &rqds);

    COMMAND_ERR_CODE_STRUCT getErrCodeFromQDS(const M2MreadResult &rqds);


    qint64 mWrite2socket(const QByteArray &writearr, const quint16 &scommand, const qint64 &srclen);
signals:
    void connectionStateChanged(quint16 sessionId, int state);

    void onConnectionClosed(quint16 sessionId);//Connection was closed by not this object

    void onConnectionDown(quint16 sessionId);//onDisconnected

    void onSocketKilled(quint16 sessionId, QString socketStamp);

//all errors must be structured, and must be sent in codes, messages from UC are allowed
    void showMessage(quint16 sessionId, int errorcode, QString comment);
    void showMessageCritical(quint16 sessionId, int errorcode, QString comment);

    void onYouCanSelectDevice(QStringList devlist);

    void commandWasWritten(quint16 command, qint64 srclen, qint64 outlen);


public slots:
    void addData2cache(const quint16 &command, const QVariant &dataVar);

    void changeConnectionState(const quint8 &connectionstate);

    void sendMessage(const int &errorcode, const QString &comment);

    void sendMessageCritical(const int &errorcode, const QString &comment);

    void setConnectionSett(const QString &host, const quint16 &port, const QString &objIdOrMac, const bool &cmMAC, const int &timeOut, const int &timeOutB,
                           const QString &login, const QString &pas, const bool &useMac, const int &daChannelNumber);

    void closeTheConnection();

    void onDisconnected();

    void stopAllSlotDirect();

    void stopAllAndDestroy();

    void prepary4anewconnection();

    void onDataIsWritten(quint16 command, qint64 srclen, qint64 outlen);





};

#endif // M2MTCPSOCKETBASE_H
