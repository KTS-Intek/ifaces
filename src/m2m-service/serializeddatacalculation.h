#ifndef SERIALIZEDDATACALCULATION_H
#define SERIALIZEDDATACALCULATION_H

#include <QVariant>

class SerializedDataCalculation
{

public:


    static int checkMessSize(const QVariant s_data, const quint16 s_command);

    static qint64 chkMessSize(const QVariant &s_data);

};

#endif // SERIALIZEDDATACALCULATION_H
