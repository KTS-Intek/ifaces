#include "ifaceexchangeserializedtypes.h"

#ifdef HASNETWORKQTLIB
#include <QHostAddress>
#endif

#include <QUrl>

void IfaceExchangeSerializedTypes::makeRegistration()
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)

    //call this function once, before using these types
    if(!QMetaType::isRegistered(QMetaType::type("ZbyrConnSett"))){
        qRegisterMetaTypeStreamOperators<ZbyrConnSett>("ZbyrConnSett");



    }

    if(!QMetaType::isRegistered(QMetaType::type("QHash<QString,QString>"))){
        qRegisterMetaType<QHash<QString,QString> >("QHash<QString,QString>");

    }
#endif
}

ZbyrConnSett IfaceExchangeSerializedTypes::getZbyrConnSettFromArgs(const QStringList &list, const bool &verboseMode, bool &ok)
{
    ok = true;
    ZbyrConnSett connSett;
    if(true){
        const int listSize = list.size();
        bool allowCheckSett = true;

        QString remAddr("");
         quint16 remPort = 0;
        if(list.contains("-p")){
            const int i = list.indexOf("-p") + 1;
            bool valGood = false;
            if(i < listSize){
                remPort = list.at(i).toInt(&valGood);
                if(valGood && remPort > 2999 && remPort < 65535)
                    valGood = true;
            }
            if(!valGood){
                if(verboseMode)
                    qDebug() << "Invalid argument -p";
                ok = false;
                return connSett;
            }
            allowCheckSett = false;
            connSett.prdvtrPort = remPort;
        }


        if(list.contains("-s")){
            int i = list.indexOf("-s");
            i++;
            bool valGood = false;
            if(i < listSize){
                remAddr = list.at(i).simplified().trimmed();
#ifdef HASNETWORKQTLIB
                const QHostAddress haddr(remAddr);
                if(!remAddr.isEmpty() && remAddr == haddr.toString()){
                    valGood = true;
                }else{
                    const QUrl url(remAddr);
                    if(url.isValid() && !remAddr.isEmpty())
                        valGood = true;
                    if(verboseMode)
                        qDebug() << "remAddr=" << remAddr << haddr.toString() << haddr;
                }
#endif
            }
            if(!valGood){
                if(verboseMode)
                    qDebug() << "Invalid argument -s" << remAddr ;
                ok = false;
                return connSett;
            }
            allowCheckSett = false;
            connSett.prdvtrAddr = remAddr;
        }

        if(!allowCheckSett){
            connSett.timeOutG = 11111;
            connSett.timeOutB = 333;
        }

    }
    return connSett;
}

EMBZombieExchangeTypes IfaceExchangeSerializedTypes::getDefaultValues4zombie(const DeviceTimeouts &timeouts)
{
    EMBZombieExchangeTypes t;
    t.timeouts = timeouts;
    return t;
}

//must be in a cpp file!!!
QDataStream &operator <<(QDataStream &out, const ZbyrConnSett &m)
{
    return out << m.prdvtrAddr << m.prdvtrPort << m.timeOutB << m.timeOutG;
}

QDataStream &operator >>(QDataStream &in, ZbyrConnSett &m)
{
    in >> m.prdvtrAddr >> m.prdvtrPort >> m.timeOutB >> m.timeOutG;
    return in;
}

QDebug operator<<(QDebug d, const ZbyrConnSett &m)
{
    d << m.prdvtrAddr << m.prdvtrPort << m.timeOutB << m.timeOutG;
    return d;
}



