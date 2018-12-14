#include "ifaceexchangeserializedtypes.h"


void IfaceExchangeSerializedTypes::makeRegistration()
{
    //call this function once, before using this types
    if(!QMetaType::isRegistered(QMetaType::type("ZbyrConnSett"))){
        qRegisterMetaTypeStreamOperators<ZbyrConnSett>("ZbyrConnSett");
        qRegisterMetaTypeStreamOperators<UniversalMeterSett>("UniversalMeterSett");
        qRegisterMetaTypeStreamOperators<MeterTransformer>("MeterTransformer");
        qRegisterMetaTypeStreamOperators<UniverslaMeterOnlyCache>("UniverslaMeterOnlyCache");
        qRegisterMetaTypeStreamOperators<PollDateMemoExt>("PollDateMemoExt");


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



QDataStream &operator <<(QDataStream &out, const UniversalMeterSett &m)
{
    return out << m.meterType << m.model << m.sn << m.ni << m.memo << m.passwd << m.pollEnbl << m.enrg << m.tariff << m.coordinate << m.version << m.powerin << m.transformer << m.disableTimeCorrection
               << m.cache;
}

QDataStream &operator >>(QDataStream &in, UniversalMeterSett &m)
{
    in >> m.meterType >> m.model >> m.sn >> m.ni >> m.memo >> m.passwd >> m.pollEnbl >> m.enrg >> m.tariff >> m.coordinate >> m.version >> m.powerin >> m.transformer >> m.disableTimeCorrection
                   >> m.cache;
    return in;
}

QDebug operator<<(QDebug d, const UniversalMeterSett &m)
{
    d << m.meterType << m.model << m.sn << m.ni << m.memo << m.passwd << m.pollEnbl << m.enrg << m.tariff << m.coordinate << m.version << m.powerin << m.transformer << m.disableTimeCorrection
               << m.cache;
    return d;
}


bool operator ==(const MeterTransformer &t0, const MeterTransformer &t1)
{
    return (t0.endCoefitient == t1.endCoefitient && t0.endIcoefitient == t1.endIcoefitient && t0.endUcoefitient == t1.endUcoefitient && t0.hasTransformer == t1.hasTransformer &&
            t0.icDividend == t1.icDividend && t0.icDivisor == t1.icDivisor && t0.only4display == t1.only4display && t0.ucDividend == t1.ucDividend && t0.ucDivisor == t1.ucDivisor);
}

bool operator ==(const UniversalMeterSett &s, const UniversalMeterSett &s1)
{
    return (s.cache.enrg4vrsn == s1.cache.enrg4vrsn && s.coordinate == s1.coordinate && s.disableTimeCorrection == s1.disableTimeCorrection && s.enrg == s1.enrg && s.memo == s1.memo &&
            s.meterType == s1.meterType && s.model == s1.model && s.ni == s1.ni && s.passwd == s1.passwd && s.pollEnbl == s1.pollEnbl && s.powerin == s1.powerin && s.sn == s1.sn &&
            s.tariff == s1.tariff && s.transformer == s1.transformer && s.version == s1.version);
}

//bool hasTransformer;
//bool only4display;
//quint32 ucDividend;
//quint32 ucDivisor;
//quint32 icDividend;
//quint32 icDivisor;

//qreal endUcoefitient;
//qreal endIcoefitient;
//qreal endCoefitient;

QDataStream &operator <<(QDataStream &out, const MeterTransformer &m)
{
    return out << m.hasTransformer << m.only4display << m.ucDividend << m.ucDivisor << m.icDividend << m.icDivisor << m.endUcoefitient << m.endIcoefitient << m.endCoefitient;
}


QDebug operator<<(QDebug d, const MeterTransformer &m)
{
    d << m.hasTransformer << m.only4display << m.ucDividend << m.ucDivisor << m.icDividend << m.icDivisor << m.endUcoefitient << m.endIcoefitient << m.endCoefitient;
    return d;
}

QDataStream &operator >>(QDataStream &in, MeterTransformer &m)
{
    in >> m.hasTransformer >> m.only4display >> m.ucDividend >> m.ucDivisor >> m.icDividend >> m.icDivisor >> m.endUcoefitient >> m.endIcoefitient >> m.endCoefitient;
    return in;
}



QDataStream &operator <<(QDataStream &out, const UniverslaMeterOnlyCache &m)
{
    return out << m.enrg4vrsn;
}

QDataStream &operator >>(QDataStream &in, UniverslaMeterOnlyCache &m)
{
    in  >> m.enrg4vrsn;
    return in;

}

QDebug operator<<(QDebug d, const UniverslaMeterOnlyCache &m)
{
    d << m.enrg4vrsn;
    return d;
}



QDataStream &operator <<(QDataStream &out, const PollDateMemoExt &m)
{
    return out << m.depth << m.pollCode << m.tableName << m.fullTableName << m.ignorePrevData << m.pollDateTime << m.pollDateTimeFirstIntrvl << m.pollDateTimeNextIntrvl << m.dtLastStartPoll << m.operationArgument;
}

QDataStream &operator >>(QDataStream &in, PollDateMemoExt &m)
{
    in >> m.depth >> m.pollCode >> m.tableName >> m.fullTableName >> m.ignorePrevData >> m.pollDateTime >> m.pollDateTimeFirstIntrvl >> m.pollDateTimeNextIntrvl >> m.dtLastStartPoll >> m.operationArgument;
    return in;
}

QDebug operator<<(QDebug d, const PollDateMemoExt &m)
{
    d << m.depth << m.pollCode << m.tableName << m.fullTableName << m.ignorePrevData << m.pollDateTime << m.pollDateTimeFirstIntrvl << m.pollDateTimeNextIntrvl << m.dtLastStartPoll << m.operationArgument;
    return d;
}

