# Version=0.0.1
#
#
#Dependencies -
# ifaces
# type-converter
#
#
# Defines
# ENABLE_EXTSUPPORT_OF_IFACES - it enables SerialPort and m2m-client
# ENABLE_BIGGER_BLOCKTIMEOUT - value is msec, default value is 50
# DISABLE_QUICK_READMODE - do not reduce timeouts less than 1000 - global and 500 msec for block when the modem is reading
#
# DISABLE_M2M_MODULE
# DISABLE_TCPCLIENT_MODE
# DISABLE_SERIALPORT_MODE
# ISNATIVEIFACE

#linux-beagleboard-g++:{
#   QT -= gui
#}else{
#include(../m2m-connector/m2m-connector.pri)

#}

INCLUDEPATH  += $$PWD\
                $$PWD/../../defines/defines
                $$PWD/../../defines/define-types
#isEmpty(ENABLE_EXTSUPPORT_OF_IFACES){
#message(ENABLE_EXTSUPPORT_OF_IFACES is disabled)

#}else{
#message(ENABLE_EXTSUPPORT_OF_IFACES is enabled)
#include(../m2m-connector/m2m-connector.pro)
#}


#android:{
#QT += androidextras
#SOURCES += snif-android-src/qtandroidftdi.cpp
#HEADERS += snif-android-src/qtandroidftdi.h
#DEFINES += ADDANDROIDFTDIUART=1
#}
contains( QT, network) {
    DEFINES += HASNETWORKQTLIB
    contains(DEFINES, ENABLE_EXTSUPPORT_OF_IFACES):!contains(DEFINES, DISABLE_M2M_MODULE){
        include(../m2m-connector/m2m-connector.pri)
        message($$TARGET ", ifaces: m2m-connector is enabled")
    }else{

        contains(DEFINES, ENABLE_EXTSUPPORT_OF_IFACES)
            message($$TARGET ", ifaces: m2m-connector is disabled, !ENABLE_EXTSUPPORT_OF_IFACES")

        contains(DEFINES, DISABLE_M2M_MODULE)
            message($$TARGET ", ifaces: m2m-connector is disabled, DISABLE_M2M_MODULE")


    }

}
contains( QT, serialport) {
    contains(DEFINES, DISABLE_SERIALPORT_MODE){
        message($$TARGET ", checkcurrport: DISABLE_SERIALPORT_MODE")
    }else{
        DEFINES += HASSERIALPORT
        HEADERS += \
            $$PWD/src/emb/checkcurrport.h

        SOURCES += \
            $$PWD/src/emb/checkcurrport.cpp
    }
}


HEADERS += \
    $$PWD/src/emb/conf2modem.h \
    $$PWD/src/emb/conf2modemhelper.h \
    $$PWD/src/emb/embeelimits.h \
    $$PWD/src/emb/ifaceexchange.h \
    $$PWD/src/emb/ifaceexchangeserializedtypes.h \
    $$PWD/src/emb/ifaceexchangetypes.h \
    $$PWD/src/emb/peredavatorpriority.h \
    $$PWD/src/emb/conn2modem.h \
    $$PWD/src/emb/ifaceconnectiondefs.h \
    $$PWD/src/emb/serial2tcpperedavator.h \
    $$PWD/src/shared/peredavatorpriority.h \
    $$PWD/src/emb/ifaceexchangetypesdefs.h \
    $$PWD/src/emb/embnodediscoverytypes.h \
    $$PWD/src/emb/embnodediscoveryconverter.h

SOURCES += \
    $$PWD/src/emb/conf2modem.cpp \
    $$PWD/src/emb/conf2modemhelper.cpp \
    $$PWD/src/emb/ifaceexchange.cpp \
    $$PWD/src/emb/ifaceexchangeserializedtypes.cpp \
    $$PWD/src/emb/conn2modem.cpp \
    $$PWD/src/emb/embnodediscoveryconverter.cpp \
    $$PWD/src/emb/serial2tcpperedavator.cpp
