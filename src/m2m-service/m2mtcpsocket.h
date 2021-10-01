#ifndef M2MTCPSOCKET_H
#define M2MTCPSOCKET_H

#include "m2mtcpsocketbase.h"

class M2MTcpSocket : public M2MTcpSocketBase
{
    Q_OBJECT
public:
    explicit M2MTcpSocket(const bool &enableTestFeatures, const bool &ignoreProtocolVersion, const quint16 &sessionId, QObject *parent = nullptr);

    bool isWritten2SocketJSON(const QJsonObject &json, const quint16 &s_command);

    qint64 mWrite2SocketJSON(QJsonObject json, const quint16 &s_command, int lastHashSumm);


    bool isWritten2SocketQDS(const QVariant &var, const quint16 &s_command);

    qint64 mWrite2SocketQDS(const QVariant &var, const quint16 &s_command);


    M2MreadResult mReadyReadF();


signals:
    void dataFromCoordinator(quint16 sessionId, QString socketStamp, QByteArray arr);

    void onYouCanSelectDevice(quint16 sessionId, QStringList listofdevices);


    void onM2MSocketReady(quint16 sessionId, QString socketStamp);//onSvahaSocketReady


    //4 meter list
//    void uploadProgress(quint8 sessionId, int val, QString txt);

    void onCOMMAND2GUI(quint16 sessionId, quint16 command, QVariantHash varHash);








public slots:
    void conn2thisDev(QString objN, QString login, QString passwd, QString add, quint16 port, int timeoutMsec, bool add2list, bool useMac, QString macAddr, bool useMacAddr2conn, int daChannelNumber);

    void startConnection();

    void openDirectAccessConnection();

    void mReadyRead();

private:
    bool createAconnection();


    bool isAble2connectViaM2Mservice();

    bool isAble2authorize();



    M2MreadResult processJSONread(const M2MreadMethod &readval);

    M2MreadResult processQDSread(M2MreadMethod readval);


};

#endif // M2MTCPSOCKET_H
