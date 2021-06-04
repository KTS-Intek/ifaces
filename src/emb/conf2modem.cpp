#include "conf2modem.h"
#include <QTime>
#include <QThread>

#include "conf2modemhelper.h"
#include "embeelimits.h"

///[!] type-converter
#include "src/base/convertatype.h"
#include "src/shared/embeedefaultreadcommandlist.h"
#include "src/base/valuevalidator.h"

//-------------------------------------------------------------------------------------

Conf2modem::Conf2modem(const int &dbgExtSrcId, const bool &verboseMode, QObject *parent) : Conn2modem(dbgExtSrcId, verboseMode, parent)
{


}

Conf2modem::RezUpdateConnSettings Conf2modem::convertFromVarMap(const QVariantMap &interfaceSettings)
{
    ZbyrConnSett connSett;
    return convertFromVarMapExt(interfaceSettings, connSett);
}

Conf2modem::RezUpdateConnSettings Conf2modem::convertFromVarMapExt(const QVariantMap &interfaceSettings, ZbyrConnSett connSett)
{
    connSett.connectionType = interfaceSettings.value("ifaceMode").toInt();
    switch(connSett.connectionType){

    case IFACECONNTYPE_TCPCLNT:{
        connSett.prdvtrAddr = interfaceSettings.value("hostName", "::1").toString();
        connSett.prdvtrPort = interfaceSettings.value("port", 8989).toUInt();
        break;}

    case IFACECONNTYPE_M2MCLNT:{
        connSett.m2mhash = interfaceSettings.value("m2mProfile").toHash();
        break;}

    case IFACECONNTYPE_UART:{
        connSett.uarts = interfaceSettings.value("uarts").toStringList();
        connSett.prdvtrAddr = interfaceSettings.value("uart").toString();
        connSett.prdvtrPort = interfaceSettings.value("baudRate").toInt();
        break;}

    }
    QString ifaceParams;
    switch(connSett.connectionType){

    case IFACECONNTYPE_TCPCLNT :{
        ifaceParams = QString("%1\n%2").arg(connSett.prdvtrAddr).arg(connSett.prdvtrPort);
        break;}

    case IFACECONNTYPE_M2MCLNT :{
        QList<QString> lk = connSett.m2mhash.keys();
        lk.removeOne("t");//timeout
        std::sort(lk.begin(), lk.end());
        for(int i = 0, imax = lk.size(); i < imax; i++)
            ifaceParams.append(QString("%1\n%2\n\r\n").arg(lk.at(i)).arg(connSett.m2mhash.value(lk.at(i)).toString()));
        break;}

    default :{
        ifaceParams = QString("%1\n\n\n%2\n%3").arg(connSett.uarts.join("\r")).arg(connSett.prdvtrAddr).arg(connSett.prdvtrPort);
        break;}

    }
    connSett.timeOutG = interfaceSettings.value("timeoutMsec", 11000).toInt();
    connSett.timeOutB = interfaceSettings.value("blockTimeout", 300).toInt();

//    ui->cbxUnknownPrtcl->setChecked(h.value("unknownProtocolAsData").toBool());
//    ui->cbxRecoverNI->setChecked(h.value("recoverNI").toBool());
//    ui->cbxDisableAPIprocessing->setChecked(h.value("disableAPImode").toBool());

    connSett.try2recoverNI = interfaceSettings.value("recoverNI").toBool();// try2recoverNI;
    connSett.unknownProtocolAsData = interfaceSettings.value("unknownProtocolAsData").toBool();
    connSett.disableAPImode = interfaceSettings.value("disableAPImode").toBool();
    connSett.forceHrdAddrsn = interfaceSettings.value("forceHrdAddrsn").toBool();
    connSett.settext = interfaceSettings.value("settext");//m2mDAchannel 4 m2m connection
    connSett.databits = ValueValidator::validateIntegerRange("databits", interfaceSettings, 8, 5, 8);
    connSett.stopbits = ValueValidator::validateIntegerRange("stopbits", interfaceSettings, 1, 1, 2);
    connSett.parity = ValueValidator::validateIntegerRange("parity", interfaceSettings, 0, 0, 2);
    connSett.flowcontrol = ValueValidator::validateIntegerRange("flow", interfaceSettings, 0, 0, 4);

    return Conf2modem::RezUpdateConnSettings(connSett, ifaceParams);
}
//-------------------------------------------------------------------------------------
bool Conf2modem::openAconnection(const ZbyrConnSett &connSett, QString &connline)
{
    bool r = false;
    switch(connSett.connectionType){
#ifndef DISABLE_TCPCLIENT_MODE
    case IFACECONNTYPE_TCPCLNT: r = openTcpConnection(connSett.disableAPImode, connSett.prdvtrAddr, connSett.prdvtrPort); break;
#endif

#ifdef ENABLE_EXTSUPPORT_OF_IFACES

#ifndef DISABLE_M2M_MODULE
    case IFACECONNTYPE_M2MCLNT : r = openM2mConnection(connSett.disableAPImode, connSett.m2mhash); break;
#endif

#ifndef DISABLE_SERIALPORT_MODE
    case IFACECONNTYPE_UART : r = openSerialPort(connSett.disableAPImode, connSett.prdvtrAddr, connSett.prdvtrPort, connSett.uarts, connSett.databits, connSett.stopbits, connSett.parity, connSett.flowcontrol); break;
#endif

#endif


    }

    if(r){

        switch(connSett.connectionType){
#ifndef DISABLE_TCPCLIENT_MODE
        case IFACECONNTYPE_TCPCLNT: connline = QString("%1\n%2").arg(connSett.prdvtrAddr).arg(connSett.prdvtrPort); break;
#endif

#ifdef ENABLE_EXTSUPPORT_OF_IFACES
#ifndef DISABLE_M2M_MODULE
        case IFACECONNTYPE_M2MCLNT: connline = ConvertAtype::varHash2str(connSett.m2mhash); break;
#endif
#ifndef DISABLE_SERIALPORT_MODE
        case IFACECONNTYPE_UART : connline = QString("%1\n%2").arg(connSett.prdvtrAddr).arg(connSett.prdvtrPort); break;
#endif
#endif
        }

    }
    lastConnState = r;
    return r;
}


//-------------------------------------------------------------------------------------

bool Conf2modem::enterCommandMode(const QString &operationname)
{
    emit currentOperation(tr("%1, entering the command mode").arg(operationname));
    return enterCommandMode();
}

//-------------------------------------------------------------------------------------

bool Conf2modem::enterCommandMode()
{
    QByteArray dataP("+++\r\n");/*tmpStr.toLocal8Bit();*/

    readAll();//clear a buffer

    write2dev(dataP);

    QByteArray readArr = readDeviceQuick("\r\n", false);
    if(readArr.isEmpty() && lModemState.lastCommandWasAtcn){
        write2dev(dataP);
        readArr = readDevice();
    }

    if(sayGoodByeIfUartIsNFree("enterCommandMode 3643 ", 555))
        return false;



    if(readArr.left(6).toUpper() == "ERROR "){ //vidkryta proshyvka za4ekaty 4 cek i uvijty
        for(int i = 0; i < 5 && !stopAll; i++)
            lSleep(1000);

        readDevice();

        if(sayGoodByeIfUartIsNFree("enterCommandMode 3644 ", 555))
            return false;

        write2dev(dataP);

        readArr = readDevice();

    }

//#ifdef HASGUI4USR
    lModemState.modemIsOverRS485 = (readArr == "O\r\n");
//#endif
    for( int retry = 0, odyRaz = 0; retry < 7 && !isCommandModeAnswer(readArr) ; retry++) {//zminyty symvoly vhody v comandnyj rejym

        if(sayGoodByeIfUartIsNFree("enterCommandMode 3645 ", 555))
            return false;

        if(readArr.isEmpty()) {
            if(retry < 5){
                lSleep(((retry + 1) * 200));
            }else {
                for(int i = 6; i > 0; i--) {
                    if(sayGoodByeIfUartIsNFree("enterCommandMode 3653 ", 555))
                        return false;

                    lSleep(1000);
                }
            }
        }else{
            if(readArr.left(7).toUpper() != "OKERROR" && (readArr.left(2).toUpper() == "OK" || readArr.left(3).toUpper() == "O\r\n" || readArr.left(3).toUpper() == "K\r\n")){
                retry--;//rs485 and 115200 baud
            }
        }
        if(sayGoodByeIfUartIsNFree("directAccess 3654 ", 555))
            return false;

        readArr = readAll();//just quick read, it can contains the data
        if(isCommandModeAnswer(readArr))
            break;


        write2dev(dataP);
        readArr = readDevice();
        if(retry == 6 && odyRaz == 0 && !readArr.isEmpty()){
            odyRaz = 1;
            retry--;
        }

//#ifdef HASGUI4USR
        if(readArr == "O\r\n")
            lModemState.modemIsOverRS485 = true;
//#endif

    }

#ifdef ENABLE_VERBOSE_SERVER
    if(activeDbgMessages)  emit appendDbgExtData(dbgExtSrcId, QString("Conf2modem enterCommandMode 3643 directAccess=%1, uartBlockPrtt=%2, myPrtt=%3, readArr=%4, isEntered=%5")
                          .arg(lModemState.directAccess).arg(lModemState.uartBlockPrtt).arg(QString(writePreffix)).arg(QString(readArr.toHex())).arg((bool)(readArr.left(7).toUpper() == "OKERROR" || readArr.left(5).toUpper() == "ERROR")));
#endif
    return isCommandModeAnswer(readArr);
}

bool Conf2modem::isCommandModeAnswer(const QByteArray &readArr)
{
    return (readArr.startsWith("OKERROR") || readArr.startsWith("ERROR"));
}



//-------------------------------------------------------------------------------------

bool Conf2modem::networkReset(QString &errStr)
{
//    bool writeSomeCommand(const QStringList &list2write, const bool &enterTheCommandMode, const bool &exitCommandMode, const bool &atfrAtTheEnd, const QString &operationName, QString &errStr);

   const bool r = writeSomeCommand(QString("ATNR 5").split("\t"), true, false, false, tr("Network reset"), errStr);
 #ifndef HASGUI4USR
   if(r){
       QTime time;
       time.start();
       QTime locktime;
       locktime.start();
       for(int i = 0; i < 60 && time.elapsed() < 30000; i++){
           if(lModemState.isMainConnectionUsed && !writePreffix.isEmpty() && locktime.elapsed() > 1000){
               writeATcommand("ATCN");
               locktime.restart();
           }
           readDevice("\r\n", false);//wait for the second OK\r\n
       }
   }
#endif


   return r;

}

//-------------------------------------------------------------------------------------

bool Conf2modem::resetAmodem(QString &errStr)
{
    return writeSomeCommand(QStringList(), true, true, true, tr("Reset the modem"), errStr);
}

//-------------------------------------------------------------------------------------

bool Conf2modem::factorySettings(QString &errStr)
{
    const QMap<QString,QString> map = getTheModemInfo("ATBD", false, false, tr("Factory settings"), errStr);
    if(map.isEmpty())
        return false;

    bool ok;
    const int baudindx = map.value("ATBD").toInt(&ok);
    if(ok && baudindx >= 0 && baudindx < 8)
        return writeSomeCommand(QString("ATRE;ATBD%1;ATWR").arg(baudindx).split(";"), false, true, true, tr("Factory settings"), errStr);

    errStr = tr("Received a bad value(");
    return false;

}
//-------------------------------------------------------------------------------------
bool Conf2modem::readAboutModem(QVariantMap &atcommands, QString &errStr)
{

    const QStringList listat = EmbeeDefaultReadCommandList::getReadCommandList();
//ATSM
    QStringList list2write;
    for(int i = 0, imax = listat.size(); i < imax; i++){
        if(!atcommands.contains(listat.at(i)))
            list2write.append(listat.at(i));
    }

    if(!atcommands.contains("AT_list"))
        atcommands.insert("AT_list", listat);

    list2write.removeOne("ATSM");

    const QMap<QString,QString> map = getTheModemInfo(list2write, false, false, tr("Reading the modem"), errStr);
    if(map.isEmpty())
        return false;


    for(int i = 0, imax = list2write.size(); i < imax; i++)
        atcommands.insert(list2write.at(i), map.value(list2write.at(i)).toUpper());

    bool exitCommandModeLater = true;
    if(!atcommands.contains("ATSM") ){
        if(atcommands.value("ATAD").toString().toUpper() == "C" && atcommands.value("ATVR").toString().mid(1).toInt() > 205){
            const QMap<QString,QString> map = getTheModemInfo("ATSM", true, false, tr("Reading the modem"), errStr);
            if(map.isEmpty())
                return false;
            atcommands.insert(QString("ATSM"), map.value("ATSM").toUpper());
            exitCommandModeLater = false;
        }else
            atcommands.insert(QString("ATSM"), "!");
    }
    if(exitCommandModeLater){
        writeATcommand("ATCN");
        readAll();
    }

    return true;

}

//-------------------------------------------------------------------------------------

bool Conf2modem::nodeDiscovery(const int &totalModemCount, const qint64 &totalMsecElapsed, const qint64 &totalMsecLimt, const bool &hardRecovery, int &modemReady, QStringList &listreadynis, QVariantMap &ndtParams, QString &errStr)
{

    const QString opername = tr("Node discovery");
    bool sendAtfr = true;
    if(ndtParams.isEmpty()){//first time only
        ndtParams.insert("started", QDateTime::currentDateTimeUtc().toString("yyyy/MM/dd hh:mm:ss"));
        const QMap<QString,QString> map = getTheModemInfo("ATAD", false, false, opername, errStr);
        if(map.isEmpty())
            return false;
        if(lModemState.modemIsOverRS485){
            errStr = tr("Node Discovery Tool doesn't support RS485");
#ifdef ENABLE_EXTSUPPORT_OF_IFACES

#ifdef ISNATIVEIFACE
            //WTF???
                  lModemState.modemIsOverRS485 = false;
                  return false;
#else

            return true;//the end
#endif

#else
      //WTF???
            lModemState.modemIsOverRS485 = false;
            return false;
#endif
        }


        if(map.value("ATAD").toUpper() != "C"){
            errStr = tr("The modem is not a coordinator");
            return true;//the end
        }

        const QMap<QString,QString> mapparams = getTheModemInfo(QString("ATSM ATRN").split(" "), false, false, opername, errStr);
        if(mapparams.isEmpty())
            return false;

        if(mapparams.value("intATSM") != "0"){
            ndtParams.insert("intATSM", mapparams.value("intATSM"));

            if(!writeSomeCommand(QString("ATSM0 ATWR").split(" "), false, true, true, opername, errStr))
                return false;
            readDevice();
            sendAtfr = false;
        }

//        if(!mapparams.value("ATRN") != "1"){
//            waitForReadyRead(2000);
//            readDevice();
//        }
    }
    if(sendAtfr){
        writeSomeCommand("", false, true, true, opername, errStr);
        readDevice();
        sendAtfr = false;
    }


    const qint64 timeout = 3 * 60 * 1000;// 155000;
    QTime time;
    time.start();

    bool wasInCommandMode = false;
    QString readstr;

    const int modemMaximum = (totalModemCount < 1) ? 0xFFFF : totalModemCount;

    qint64 timeelapsed = time.elapsed();

    bool sayGoodByCoordinator = false;

    for(int i = 0; !stopAll && isConnectionWorking() && i < 0xFFFFFFF && timeelapsed < timeout && modemReady < modemMaximum; i++){

        if(!wasInCommandMode){
            if(!enterCommandMode(opername))
                break;
            wasInCommandMode = true;
            readstr.clear();
            writeATcommand("ATND");
        }

        readstr.append(readDevice());
        if(readstr.contains("\r\n")){
            bool need2reenterTheCommandMode;
            modemReady += deocodeNtdOut(processNdtLine(readstr), hardRecovery, need2reenterTheCommandMode, listreadynis);
            if(modemReady >= modemMaximum)
                break;

            if(need2reenterTheCommandMode)
                wasInCommandMode = false;
        }
        timeelapsed = time.elapsed();
        if((totalMsecElapsed + timeelapsed) >= totalMsecLimt){
            sayGoodByCoordinator = true;
            break;
        }
    }


    if(stopAll){
        exitCommandModeSimple();
        return true;
    }

    if(modemReady >= modemMaximum)
        sayGoodByCoordinator = true;


    if(sayGoodByCoordinator){
        QStringList writecommands;

#ifdef ENABLE_EXTSUPPORT_OF_IFACES        
#ifdef ISNATIVEIFACE
        writecommands.append("ATC2 0");//I need at least one item in the list

#else
        if(ndtParams.value("intATSM") == "1"){
            writecommands.append("ATSM1");
            writecommands.append("ATWR");
        }
#endif

#else
        writecommands.append("ATC2 0");//I need at least one item in the list
#endif
        const bool r = writeSomeCommand(writecommands, false, true, true, opername, errStr);
        emit currentOperation(tr("Exiting the command mode, result is '%1'").arg(int(r)));
        if(!r)
            readDeviceQuick("\r\n", false);
        return true;
    }

    return false;
}
//-------------------------------------------------------------------------------------
bool Conf2modem::writeAtmd(const int &atmdval, QString &errstr)
{
    const bool r = writeCommands2aModem(QString("ATMD %1;ATWR;ATCN").arg(atmdval).split(";"), errstr);
    if(r)
        lModemState.lastAtmdValue = atmdval;
    return r;
}

//-------------------------------------------------------------------------------------

bool Conf2modem::writeCommands2aModem(const QStringList &lcommands, QString &errStr)
{
    bool exitthecommandmode = true;
    if(!lcommands.isEmpty()){
        const QStringList lexitcm = QString("ATLN ATNR ATFR ATCN ATAC ").split(" ", QString::SkipEmptyParts);
        exitthecommandmode = !lexitcm.contains(lcommands.last().toUpper().left(4));
    }

    return writeSomeCommand(lcommands, true, exitthecommandmode, true, tr("Changing the configuration"), errStr);

}

//-------------------------------------------------------------------------------------

bool Conf2modem::checkConnectionTool(const QString &ni, QString &errStr)
{

//    15:37:56.470 192.168.88.143 <- 34 35 34 35 20 44 31 0D 0A                      4545 D1
//    15:38:01.969 192.168.88.143 -> 45 52 52 4F 52 0D 0A                            ERROR
//    15:38:30.271 192.168.88.143 <- 32 33 33 62 31 30 38 20 44 31 0D 0A             233b108 D1
//    15:38:37.623 192.168.88.143 -> 45 52 52 4F 52 0D 0A                            ERROR
//    15:38:42.993 192.168.88.143 <- 32 33 33 62 31 30 38 20 44 31 0D 0A             233b108 D1
//    15:38:43.493 192.168.88.143 -> 4F 4B 0D 0A                                     OK
//    15:38:43.752 192.168.88.143 -> 32 33 33 62 31 30 38 20 44 31 0D 0A 45 52 52 4F 233b108 D1 ERRO
//                 192.168.88.143 -> 52 0D 0A                                        R


    if(!lModemState.lastCommandWasAtcn){
        lModemState.lastCommandWasAtcn = true;
        writeATcommand("ATCN");
    }
    readDeviceQuick("\r\n", true);


    const QByteArray writearr = QString("%1 D0\r\n").arg(ni).toUtf8();
    write2dev(writearr);

    QTime time;
    time.start();
    const int msecwait = qMax(timeouts.global, 7777);

    QByteArray readarr;
    for(int i = 0; i < 10000 && time.elapsed() < msecwait; i++){
        const QByteArray arr = readDevice();
        if(!arr.isEmpty()){
            readarr.append(arr);
            if((readarr.startsWith("ERROR") || readarr.contains(writearr)) && readarr.endsWith("\r\n")){
                break;
            }

        }
    }
    const QByteArray perfectprefix = QByteArray("OK\r\n") + writearr;
    if(readarr.startsWith(perfectprefix)){
        if(readarr.length() > perfectprefix.length()){//contains more data than perfectprefix
            errStr = tr("Device %1 was found").arg(ni);
            return true;
        }
        errStr.append(tr("The answer len is not valid"));
    }else{
        errStr.append(tr("Bad answer was detected"));
    }

    return false;


}

//-------------------------------------------------------------------------------------

bool Conf2modem::writeSomeCommand(const QString &atcommand, const bool &enterTheCommandMode, const bool &exitCommandMode, const bool &atfrAtTheEnd, const QString &operationName, QString &errStr)
{
    return writeSomeCommand(atcommand.split("\n", QString::SkipEmptyParts), enterTheCommandMode, exitCommandMode, atfrAtTheEnd, operationName, errStr);
}

//-------------------------------------------------------------------------------------

bool Conf2modem::writeSomeCommand(const QStringList &list2write, const bool &enterTheCommandMode, const bool &exitCommandMode, const bool &atfrAtTheEnd, const QString &operationName, QString &errStr)
{

    for(int retr = 0, jmax = list2write.size(); retr < 3; retr++){
        if(retr > 0)
            readAll();

        errStr.clear();

        emit currentOperation(tr("Executing '%1'").arg(operationName));

        if(stopAll){
            if(verboseMode)
                qDebug() << "power loss 3008";

            return true;
        }


        if((enterTheCommandMode || retr > 0 ) &&  !enterCommandMode(operationName)){
            if(sayGoodByeIfUartIsNFree(operationName, 555))
                return false;

            errStr = tr("Couldn't enter the command mode(");
            continue;
        }

        bool hasErr = false;

        for(int j = 0; j < jmax && !hasErr; j++){
            writeATcommand(list2write.at(j));// "ATNR 5");
            const QByteArray readarr = readDevice();
            if(readarr.startsWith("OK\r\n") || readarr.endsWith("OK\r\n"))
                continue;
            hasErr = true;
            errStr = tr("The command '%1' failed").arg(list2write.at(j));
        }


        if(hasErr){
            continue;
        }

        if(!exitCommandMode){
            emit currentOperation(tr("Done '%1'").arg(operationName));
            return true;
        }
        emit currentOperation(tr("atfr 510 '%1'").arg(int(atfrAtTheEnd)));

        writeATcommand(atfrAtTheEnd ? "ATFR" : "ATCN");



        if( (atfrAtTheEnd && wait4doubleOk(false, false)) || (!atfrAtTheEnd && readDevice().startsWith("OK\r\n"))){
            emit currentOperation(tr("Done '%1'").arg(operationName));

            lModemState.isCoordinatorConfigReady = false;
            return true;
        }



        errStr = tr("Unknown error (");
        exitCommandModeSimple();

    }
    return false;
}


//-------------------------------------------------------------------------------------

bool Conf2modem::wait4doubleOk(const bool &isAtlbCommand, const bool &ignoreSecondErr)
{

    QTime time;
    time.start();
    QByteArray readArr = readDevice().toUpper();
    if(readArr.isEmpty())
        return false;

    const int maxtimeout = isAtlbCommand ? 7500 : 6500;
    const QByteArray answr = isAtlbCommand ? "OK\r\nOK\r\r\n" : "OK\r\nOK\r\n";

    const QByteArray answr2 = ignoreSecondErr ? "OK\r\nERROR\r\r\n" : QByteArray();


    for(int nn = 0; nn < 1000 && time.elapsed() < maxtimeout && readArr != answr && !stopAll; nn++){
        readArr.append(readDevice().toUpper());
        if(ignoreSecondErr && readArr == answr2)
            return true;
        QThread::msleep(50);
        if(lModemState.modemIsOverRS485 && readArr == "OK\r\n" && time.elapsed() > 3000)
            return true;//only for RS485

    }
    //for ATFR I need only OK
    const bool r = (isAtlbCommand ? (readArr == answr) : (readArr.contains("OK")));
    return r;


}

//-------------------------------------------------------------------------------------

QMap<QString, QString> Conf2modem::getTheModemInfo(const QString &atcommand, const bool &exitCommandMode, const bool &atfrAtTheEnd, const QString &operationName, QString &errStr)
{
    return getTheModemInfo(atcommand.split("\n"), exitCommandMode, atfrAtTheEnd, operationName, errStr);
}

//-------------------------------------------------------------------------------------

QMap<QString, QString> Conf2modem::getTheModemInfo(const QStringList &list2read, const bool &exitCommandMode, const bool &atfrAtTheEnd, const QString &operationName, QString &errStr)
{
    QMap<QString,QString> map;
    if(!writeSomeCommand(QStringList(), true, false, false, tr("%1, entering the command mode").arg(operationName), errStr))
        return map;

    emit currentOperation(tr("Reading '%1'").arg(operationName));

    for(int i = 0, imax = list2read.size(); i < imax; i++){
        writeATcommand(list2read.at(i));
        const QByteArray readarr = readDevice();
        if(!readarr.isEmpty() && readarr != "ERROR\r\n"){
            const QString rez = QString(readarr).left(readarr.length() - 2);

            bool ok;
            const int v = rez.toInt(&ok);

            map.insert(list2read.at(i), rez);
            if(ok)
                map.insert("int" + list2read.at(i), QString::number(v));
            emit atCommandRez(list2read.at(i), rez);
        }else
            errStr = tr("Command '%1' failed").arg(list2read.at(i));
    }

    if(exitCommandMode && !writeSomeCommand(QStringList(), false, exitCommandMode, atfrAtTheEnd, tr("%1, exiting the command mode").arg(operationName), errStr))
        return map;

    return map;
}

//-------------------------------------------------------------------------------------

bool Conf2modem::enableDisableApi(const bool &enable, const bool &readAboutZigBee)
{
    bool apiPlus = true;
    bool ifTrueATCN = true;

    bool breakNow = false;
#ifdef ENABLE_VERBOSE_SERVER
    if(activeDbgMessages)  emit appendDbgExtData(dbgExtSrcId, QString("Conf2modem enableDisableApi a directAccess=%1, uartBlockPrtt=%2, myPrtt=%3, lModemState.apiErrCounter=%4, readAboutZigBee=%5")
                          .arg(lModemState.directAccess).arg(lModemState.uartBlockPrtt).arg(QString(writePreffix)).arg(lModemState.apiErrCounter).arg(int(readAboutZigBee)));
#endif
    bool wasOk4atfr = false, wasOk4atfrFunction = false;
    for(int i = 0; i < 3 && !breakNow; i++){
        emit currentOperation(tr("API enbl=%1, prtt=%2, rtr=%3").arg(enable).arg(QString(writePreffix)).arg(i));
        if(verboseMode)
            qDebug() << "ZbyratorObject::enableDisableApi ATFR " << enable << i << lModemState.directAccess;

        if(!isCoordinatorFreeWithRead()){
            emit currentOperation(tr("API enbl=%1, prtt=%2, rtr=%3 isCoordinatorFreeWithRead").arg(enable).arg(QString(writePreffix)).arg(i));

            continue;
        }


        if(!isConnectionWorking()){
            emit currentOperation(tr("API enbl=%1, prtt=%2, rtr=%3 !isConnectionWorking").arg(enable).arg(QString(writePreffix)).arg(i));

            return false;
        }


        if(!enterCommandMode()){
            emit currentOperation(tr("Couldn't enter the command mode("));
            emit currentOperation(tr("API enbl=%1, prtt=%2, rtr=%3 !enterCommandMode").arg(enable).arg(QString(writePreffix)).arg(i));


            if(!isCoordinatorFreeWithRead())
                readDeviceQuick("\r\n", false);

            continue;
        }

        writeATcommand("ATFR");
//        readDevice();

        wasOk4atfr = wasOk4atfrFunction = wait4doubleOk(false, false);

        emit currentOperation(tr("for wasOk4atfr =%1, stopAll=%2").arg(int(wasOk4atfr)).arg(int(stopAll)));
        if(!wasOk4atfr){

            QThread::sleep(5);

            exitCommandModeSimple();
            wasOk4atfr = true;
        }
        break;


    }

#ifdef HASGUI4USR
    if(lModemState.modemIsOverRS485)
        return true;
#endif

    emit currentOperation(tr("!for wasOk4atfr =%1, stopAll=%2").arg(int(wasOk4atfr)).arg(int(stopAll)));


    if(!wasOk4atfr)
        return false;

    QVariantHash aboutModem;
    for(int i = 0; i < 3 && !breakNow; i++){
        qint32 prosh = 0;
        if(verboseMode)
            qDebug() << "ZbyratorObject::enableDisableApi " << enable << i;

        if(stopAll)
            return false;

        if(i < 1 && !wasOk4atfrFunction){//function has returned false - so I need to one more read
            readDeviceQuick("\r\n", true);
        }else{
            if(!isCoordinatorFreeWithRead())
                continue;

            if(!isConnectionWorking())
                return false;
        }
        if(!enterCommandMode()){
            emit currentOperation(tr("Couldn't enter the command mode("));

            if(!isCoordinatorFreeWithRead())
                continue;
            if(stopAll)
                return false;
            continue;
        }
        if(stopAll)
            return false;
        int indxLummerLog = 2;
        QStringList listArr;
        if(enable){
            writeATcommand("ATAD");

            if(true){
                const QByteArray readArr = readDevice();
                if(readArr.isEmpty())
                    continue;//priority access error
                if(readArr.toUpper() != "C\r\n"){
                    writeATcommand("ATAD C");
                    readDevice();
                    writeATcommand("ATSM 0");
                    readDevice();
                    ifTrueATCN = false;
                }
            }
            writeATcommand("ATVR");
            bool okkk;
            const QByteArray proshMy = readDevice();
            prosh = proshMy.mid(1,3).toInt(&okkk, 10);
            const bool fuckingEnrgmr = (proshMy.left(1) == "E" && okkk);

            emit currentOperation(tr("Checking the %1 mode.").arg(apiPlus ? "API" : "API+"));

            if(prosh >= 206 && okkk) {
                if(stopAll)
                    return false;
                writeATcommand("ATSM");
                bool ok;
                const int tmpIntVal = readDevice().left(2).toInt(&ok);
                if(tmpIntVal != 0 || !ok){
                    ifTrueATCN = false;
                    writeATcommand("ATSM 0");
                    if(readDevice().toUpper() != "OK\r\n"){
                        emit currentOperation(tr("Couldn't set up the modem"));
                        lSleep(11111);
                        continue;
                    }
                }
            }


            QList<int> listVal;
            listVal << 1 << 0 << 1 << 0 ;

            listArr = QString("ATAP ATIR ATIU ATC1").split(' ');

            if(lModemState.atmd2write > 0){
                listVal.append(lModemState.atmd2write);
                listArr.append("ATMD");
                lModemState.lastAtmdValue = lModemState.atmd2write;
            }

            if(apiPlus){
                if(fuckingEnrgmr)
                    listVal.append(6);
                else
                    listVal.append(1);
                listArr.append("ATCP0");
                indxLummerLog++;

            }

            bool allGood = true;
             int retryList = 0;
            for(int la = 0, laMax = listArr.size(), laMax2 = listVal.size(); la < laMax && la < laMax2; la++){
                bool ok;
                writeATcommand(listArr.at(la));
                const QByteArray readArr = readDevice();
                int tmpIntVal = readArr.simplified().trimmed().toInt(&ok, 16);
                if(tmpIntVal != listVal.at(la) || !ok){
                    if(verboseMode)
                        qDebug() << "enableDisableApi checkVal " << ok << tmpIntVal << readArr << la << listArr.at(la) << listVal.at(la);
                    if(stopAll)
                        return false;
                    writeATcommand(listArr.at(la) + QString::number(listVal.at(la)));
                    if(readDevice().toUpper() != "OK\r\n"){
                        retryList++;
                        la--;
                        if(retryList > 1){
                            allGood = false;
                            break;
                        }
                    }else{
                        retryList = 0;
                    }
                }

            }
            listArr.clear();
            if(!allGood){
                emit currentOperation(tr("Couldn't set up the modem"));
                lSleep(6666);
                continue;
            }


            if(!ifTrueATCN){
                listArr.append("ATWR");
                emit currentOperation(tr("Enabling the %1 mode.").arg(apiPlus ? "API +" : "API"));
            }




        }else{
                listArr.append("ATCP0 0");
                listArr.append("ATMD 0");
                indxLummerLog = 1;


            listArr.append("ATAP 0");
            listArr.append("ATWR");
            emit currentOperation(tr("Disabling the %1 mode.").arg(apiPlus ? "API +" : "API"));
        }


        QStringList listAboutModem;

        if(readAboutZigBee){
            listAboutModem = QString("ATAD ATPL ATCH ATID ATSH ATSL ATHV ATMD ATVR ATDB").split(" ", QString::SkipEmptyParts);
            if(prosh > 203)
                listAboutModem.append("ATHP");

            listArr.append(listAboutModem);
            ifTrueATCN = false;//after ATDB reset a coordinator
        }

        listArr.append(ifTrueATCN ? "ATCN" : "ATFR");


        int retryList = 0;
        for(int i = 0, iMax = listArr.size(); i < iMax; i++){
            if(stopAll)
                return false;


             if(listArr.at(i) == "ATDB" && lModemState.modemIsOverRS485){

                 aboutModem.insert(listArr.at(i), "ATDB is not supported over RS485");
                 retryList = 0;
                 continue;

             }
                 writeATcommand(listArr.at(i));
                  QByteArray readArr = readDevice();



            if(!readArr.isEmpty() && listAboutModem.contains(QString(listArr.at(i).left(4)) )){
                if(listArr.at(i) == "ATDB" ){


                    QTime time;
                    time.start();
                    for(int v = 0; v < 1000 && time.elapsed() < 30000 && !(readArr.contains("LQI:") && readArr.lastIndexOf("\r\n") > readArr.indexOf("LQI:") ); v++){
                        readArr.append(readDevice());
                        if(readArr.contains("OK\r\n") || readArr.contains("ERROR\r\n") || stopAll)
                            break;
                        QThread::msleep(11);
                    }
                    emit currentOperation(QString("atdb out '%1'").arg(QString("r=%1, h=%2").arg(QString(readArr.simplified())).arg(QString(readArr.toHex()))));

                    if(readArr.contains("OK\r\n") || readArr.contains("ERROR\r\n"))
                        readArr.clear();

                    if(!(readArr.contains("LQI:") && readArr.lastIndexOf("\r\n") > readArr.indexOf("LQI:")))
                        readArr.clear();

                }
                aboutModem.insert(listArr.at(i), readArr.simplified().trimmed().toUpper());
                retryList = 0;
                continue;
            }

            if(readArr.toUpper() != "OK\r\n"){
                retryList++;
                i--;
            }else
                retryList = 0;

            if(retryList > 1)
                break;

        }

        if(retryList > 0){
            ifTrueATCN = false;
            emit currentOperation(tr("Couldn't set up the modem"));
        }else{
            if(!ifTrueATCN){ //write ATFR
                QTime time;
                time.start();
                QByteArray readArr = readDevice().toUpper();


                for(int i = 0; i < 1000000 && time.elapsed() < 11000 && readArr != "OK\r\n" && !stopAll; i++){
                    readArr.append(readDevice().toUpper());                    

                }
                 if(!readArr.contains("OK\r\n") && lModemState.modemIsOverRS485 ){

                     readArr.append("OK\r\n");//only for RS485

                 }


                if(!readArr.contains("OK\r\n")){
                    emit currentOperation(tr("Couldn't set up the modem"));
                    lSleep(6666);
                    continue;
                }

            }

            emit currentOperation(tr("The %1 mode was %2.").arg(apiPlus ? "API+" : "API").arg(enable ? "enabled" : "disabled"));


            if(readAboutZigBee && !aboutModem.isEmpty()){
                setAtmdValueFromString(aboutModem.value("ATMD").toString());


                aboutModem = conf2modemHelper::aboutZigBeeModem2humanReadable(aboutModem);
                if(!aboutModem.isEmpty()){
                    aboutModem.insert("app", QString(MYAPPNAME));
                    emit onAboutZigBee(addCurrentDate2aboutModem(aboutModem));
                }
            }
            return true;
        }
    }

    return false;
}


//-------------------------------------------------------------------------------------


bool Conf2modem::isCoordinatorGood(const bool &forced, const bool &readAboutZigBee)
{

#ifdef ENABLE_EXTSUPPORT_OF_IFACES
#ifndef ISNATIVEIFACE
    lModemState.isCoordinatorConfigReady = (lModemState.isCoordinatorConfigReady || !forced);
#endif
#endif


    if(!forced && lModemState.isCoordinatorConfigReady && qAbs(lModemState.hashMsecWhenCoordinatorWasGood.value(ifaceName) - QDateTime::currentMSecsSinceEpoch()) < MAX_MSEC_FOR_COORDINATOR_READY ){
        if(lModemState.isModemInCommandMode){
            writeATcommand("ATCN");
            readDeviceQuick("\r\n", true);
        }
        return lModemState.isCoordinatorConfigReady;
    }

    if(verboseMode)
        qDebug() << "isCoordinatorGood " << qAbs(lModemState.hashMsecWhenCoordinatorWasGood.value(ifaceName) - QDateTime::currentMSecsSinceEpoch()) << MAX_MSEC_FOR_COORDINATOR_READY ;


    lModemState.isCoordinatorConfigReady = false;
    if(enableDisableApi(true, readAboutZigBee)){
        emit currentOperation(tr("The API mode was activated"));

        emit onApiModeEnabled();
        lModemState.isCoordinatorConfigReady = true;
        lModemState.apiErrCounter = 0;
        lModemState.hashMsecWhenCoordinatorWasGood.insert(ifaceName, QDateTime::currentMSecsSinceEpoch());

    }else{
        emit currentOperation(tr("Couldn't activate the API mode, isCoordinatorConfigReady=%1, interface '%2'").arg(lModemState.isCoordinatorConfigReady).arg(ifaceName));
    }


    if(lModemState.uartBlockPrtt || lModemState.directAccess)
        return lModemState.isCoordinatorConfigReady;

    if(!lModemState.isCoordinatorConfigReady)
        incrementApiErrCounter();

    return lModemState.isCoordinatorConfigReady;

}

//-------------------------------------------------------------------------------------


QVariantHash Conf2modem::addCurrentDate2aboutModem(QVariantHash &aboutModem)
{
    aboutModem.insert("Updated", QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
    if(verboseMode)
        qDebug() << "addCurrentDate2aboutModem aboutModem " << aboutModem;
    return aboutModem;
}

//-------------------------------------------------------------------------------------


QStringList Conf2modem::processNdtLine(QString &line)
{


    const QStringList listSeperatist = line.split("\r\n", QString::SkipEmptyParts);

    if(!line.endsWith("\r\n") && !listSeperatist.isEmpty())
        line = listSeperatist.last();
    else
        line.clear();


    return listSeperatist;
}

//-------------------------------------------------------------------------------------

int Conf2modem::deocodeNtdOut(const QStringList &list, const bool &allowHard, bool &need2reenterTheCommandMode, QStringList &listreadynis)
{
    need2reenterTheCommandMode = false;
    QMap<QString,QString> brokenStts;
    //Default - OK, Recovered, Corrupted

    int foundModems = 0;
    for(int i = 0, imax = list.size(); i < imax; i++){
        QString line = list.at(i);
        if(line.startsWith("OK") || line.startsWith("ERR")){
            need2reenterTheCommandMode = true;
            break;
        }

        if(line.contains(":") && line.contains(",") && (line.contains("r:", Qt::CaseInsensitive) ||
                                                        line.contains("c:", Qt::CaseInsensitive) ||
                                                        line.contains("e:", Qt::CaseInsensitive) ||
                                                        line.contains("m:", Qt::CaseInsensitive))){

            const qint64 msec = QDateTime::currentMSecsSinceEpoch();

            for(int j = 0; !line.isEmpty() && j < 0xFFFF; j++){
                if(line.mid(1,1) == ":")
                    break;
                line = line.mid(1);
            }


            if(line.length() > 37){
                if(allowHard)
                    foundModems += decodeNtdOneLineHard(line, msec, brokenStts, listreadynis);
                continue;
            }

            if(line.mid(1,1) == ":" && line.mid(18,1) == "," && decodeNtdOneLine(line, msec, false, listreadynis))
                foundModems++;
        }
    }
    return foundModems;
}

//-------------------------------------------------------------------------------------


int Conf2modem::decodeNtdOneLineHard(const QString &line, const qint64 &msec, QMap<QString, QString> &brokenStts, QStringList &listreadynis)
{
    const int li4ylnyk = line.count(":00", Qt::CaseInsensitive);
    const int li4ylnykKoma = line.count(",", Qt::CaseInsensitive);

    int foundModems = 0;
    if(li4ylnyk > 1 && li4ylnykKoma > 1){
        QString line2 = line;
        QStringList listSeperatist2;
        const QStringList listType = QString("R:00,E:00,M:00,C:00").split(',');
        const int listTypeSize = listType.size();

        for(int i = 0; i < listTypeSize; i++){
            int lastIndx = line2.lastIndexOf(listType.at(i), -1, Qt::CaseInsensitive);
            if(lastIndx > 0){
                /*
                     * 16:21:47.081 ttyUSB1 -> 72 3A 30 30 30 44 36 46 30 30 36 46 30 30 30 30      r:000D6F006F0000
                     *              ttyUSB1 -> 37 41 36 44 42 39 2C 38 72 3A 30 30 30 44 36 46      7A6DB9,8r:000D6F
                     *              ttyUSB1 -> 30 30 30 30 37 41 38 37 35 44 2C 36 32 31 0D 0A      00007A875D,621
                     */
                //                                    qDebug() << 4354 << line2.mid(lastIndx) << line2.mid(lastIndx).toLocal8Bit().toHex();
                listSeperatist2.append(line2.mid(lastIndx));
                line2 = line2.left(lastIndx);
                i--;
            }

        }
        listSeperatist2.append(line2);

        //                                QStringList listSeperatist3;
        for(int i = 0, imax = listSeperatist2.size(); i < imax; i++){// (!listSeperatist2.isEmpty()){
            QString line2 = listSeperatist2.at(i);
            while(!line2.isEmpty()){
                if(line2.mid(1,1) == ":")
                    break;
                line2 = line2.mid(1);
            }

            int indxKoma = line2.indexOf(",");

            if(indxKoma > 17){
                const QString typeDev2 = line2.left(2);
                QString sn = line2.mid(2,indxKoma - 2).right(16);
                QString ni = line2.mid(indxKoma + 1).left(32);

                if(sn.size() != 16)
                    continue;

                if(sn.left(2) != "00")
                    sn = "00" + sn.mid(2);

                indxKoma = ni.indexOf(",");

                if(indxKoma > 0)
                    ni = ni.left(indxKoma);

                 if(decodeNtdOneLine(QString("%1%2,%3").arg(typeDev2).arg(sn).arg(ni), msec, true, listreadynis))
                     foundModems++;

                if(sn.startsWith("000D6F"))
                    brokenStts.insert(line, QString("Recovered"));//Corrupted
                else
                    brokenStts.insert(line, QString("Corrupted"));//

            }
        }

    }
    return foundModems;
}


//-------------------------------------------------------------------------------------



bool Conf2modem::decodeNtdOneLine(const QString &line, const qint64 &msec, const bool &wasRestored, QStringList &listreadynis)
{
    const QString typeDev = line.left(1).toUpper();

    if(!line.startsWith("R", Qt::CaseInsensitive) &&
            !line.startsWith("C", Qt::CaseInsensitive) &&
            !line.startsWith("E", Qt::CaseInsensitive) &&
            !line.startsWith("M", Qt::CaseInsensitive))
        return false;



    const QString sn = line.mid(2,16);
    const QString ni = line.mid(19);

    emit ndtFounADev(msec, typeDev, sn, ni, wasRestored);// brokenStts.contains(line));

    if(listreadynis.contains(ni))
        return false;
    listreadynis.append(ni);
    emit ndtFounNewDev(msec, typeDev, sn, ni, wasRestored);// brokenStts.contains(line));

    return true;
}

//-------------------------------------------------------------------------------------

bool Conf2modem::writeZombieCommands(const QStringList &lcommands, QString &errStr)
{

    Q_UNUSED(errStr);

//    emit currentOperation(tr("Reading '%1'").arg(operationName));

//    for(int i = 0, imax = list2read.size(); i < imax; i++){
//        writeATcommand(list2read.at(i));
//        const QByteArray readarr = readDevice();
//        if(!readarr.isEmpty() && readarr != "ERROR\r\n"){
//            const QString rez = QString(readarr).left(readarr.length() - 2);

//            bool ok;
//            const int v = rez.toInt(&ok);

//            map.insert(list2read.at(i), rez);
//            if(ok)
//                map.insert("int" + list2read.at(i), QString::number(v));
//            emit atCommandRez(list2read.at(i), rez);
//        }else
//            errStr = tr("Command '%1' failed").arg(list2read.at(i));
//    }

//    if(exitCommandMode && !writeSomeCommand(QStringList(), false, exitCommandMode, atfrAtTheEnd, tr("%1, exiting the command mode").arg(operationName), errStr))
//        return map;


    EMBZombieExchangeTypes zombiereadparams = IfaceExchangeSerializedTypes::getDefaultValues4zombie(timeouts);

/*
 *
*/

    for(int i = 0, imax = lcommands.size(); i < imax && !stopAll; i++){
        processOneZombieCommand(lcommands.at(i), zombiereadparams);
    }
    return true;
}

//----------------------------------------------------------------------------------

void Conf2modem::processOneZombieCommand(const QString &command, EMBZombieExchangeTypes &zombiereadparams)
{
    if(command.isEmpty())
        return;

    if(command.startsWith("$rp ")){//read
        checkReadZombieCommand(command, zombiereadparams);
        return;
    }

    if(command.startsWith("$t:")){//make a pause
        checkMakeZombiePause(command);
        return;
    }

    QByteArray writearr;
    if(command.startsWith("$h:")){
        const QByteArray arr = QByteArray::fromHex(command.mid(3).toUtf8());
        if(!arr.isEmpty())
            return;
        writearr = arr;
    }else{
        writearr = replace0D0A(command);
    }

    write2dev(writearr);


}

//----------------------------------------------------------------------------------

void Conf2modem::checkReadZombieCommand(const QString &command, EMBZombieExchangeTypes &zombiereadparams)
{

    QTime msecwait;
    msecwait.start();

    QList<QByteArray> listanswers;
    QList<QByteArray> listends;
    int minl = 0;
    int tmsec = zombiereadparams.timeouts.global;

    const QStringList list = command.split(" ", QString::SkipEmptyParts);



    int lasttag = 0;
    /*
     * 0 - rp
     * 1 - rph
     * 2 - t
     * 3 - end
     * 4 - endh
     * 5 - minl
*/
    for(int i = 0, imax = list.size(); i < imax; i++){
        const QString line = list.at(i);


        const int tag = getTag4command(line, lasttag);

        const bool tagchanged = (tag != lasttag);
        lasttag = tag;
        switch(lasttag){
        case 0:{
            if(i == 0 || tagchanged)//it is impossible to get tagchanged=true
                continue;//this line doesn't have data
            listanswers.append( replace0D0A(line));
            break;}
        case 1:{
            const QByteArray arr = QByteArray::fromHex(line.mid(tagchanged ? 2 : 0).toLocal8Bit());
            if(!arr.isEmpty())
                listanswers.append(arr);
            break;}

        case 2:{
            bool ok;
            tmsec = getTimeFromLine(line, ok);
            break;}

        case 3:{
            listends.append( replace0D0A(line.mid(tagchanged ? 4 : 0)));
            break;}


        case 4:{
            const QByteArray arr = QByteArray::fromHex(line.mid(tagchanged ? 5 : 0).toLocal8Bit());
            if(!arr.isEmpty())
                listends.append(arr);
            break;}

        case 5:{

            bool ok;
            minl = line.mid(5).toInt(&ok);
            if(!ok || minl < 0)
                minl = 0;
            break;}
        }

    }
    QByteArray readarr;


    for(int i = 0, amax = listends.size(); i < 10000; i++){

        const QByteArray arr = readDeviceQuick(listends.isEmpty() ? "" : listends.first(), false);


        if(tmsec > 0 && msecwait.elapsed() > tmsec){
            return;
        }

        if(!arr.isEmpty()){
            readarr.append(arr);

            if(!listanswers.isEmpty() && listanswers.contains(readarr))
                return;

            const int readarrlen = readarr.length();
            if(minl > 0 && readarrlen < minl)
                continue;

            if(!listanswers.isEmpty() && listanswers.contains(readarr))
                break;

            for(int a = 0; a < amax; a++){
                if(readarr.endsWith(listends.at(a))){
                    return;
                }
            }

            if(minl > 0 && readarrlen >= minl && listanswers.isEmpty() && listends.isEmpty())
                return;
        }

    }

}

//----------------------------------------------------------------------------------

QByteArray Conf2modem::replace0D0A(QString line)
{
    if(line.contains("\\r")){
        line = line.replace("\\r", "\r");
    }
    if(line.contains("\\n")){
        line = line.replace("\\n", "\n");
    }

    return line.toUtf8();
}

//----------------------------------------------------------------------------------

int Conf2modem::getTag4command(const QString &line, int lasttag)
{

    if(line.startsWith("h:"))
        return 1;


    if(line.startsWith("t:"))
        return 2;


    if(line.startsWith("end:"))
        return 3;

    if(line.startsWith("endh:"))
        return 4;

    if(line.startsWith("minl:"))
        return 5;

    return lasttag;

}

//----------------------------------------------------------------------------------

int Conf2modem::getTimeFromLine(const QString &command, bool &ok)
{
    const QString s = command.mid(3).simplified().trimmed().split(" ", QString::SkipEmptyParts).first();
    int msecwait = s.toInt(&ok);
    if(msecwait < 1 || !ok)
        return 0;

    if(msecwait > 60000)
        msecwait = 60000;
    return msecwait;
}

//----------------------------------------------------------------------------------

void Conf2modem::checkMakeZombiePause(const QString &command)
{
    QTime time;
    time.start();
    bool ok;

    const int msecwait = getTimeFromLine(command, ok);

    const int stepmsec = qMax(1, msecwait/20);

    for(int i = 0; i < 600000 && time.elapsed() < msecwait && !stopAll; i++){
        QThread::msleep(stepmsec);
    }
}

//-------------------------------------------------------------------------------------

void Conf2modem::setAtmdValueFromString(const QString &s)
{
    if(!s.isEmpty()){
        bool ok;
        const int v = s.simplified().trimmed().toInt(&ok, 16);
        if(ok)
            lModemState.lastAtmdValue = v;
    }


}

//-------------------------------------------------------------------------------------
#ifdef ENABLE_EMBEEMODEM_EXTENDED_OPERATIONS

bool Conf2modem::quickRadioSetupExt(const QVariantMap &insettings, QString &errstr)
{

//    j.insert("channelold", lQrsParams.oldparams.channel);
//    j.insert("idold", lQrsParams.oldparams.id);
//    j.insert("keyold", lQrsParams.oldparams.key);
//    j.insert("channelnew", lQrsParams.newparams.channel);
//    j.insert("idnew", lQrsParams.newparams.id);
//    j.insert("keynew", lQrsParams.newparams.key);
//    j.insert("writeold", lQrsParams.writeOld);

//    j.insert("changeni", lQrsParams.changeNI);

//    QStringList listni = getNiList();

//    if(lQrsParams.changeNI){
//        j.insert("qrsmulticastmode", true);
//        if(!listni.isEmpty())
//            listni = listni.first().split(" ");//only first item
//    }else{
//        j.insert("qrsmulticastmode", lQrsParams.qrsmulticastmode);

//        if(lQrsParams.qrsmulticastmode)
//            listni = QString("#0").split(" ");

//    }

//    if(listni.isEmpty())
//        return QVariantMap();
//    j.insert("listni", listni.join(" "));

    QMap<QString,QString> mapAboutTheCoordinator;
    if(!isCoordinatorReady4quickRadioSetupExt(insettings, mapAboutTheCoordinator, errstr))
        return false;

    const QStringList listni = insettings.value("listni").toString().split(" ", QString::SkipEmptyParts);

    if(listni.isEmpty()){
        errstr = tr("bad operator");
        return true;
    }


    if(insettings.value("changeni").toBool()){
        if(!changeni(QString(), listni.first(), false)){
            errstr = tr("Couldn't change the NI");
            return true;
        }
    }
    if(stopAll)
        return true;


    const QStringList listniready = sendNetworParams(listni, insettings);
    if(stopAll)
        return true;

    return applyNewNetworkSettings(listniready, insettings, mapAboutTheCoordinator);



}

//-------------------------------------------------------------------------------------


bool Conf2modem::isCoordinatorReady4quickRadioSetupExt(const QVariantMap &insettings, QMap<QString, QString> &mapAboutTheModem, QString &errstr)
{
    if(insettings.value("ignoreCoordinatorSett").toBool()){
        const QStringList lcommands = QString("ATVR ATHV ATSL ATSH ATAD ATAP ATSM ATCP0 ATCH ATID ATMD").split(" ");
        for(int i = 0, imax = lcommands.size(); i < imax; i++){
            if(mapAboutTheModem.contains(lcommands.at(i)))
                continue;
            mapAboutTheModem.insert(lcommands.at(i), "?");
        }
        return true;

    }

    bool modemtocoordiantor = false;

    const bool hasMulticastMessages = (insettings.value("qrsmulticastmode").toBool() || insettings.value("changeni").toBool());

    if(true){
        QStringList lcommands = QString("ATVR ATHV ATSL ATSH").split(" ");

        if(!insettings.value("writeold").toBool()){
            const QStringList l = lcommands;
            lcommands = QString("ATAD ATAP ATSM ATCP0 ATCH ATID").split(" ");

            if(hasMulticastMessages)
                lcommands.append("ATMD");

            lcommands.append(l);
        }

        mapAboutTheModem = getTheModemInfo(lcommands, false, false, tr("Checking the configuration..."), errstr);
        if(mapAboutTheModem.isEmpty())
            return false;

        if(!insettings.value("writeold").toBool()){
            const QStringList lcommandsanswr = QString("c 1 0 6").split(" ");

            for(int i = 0, imax = lcommandsanswr.size(); i < imax; i++){
                const QString s = mapAboutTheModem.value(lcommands.at(i));
                const int answr = s.toInt();

                if(answr != lcommandsanswr.at(i).toInt() && mapAboutTheModem.value(lcommands.at(i)).toUpper() != lcommandsanswr.at(i).toUpper()){
                    modemtocoordiantor = true;
                    break;
                }
            }
        }
    }

    if(modemtocoordiantor || insettings.value("writeold").toBool()){

        QStringList lcommands;

        lcommands.append("ATAD C");
        lcommands.append("ATAP 1");
        lcommands.append("ATSM 0");
        lcommands.append("ATCP0 6");
        if(hasMulticastMessages){
            lcommands.append("ATMD 1");
            mapAboutTheModem.insert("ATMD", "01");
        }

        int wait4readiness = 5000;
        if(!modemtocoordiantor){
            wait4readiness = 35000;
            lcommands.append(QString("ATCH %1").arg(insettings.value("channelold").toInt(), 0, 16));
            lcommands.append(QString("ATID %1").arg(insettings.value("idold").toInt(), 0, 16));
            lcommands.append(QString("ATKY %1").arg(insettings.value("keyold").toString()));
        }

        lcommands.append("ATWR");
         if(!writeSomeCommand(lcommands, true, true, true, tr("Writing the configuration"), errstr))
             return false;

         QTime time;
         time.start();
         emit currentOperation(tr("Waiting for the network readiness..."));

         for(int i = 0; i < 10000 && time.elapsed() < wait4readiness && !stopAll; i++){
             readDevice();//wait for the second OK, readiness
             QThread::msleep(200);
         }

         for(int i = 0, imax = lcommands.size(); i < imax; i++){
             const QString s = lcommands.at(i);
             mapAboutTheModem.insert(s.split(" ").first(), s.split(" ").last());
         }

         modemtocoordiantor = true;//
    }

    if(!modemtocoordiantor){//exit the command mode
        const int atmd = hasMulticastMessages ? mapAboutTheModem.value("ATMD").toInt() : 7;
        if(!writeSomeCommand( ( atmd > 0) ? QString() :  QString("ATMD 1\nATWR"), false, true, false, tr("Exiting the command mode"), errstr))
            return false;
    }
    return true;

}

//-------------------------------------------------------------------------------------

QStringList Conf2modem::sendNetworParams(const QStringList &listni, const QVariantMap &insettings)
{
    const EmbeeNetworkParamsStr netparams = convert2netParams(insettings, QString("channelnew idnew  keynew"));
    QStringList listniready;
    for(int i = 0, imax = listni.size(); i < imax && !stopAll; i++){

        for(int r = 0; r < 3 && !stopAll; r++){

            if(r > 0){
                QThread::sleep(2);
                readAll();
            }

            const QByteArray writearr = QString("%1\r\n%2\r\n%3\r\n%4\r\n")
                    .arg((listni.at(i) == "#0") ? QString() : listni.at(i))
                    .arg(netparams.chhex)
                    .arg(netparams.idhex)
                    .arg(netparams.key).toUtf8();
            write2dev(writearr);

            const QByteArray readarr = readDevice();
            if(!readarr.startsWith("OK\r\n"))
                continue;
            listniready.append(listni.at(i));
            break;
        }

    }
    return listniready;
}

//-------------------------------------------------------------------------------------

bool Conf2modem::applyNewNetworkSettings(const QStringList &listni, const QVariantMap &insettings,  const QMap<QString, QString> &mapAboutTheCoordinator)
{
    bool parameterssended = false;
    if(!listni.isEmpty()){
        for(int i = 0, j = 0; i < 3; i++){

            if(i > 0){
                QThread::sleep(3);
                readAll();
                if(j > 0){                    
                    writeATcommand("ATCN");
                    readDevice();
                }
                j = 0;
            }

            if(stopAll)
                return true;

            if(!enterCommandMode(tr("Applying new network settings"))){
                j++;
                continue;
            }

            if(stopAll)
                return true;
            writeATcommand("ATLB 4");
            if(!wait4doubleOk(true, false)){
                j++;
                continue;
            }

            QThread::sleep(2);//some timeout


            writeATcommand("ATDN #0");
            if(!readDevice().startsWith("OK")){
                j++;
                continue;
            }

            writeATcommand("ATLB 5");
            if(!wait4doubleOk(true, true)){
                j++;
                continue;
            }
            parameterssended = true;
            break;

        }
    }

    const EmbeeNetworkParamsStr netparamsnew = convert2netParams(insettings, QString("channelnew idnew  keynew"));
    const EmbeeNetworkParamsStr netparamsold = insettings.value("writeold").toBool() ? convert2netParams(insettings, QString("channelold idold  keyold")) :
                                                                                       convert2netParams(mapAboutTheCoordinator.value("ATCH").toInt(0, 16), mapAboutTheCoordinator.value("ATID").toInt(0, 16), "#0");

    QVariantMap aboutcoordinator;
    const QList<QString> lk = mapAboutTheCoordinator.keys();
    for(int i = 0, imax = lk.size(); i < imax; i++)
        aboutcoordinator.value(lk.at(i), mapAboutTheCoordinator.value(lk.at(i)));


    emit qrsStatus(QDateTime::currentMSecsSinceEpoch(), parameterssended ? listni : QStringList(), insettings.value("listni").toString().split(" "),
                   netparamsold.chhex, netparamsold.idhex, netparamsold.key,
                   netparamsnew.chhex, netparamsnew.idhex, netparamsnew.key,
                   insettings.value("writeold").toBool(), insettings.value("changeni").toBool(), insettings.value("qrsmulticastmode").toBool(), insettings.value("ignoreCoordinatorSett").toBool(), aboutcoordinator, insettings );


//    void qrsStatus(qint64 msec, QStringList listniready, QStringList listniall,
//                   QString channelold, QString idold, QString keyold,
//                   QString channelnew, QString idnew, QString keynew,
//                   bool writeold, bool changeni, bool qrsmulticastmode, QVariantMap aboutcoordinator, QVariantHash insettings);


    if(parameterssended){
        QThread::sleep(2);
        writeATcommand("ATFR");
        readDevice();
    }

    return parameterssended;
}

//-------------------------------------------------------------------------------------

bool Conf2modem::changeni(const QString &from, const QString &toni, const bool &sendatlb5)
{

    for(int i = 0, j = 0; i < 3 && !stopAll; i++){

        if(i > 0){
            QThread::sleep(3);
            readAll();
            if(j > 0){
                writeATcommand("ATCN");
                readDevice();
            }
            j = 0;
        }

        if(stopAll)
            return true;
        write2dev(QString("%1\r\n%2\r\n").arg(from).arg(toni).toUtf8());

        const QByteArray readarr = readDevice();
        if(!readarr.startsWith("OK\r\n"))
            continue;




        if(!enterCommandMode(tr("Applying new NI"))){
            j++;
            continue;
        }

        if(stopAll)
            return true;
        writeATcommand("ATLB 2");
        if(!wait4doubleOk(true, false)){
            j++;
            continue;
        }

        QThread::sleep(2);//some timeout

        if(stopAll)
            return true;
        if(!sendatlb5){

            writeATcommand("ATCN");
            readDevice();
            return true;
        }


        writeATcommand("ATDN " + toni);
        if(!readDevice().startsWith("OK"))
            return false;

        writeATcommand("ATLB 5");
        return wait4doubleOk(true, true);

    }
    return stopAll;
}

#endif
//-------------------------------------------------------------------------------------


#ifdef ENABLE_EMBEEMODEM_EXTENDED_OPERATIONS

//-------------------------------------------------------------------------------------

Conf2modem::EmbeeNetworkParamsStr Conf2modem::convert2netParams(const int &channel, const int &id, const QString &key)
{
    return EmbeeNetworkParamsStr(QString("%1").arg(channel, 0, 16).rightJustified(2, '0'), QString("%1").arg(id, 0, 16).rightJustified(4, '0'), key.left(32));
}

//-------------------------------------------------------------------------------------

Conf2modem::EmbeeNetworkParamsStr Conf2modem::convert2netParams(const QVariantMap &map, const QString &keysChIdKy)
{
    return convert2netParams(map, keysChIdKy.split(" ", QString::SkipEmptyParts));
}

Conf2modem::EmbeeNetworkParamsStr Conf2modem::convert2netParams(const QVariantMap &map, const QStringList &lkeysChIdKy)
{
    return convert2netParams(map.value(lkeysChIdKy.at(0)).toInt(), map.value(lkeysChIdKy.at(1)).toInt(), map.value(lkeysChIdKy.at(2)).toString());

}

#endif



