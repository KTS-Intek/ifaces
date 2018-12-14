#include "serializeddatacalculation.h"

#include <QDataStream>
#include <QDebug>

#include "moji_defy.h"
#include "matildalimits.h"

int SerializedDataCalculation::checkMessSize(const QVariant s_data, const quint16 s_command)
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_6); //Qt_4_0);
    out << (quint32)0;
    out << s_command << s_data;

    out.device()->seek(0);

    quint32 blSize = block.size();
    quint32 blSizeOld = blSize;

    if(blSize < SETT_MAX_UNCOMPRSS_PACkET_SIZE || s_command == COMMAND_WRITE_UPGRADE || s_command == COMMAND_COMPRESSED_STREAM){
        out << (quint32)(blSize - sizeof(quint32));
        qDebug() << "===blSize " << blSize << blSizeOld << s_command;
        return block.size();

    }else{
         blSizeOld = blSize;
        qDebug() << "===blSize " << block.size() / 6   << blSizeOld << s_command;

        return block.size() / 6;
    }
}

qint64 SerializedDataCalculation::chkMessSize(const QVariant &s_data)
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_6); //Qt_4_0);
    out <<  s_data;
    return (qint64)block.size();
}
