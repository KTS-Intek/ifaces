#ifndef M2MCONNECTORBASE_H
#define M2MCONNECTORBASE_H

#include <QObject>

class M2MConnectorBase : public QObject
{
    Q_OBJECT
public:
    explicit M2MConnectorBase(QObject *parent = nullptr);

signals:

};

#endif // M2MCONNECTORBASE_H
