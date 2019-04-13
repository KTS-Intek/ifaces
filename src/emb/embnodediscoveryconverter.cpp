#include "embnodediscoveryconverter.h"

#include <QDateTime>
#include <QRegularExpression>

#include "matildalimits.h"

//------------------------------------------------------------------------------

QString EmbNodeDiscoveryConverter::getSettError(const AtndOneModeSettStruct &sett)
{
    QStringList errs;


    if(sett.minnilen > sett.maxnilen)
        errs.append(QObject::tr("The minimum NI length can't be more than the maximum NI length"));

    if(!sett.pattern.isEmpty()){
        QRegularExpression r(sett.pattern);
        if(!r.isValid())
            errs.append(QObject::tr("Bad pattern '%1'").arg(r.errorString()));

    }
    return errs.join("\n");
}

//------------------------------------------------------------------------------

QVariantMap EmbNodeDiscoveryConverter::modeSettToMap(const AtndOneModeSettStruct &sett)
{
    QVariantMap map;
    map.insert("mnln", sett.minnilen);
    map.insert("mxln", sett.maxnilen);
    map.insert("lc", sett.acceptlowercase);
    map.insert("uc", sett.acceptuppercase);
    map.insert("pttrn", sett.pattern);
    return map;
}

//------------------------------------------------------------------------------

AtndOneModeSettStruct EmbNodeDiscoveryConverter::modeSettToStruct(const QVariantMap &map)
{
    AtndOneModeSettStruct sett;
    sett.minnilen = qMax(0, map.value("mnln", 0).toInt());
    sett.maxnilen = qMax(sett.minnilen, map.value("mxln", 32).toInt());

    sett.acceptlowercase = map.value("lc", true).toBool();
    sett.acceptuppercase = map.value("uc", false).toBool();
    sett.pattern = map.value("pttrn").toString().simplified().trimmed();
    sett.lasterror = getSettError(sett);

    return sett;
}

//------------------------------------------------------------------------------

NodeDiscoveryLimits EmbNodeDiscoveryConverter::getLimitsFromAMap(const QVariantMap &lmts)
{
    NodeDiscoveryLimits limits;
    limits.limn = qMin(qMax(1, lmts.value("limn").toInt()), MAX_METER_COUNT) ;
    limits.limt = qMin(qMax(1, lmts.value("limt").toInt()), MAX_METER_COUNT) ;
    limits.minutesDuration = qMin(qMax(1, lmts.value("mnts").toInt()), 1440) ;
    return limits;
}

//------------------------------------------------------------------------------

NodeDiscoveryStruct EmbNodeDiscoveryConverter::resetStruct(const QStringList &listOldNis, const QVariantMap &atndmap)
{
    NodeDiscoveryStruct lastNodediscoveryScan;

    lastNodediscoveryScan.state.olddevnis = listOldNis;
    lastNodediscoveryScan.state.totalDevCountAtTheBegining = listOldNis.size();
    lastNodediscoveryScan.state.scanStarted = (atndmap.value("msec").toLongLong() > 0) ? atndmap.value("msec").toLongLong() : QDateTime::currentMSecsSinceEpoch();

    if(atndmap.isEmpty())
        return lastNodediscoveryScan;


#ifdef ENABLE_EXTSUPPORT_OF_IFACES
    lastNodediscoveryScan.enAutoExport = atndmap.value("autoexp", true).toBool();
#else
    lastNodediscoveryScan.enAutoExport = true;
#endif

    lastNodediscoveryScan.limits = getLimitsFromAMap(atndmap.value("lmts").toMap());
    lastNodediscoveryScan.sett = modeSettToStruct(atndmap.value("sett").toMap());

    lastNodediscoveryScan.mdls = atndmap.value("mdls").toString().split(" ", QString::SkipEmptyParts);
    lastNodediscoveryScan.defs = atndmap.value("defs").toMap();
    return lastNodediscoveryScan;

}

//------------------------------------------------------------------------------

bool EmbNodeDiscoveryConverter::isTheRuleAcceptsThisNI(const AtndOneModeSettStruct &sett, const QString &ni)
{
    const int nilen = ni.length();
    if(nilen > sett.maxnilen || nilen < sett.minnilen)
        return false;


    if(!sett.pattern.isEmpty()){
        QRegularExpression r(sett.pattern);
        if(r.isValid())
            return ni.contains(r);

    }

    if(sett.acceptlowercase && ni == ni.toLower())
        return true;

    if(sett.acceptuppercase && ni == ni.toUpper())
        return true;

    if(!sett.acceptlowercase && !sett.acceptuppercase)
        return true;//nothing to check

    return false;

}

//------------------------------------------------------------------------------
