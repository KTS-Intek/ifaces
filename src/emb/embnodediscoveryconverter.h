#ifndef EMBNODEDISCOVERYCONVERTER_H
#define EMBNODEDISCOVERYCONVERTER_H

#include <QVariantMap>

#include "embnodediscoverytypes.h"

class EmbNodeDiscoveryConverter
{

public:
    static QString getSettError(const AtndOneModeSettStruct &sett);

    static QVariantMap modeSettToMap(const AtndOneModeSettStruct &sett);

    static AtndOneModeSettStruct modeSettToStruct(const QVariantMap &map);


    static NodeDiscoveryLimits getLimitsFromAMap(const QVariantMap &lmts);

    static NodeDiscoveryStruct resetStruct(const QStringList &listOldNis, const QVariantMap &atndmap);

    static bool isTheRuleAcceptsThisNI(const AtndOneModeSettStruct &sett, const QString &ni);
};

#endif // EMBNODEDISCOVERYCONVERTER_H
