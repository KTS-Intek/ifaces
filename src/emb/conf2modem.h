#ifndef CONF2MODEM_H
#define CONF2MODEM_H

#include "conn2modem.h"
#include "src/emb/ifaceexchangeserializedtypes.h"


//class Conf2modem : public QObject
//{

//#include "conf2modem_global.h"

struct OneEmbeeModemSmpl
{
    QString typestr;
    QString eui64;
    QString ni;
    bool wasRestored;
    qint64 lastmsec;
    OneEmbeeModemSmpl() {}
};

class Conf2modem : public Conn2modem
{
    Q_OBJECT
public:
    explicit Conf2modem(const int &dbgExtSrcId, const bool &verboseMode, QObject *parent = Q_NULLPTR);

    bool openAconnection(const ZbyrConnSett &connSett, QString &connline);


    bool enterCommandMode(const QString &operationname);

    bool enterCommandMode();

    bool networkReset(QString &errStr);

    bool resetAmodem(QString &errStr);

    bool factorySettings(QString &errStr);

    bool readAboutModem(QVariantMap &atcommands, QString &errStr);

    bool nodeDiscovery(const int &modemsLimit, const bool &hardRecovery, int &modemReady, QVariantMap &ndtParams, QString &errStr);


    bool writeCommands2aModem(const QStringList &lcommands, QString &errStr);


    bool writeSomeCommand(const QString &atcommand, const bool &enterTheCommandMode, const bool &exitCommandMode, const bool &atfrAtTheEnd, const QString &operationName, QString &errStr);

    bool writeSomeCommand(const QStringList &list2write, const bool &enterTheCommandMode, const bool &exitCommandMode, const bool &atfrAtTheEnd, const QString &operationName, QString &errStr);


    QMap<QString,QString> getTheModemInfo(const QString &atcommand, const bool &exitCommandMode, const bool &atfrAtTheEnd, const QString &operationName, QString &errStr);

    QMap<QString,QString> getTheModemInfo(const QStringList &list2read, const bool &exitCommandMode, const bool &atfrAtTheEnd, const QString &operationName, QString &errStr);

    bool enableDisableApi(const bool &enable, const bool &readAboutZigBee);


    bool isCoordinatorGood(const bool &forced, const bool &readAboutZigBee);


    QVariantHash addCurrentDate2aboutModem(QVariantHash &aboutModem);

    QStringList processNdtLine(QString &line);

    int deocodeNtdOut(const QStringList &list, const bool &allowHard, bool &need2reenterTheCommandMode, QStringList &listreadynis);


    int decodeNtdOneLineHard(const QString &line, const qint64 &msec, QMap<QString, QString> &brokenStts, QStringList &listreadynis);

    bool decodeNtdOneLine(const QString &line, const qint64 &msec, const bool &wasRestored, QStringList &listreadynis);

signals:

//    void onDaStateChanged(bool isdamodenow);

//    void onLowPriority2uart();
    void onAboutZigBee(QVariantHash hash);

    void onApiModeEnabled();


    void atCommandRez(QString atcommand, QString rez);

    void ndtFounNewDev(qint64 msec, QString devtype, QString sn, QString ni, bool wasRestored);

public slots:



private:



//    bool enExtIface;
//    bool disableUartPrtt;

};

#endif // CONF2MODEM_H
