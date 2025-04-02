#ifndef M2MCONNECTOR_H
#define M2MCONNECTOR_H

#include <QObject>
#include <QMutex>
#include <QVariantHash>


class M2MConnector : public QObject
{
    Q_OBJECT
public:
    explicit M2MConnector(QObject *parent = nullptr);

signals:

};

#endif // M2MCONNECTOR_H
