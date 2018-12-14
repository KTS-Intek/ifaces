#ifndef SVAHASERVICECONNECTOR_H
#define SVAHASERVICECONNECTOR_H

#include <QObject>
#include <QMutex>
#include <QVariantHash>

class SvahaServiceConnector : public QObject
{
    Q_OBJECT
public:
    explicit SvahaServiceConnector(QObject *parent = 0);

    ~SvahaServiceConnector();

    static QVariantHash defSvahaIps();

    QString errorString() const;

    QStringList getLastSelDevList() const;

    QString getLastConnDev() const;

    void connect2hostViaSvaha(QVariantHash oneProfile, const int &timeOut, const int &timeOutB);

    void setDoAfterConn(const int &command, const QVariantHash &s_data);


    bool waitForConnected(const int &msec);

    bool waitForReadyRead(const int &msec);
    bool waitForBytesWritten(const int &msec);

    qint64 write(const QByteArray &arr);

    void clear();

    void close();

    bool isOpen() const;

    bool isReadable() const;

    void setAsyncMode(bool isAsyn);

    QByteArray readAll();
signals:

    void disconnected();

    void stopAllSignal();

    void killSocketById(QString ignoreThisStamp);

    void write2svahaSocket(QByteArray arr);

    void killOldSockets(quint8 sessionId);

    void add2systemLog(QString line);

    void readyRead();


    //4 meter list
    void uploadProgressS(int val, QString txt);

    void onCOMMAND2GUIS(quint16 command, QVariantHash varHash);
    void appendLedLampListVar(QVariantList listvar);

    void showMess(QString mess);

    void onConnectionWorks(bool works);

    void kickOffThread();

public slots:
    void stopAllSlot();

    void stopAllSlotDirect();


    void onSvahaSocketRead(quint8 sessionId, QString socketStamp, QByteArray arr);

    void onSvahaSocketDisconn(quint8 sessionId, QString socketStamp);

    void onSvahaSocketReady(quint8 sessionId, QString socketStamp);

    void add2systemLogSlot(quint8 sessionId, QString line);

    void onYouCanSelectDevice(quint8 sessionId, QStringList list);



    //4 meter list
    void uploadProgress(quint8 sessionId, int val, QString txt);

    void onCOMMAND2GUI(quint8 sessionId, quint16 command, QVariantHash varHash);

    void appendLedLampListVar(quint8 sessionId, QVariantList listvar);

    void startWait4connInThread();

    void softDelete();

    void showMessS(quint8 sessionId, QString mess);


private:
    bool needInit;
    bool useAsynMode;
    bool oneConnReady;
    bool stopAll;
    bool socketKilled;
    int socketCounter;
    quint8 sessionId;
    QString lastSocketStamp;
    QString lastConnDev;

    QByteArray readArr;
    QMutex mutex, mutex4stamp;

    QString lastErrorStr;
    QStringList selectList;

    QHash<int, QVariantHash> hashMemoWrite;

};

#endif // SVAHASERVICECONNECTOR_H
