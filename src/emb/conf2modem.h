#ifndef CONF2MODEM_H
#define CONF2MODEM_H

#include "conn2modem.h"

///[!] ifaces
#include "src/emb/ifaceexchangeserializedtypes.h"
#include "src/emb/embnodediscoveryconverter.h"


//class Conf2modem : public QObject
//{

//#include "conf2modem_global.h"



class Conf2modem : public Conn2modem
{
    Q_OBJECT
public:
    explicit Conf2modem(const int &dbgExtSrcId, const bool &verboseMode, QObject *parent = Q_NULLPTR);

    struct RezUpdateConnSettings
    {
        ZbyrConnSett connSett;
        QString ifaceParams;
        RezUpdateConnSettings() {}
        RezUpdateConnSettings(const ZbyrConnSett &connSett, const QString &ifaceParams) : connSett(connSett), ifaceParams(ifaceParams) {}
    };

    static RezUpdateConnSettings convertFromVarMap(const QVariantMap &interfaceSettings);

    static RezUpdateConnSettings convertFromVarMapExt(const QVariantMap &interfaceSettings, ZbyrConnSett connSett);

    bool openAconnection(const ZbyrConnSett &connSett, QString &connline);


    bool enterCommandMode(const QString &operationname);

    bool enterCommandMode();

    bool networkReset(QString &errStr);

    bool resetAmodem(QString &errStr);

    bool factorySettings(QString &errStr);

    bool readAboutModem(QVariantMap &atcommands, QString &errStr);

    bool nodeDiscovery(const int &totalModemCount, const qint64 &totalMsecElapsed, const qint64 &totalMsecLimt, const bool &hardRecovery, int &modemReady, QStringList &listreadynis, QVariantMap &ndtParams, QString &errStr);


    bool writeCommands2aModem(const QStringList &lcommands, QString &errStr);


    bool writeSomeCommand(const QString &atcommand, const bool &enterTheCommandMode, const bool &exitCommandMode, const bool &atfrAtTheEnd, const QString &operationName, QString &errStr);

    bool writeSomeCommand(const QStringList &list2write, const bool &enterTheCommandMode, const bool &exitCommandMode, const bool &atfrAtTheEnd, const QString &operationName, QString &errStr);


    bool wait4doubleOk(const bool &isAtlbCommand, const bool &ignoreSecondErr);


    QMap<QString,QString> getTheModemInfo(const QString &atcommand, const bool &exitCommandMode, const bool &atfrAtTheEnd, const QString &operationName, QString &errStr);

    QMap<QString,QString> getTheModemInfo(const QStringList &list2read, const bool &exitCommandMode, const bool &atfrAtTheEnd, const QString &operationName, QString &errStr);

    bool enableDisableApi(const bool &enable, const bool &readAboutZigBee);


    bool isCoordinatorGood(const bool &forced, const bool &readAboutZigBee);


    QVariantHash addCurrentDate2aboutModem(QVariantHash &aboutModem);

    QStringList processNdtLine(QString &line);

    int deocodeNtdOut(const QStringList &list, const bool &allowHard, bool &need2reenterTheCommandMode, QStringList &listreadynis);


    int decodeNtdOneLineHard(const QString &line, const qint64 &msec, QMap<QString, QString> &brokenStts, QStringList &listreadynis);

    bool decodeNtdOneLine(const QString &line, const qint64 &msec, const bool &wasRestored, QStringList &listreadynis);


#ifdef ENABLE_EMBEEMODEM_EXTENDED_OPERATIONS

    bool quickRadioSetupExt(const QVariantMap &insettings, QString &errstr);

    bool isCoordinatorReady4quickRadioSetupExt(const QVariantMap &insettings, QMap<QString,QString> &mapAboutTheModem, QString &errstr);

    QStringList sendNetworParams(const QStringList &listni, const QVariantMap &insettings);

    bool applyNewNetworkSettings(const QStringList &listni, const QVariantMap &insettings, const QMap<QString,QString> &mapAboutTheCoordinator);

    bool changeni(const QString &from, const QString &toni, const bool &sendatlb5);


#endif


signals:

//    void onDaStateChanged(bool isdamodenow);

//    void onLowPriority2uart();
    void onAboutZigBee(QVariantHash hash);

    void onApiModeEnabled();


    void atCommandRez(QString atcommand, QString rez);

    void ndtFounNewDev(qint64 msec, QString devtype, QString sn, QString ni, bool wasRestored);//only for new devices
    void ndtFounADev(qint64 msec, QString devtype, QString sn, QString ni, bool wasRestored);//for all devices, but they must be routers

#ifdef ENABLE_EMBEEMODEM_EXTENDED_OPERATIONS

    void qrsStatus(qint64 msec, QStringList listniready, QStringList listniall,
                   QString channelold, QString idold, QString keyold,
                   QString channelnew, QString idnew, QString keynew,
                   bool writeold, bool changeni, bool qrsmulticastmode, bool ignoreCoordinatorSett, QVariantMap aboutcoordinator, QVariantMap insettings);

#endif

public slots:





private:
#ifdef ENABLE_EMBEEMODEM_EXTENDED_OPERATIONS
    struct EmbeeNetworkParamsStr
    {
        QString chhex;
        QString idhex;
        QString key;
        EmbeeNetworkParamsStr() {}
        EmbeeNetworkParamsStr(const QString &chhex, const QString &idhex, const QString &key) : chhex(chhex), idhex(idhex), key(key.left(32)) {}
    };

    EmbeeNetworkParamsStr convert2netParams(const int &channel, const int &id, const QString &key);

    EmbeeNetworkParamsStr convert2netParams(const QVariantMap &map, const QString &keysChIdKy);

    EmbeeNetworkParamsStr convert2netParams(const QVariantMap &map, const QStringList &lkeysChIdKy);

#endif


//    bool enExtIface;
//    bool disableUartPrtt;

};

#endif // CONF2MODEM_H
