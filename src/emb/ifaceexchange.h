#ifndef IFACEEXCHANGE_H
#define IFACEEXCHANGE_H

#include <QHash>

class IfaceExchange
{

public:

    static void showHexDump(const QByteArray arr, const QString mask);

    static bool unikalnistPovidomlenVnormi(const QHash<QByteArray,quint32> &hashMessRetryCounter);

    static QString byte2humanRead(const quint32 &byte);


};

#endif // IFACEEXCHANGE_H
