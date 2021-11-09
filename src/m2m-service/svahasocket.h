#ifndef SVAHASOCKET_H
#define SVAHASOCKET_H

#include <QObject>
#include <QTcpSocket>
#include <QtCore>

//it is ugly and it looks like shit, but it works)

class SvahaSocket : public QTcpSocket
{
        Q_OBJECT
public:
    explicit SvahaSocket(const quint8 &sesionId, const quint16 &m2mDAchannel, const bool &verboseMode, QObject *parent = nullptr);

    bool verboseMode;

    QMap<QString,QStringList> key2header;

    void setConnectionSett(const QString &host, const quint16 &port, const QString &objIdOrMac, const bool &cmMAC, const int &timeOut, const int &timeOutB,
                           const QString &login, const QString &pas, const bool &useMac);


    void setDoAfterConn(const int &command, const QVariantHash &s_data);

    static QString getErrMess(const int &code, const QString &commandName);

signals:


    void dataFromCoordinator(quint8 sessionId, QString socketStamp, QByteArray arr);

    void onYouCanSelectDevice(quint8 sessionId, QStringList list);

    void add2systemLog(quint8 sessionId, QString line);

//    void startWait4AnswerTimer(int);
//    void stopWait4AnswerTimer();

    void onSvahaSocketReady(quint8 sessionId, QString socketStamp);

    void onSocketKilled(quint8 sessionId, QString socketStamp);

    //4 meter list
    void uploadProgress(quint8 sessionId, int val, QString txt);

    void onCOMMAND2GUI(quint8 sessionId, quint16 command, QVariantHash varHash);

    void showMessage(quint8 sessionId, QString messageStrr);

    void appendLedLampListVar(quint8 sessionId, QVariantList listvar);

public slots:
    void startConnection();

    void stopAllSlot();

    void killSocketById(QString ignoreThisStamp);

    void data2coordiantor(QByteArray writeArr);
    void onDaOpened(bool isDaOpen);

    void killOldSockets(quint8 sessionId);

    void setHeaders4map(QStringList keys, QVariantList headers);

    void setHeader4map(QString key, QStringList header);

private slots:
    void onSocketReady();

    void decodeReadData(const QVariant &dataVar, const quint16 &command);

    void decodeReadDataJSON(const QByteArray &dataArr);


    void mWriteToSocket(const QVariant s_data, const quint16 s_command);

    void onDisconn();

    void mReadyRead();

    void openDirectAccess();

    void onZombeTo();

private:
    void mReadyReadF();

    int onCOMMAND_ERROR_CODE(const QVariantHash &h, bool &rezIsGood);

    int onCOMMAND_ERROR_CODE_EXT(const QVariantHash &h, bool &rezIsGood);



    int onCOMMAND_READ_METER_LIST_FRAMED(const QVariantHash &h, bool &rezIsGood);

    int onCOMMAND_WRITE_METER_LIST_FRAMED(const QVariantHash &h, bool &rezIsGood, const bool firstWrite = false);


    int onCOMMAND_READ_LEDLAMPLIST_FRAMED(const QVariantHash &h, bool &rezIsGood);

    int onCOMMAND_WRITE_LEDLAMPLIST_FRAMED(const QVariantHash &h, bool &rezIsGood, const bool firstWrite = false);

    int readFramedList(const QVariantHash &h, bool &rezIsGood, const QStringList &humanHeaders, const QStringList &lk, const quint16 &command, const QString &listName);


    QVariantHash errCodeLastOperation(const quint16 &command, const int &errCode) const;

    bool messHshIsValid(const QJsonObject &jObj, QByteArray readArr);

    QStringList getHshNames() const;
    qint64 mWrite2SocketJSON(QJsonObject jObj, const quint16 s_command, int lastHashSumm);

    QVariant uncompressRead(QByteArray readArr, quint16 &command, qint64 lenBefore);

    QByteArray varHash2arr(const QVariantHash &hash);

    quint8 accessLevel;


    struct connSettSvaha{//0 - not loaded, 1 - load, 2 - loaded
        QString host;
        quint16 port;
        QString objIdOrMac;
        bool cmMAC;
        int timeOut;
        int timeOutB;
        QString login;
        QString password;
        bool useMac;

    } socketSett;
    QString socketStamp;
    bool ignoreDisconn;
    bool isActiveClient;

    bool stopAll, stopAllDirect, stopAfter, daOpened;
    QList<QByteArray> loginPasswd;
    int dataStreamVersion;

    bool lastUseMacAdd2conn;
    QString lastMacOrObjId;

    bool ignoreThisDisconn;
    bool isSvahaService;
    bool disconAfterAnswer;
    QTime timeGalmo;
    bool matildaLogined;
    bool block4activeClient;
    bool iAmDone, noWriteData;
    quint8 sessionId;
    quint16 m2mDAchannel;
    quint8 zombieRetr;

    //4 meter list
    QVariant lastWriteVar, lastWriteVarZero;
    int totalTables, doneTables, blockDone;
    QHash<int, QVariantHash> hashMemoWrite;


};

#endif // SVAHASOCKET_H
