#ifndef CONF2MODEMHELPER_H
#define CONF2MODEMHELPER_H

#include <QVariantHash>
#include <QStringList>


class conf2modemHelper
{
public:
    static QVariantHash aboutZigBeeModem2humanReadable(const QVariantHash &aboutModem);

};

#endif // CONF2MODEMHELPER_H
