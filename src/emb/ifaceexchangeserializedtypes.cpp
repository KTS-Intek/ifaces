#include "ifaceexchangeserializedtypes.h"


void IfaceExchangeSerializedTypes::makeRegistration()
{
    //call this function once, before using this types
    if(!QMetaType::isRegistered(QMetaType::type("ZbyrConnSett"))){
        qRegisterMetaTypeStreamOperators<ZbyrConnSett>("ZbyrConnSett");



    }
}
//must be in a cpp file!!!
QDataStream &operator <<(QDataStream &out, const ZbyrConnSett &m)
{
    return out << m.prdvtrAddr << m.prdvtrPort << m.timeOutB << m.timeOutG;
}

QDataStream &operator >>(QDataStream &in, ZbyrConnSett &m)
{
    in >> m.prdvtrAddr >> m.prdvtrPort >> m.timeOutB >> m.timeOutG;
    return in;
}

QDebug operator<<(QDebug d, const ZbyrConnSett &m)
{
    d << m.prdvtrAddr << m.prdvtrPort << m.timeOutB << m.timeOutG;
    return d;
}



