#ifndef MATILDAMESSAGES_H
#define MATILDAMESSAGES_H

#include <QObject>

class MatildaMessages : public QObject
{
    Q_OBJECT
public:
    explicit MatildaMessages(QObject *parent = nullptr);

    static QString messFromCode(const int &messCode);

    static QString addRez2endOfLine(const QString m, const bool &rezGood);

    static QString name4command(const quint16 &command);

};

#endif // MATILDAMESSAGES_H
