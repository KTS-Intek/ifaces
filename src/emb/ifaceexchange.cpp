#include "ifaceexchange.h"
#include <QTime>
#include <QDebug>
#include "matildalimits.h"



void IfaceExchange::showHexDump(const QByteArray arr, const QString mask)
{
    QString strNorm(arr);
    const QString emptyMask = QString("").rightJustified(mask.length() + 10, ' ');


    for(int ff = 0, ffMax = arr.size(); ff < ffMax; ff += 16){

        QString hexStr = arr.mid(ff,16).toHex().leftJustified(32, ' ').toUpper();
        for(int i = 30; i > 1; i -= 2)
            hexStr = hexStr.insert(i, " ");

        if(ff == 0)
            hexStr.prepend(mask + QTime::currentTime().toString("hh:mm:ss.zzz")+" ");
        else
            hexStr.prepend(emptyMask);

        qDebug() << hexStr << "   " << strNorm.mid(ff, 16).simplified();
    }
}

bool IfaceExchange::unikalnistPovidomlenVnormi(const QHash<QByteArray, quint32> &hashMessRetryCounter)
{
    const QList<quint32> l = hashMessRetryCounter.values();

    for(quint32 i = 0, iMax = l.size(); i < iMax; i++){
        if(l.at(i) > SETT_MAX_IDENTICAL_MESS_PER_SESION)
            return false;
    }
    return true;
}

QString IfaceExchange::byte2humanRead(const quint32 &byte)
{
    if(byte > 10000000)
        return QString::number(byte / 1000000) + "M";

    if(byte > 10000)
        return QString::number(byte / 1000) + "K";

    return QString::number(byte);
}
