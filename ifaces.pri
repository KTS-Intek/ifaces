
#linux-beagleboard-g++:{
#   QT -= gui
#}else{
#include(../m2m-connector/m2m-connector.pri)

#}

INCLUDEPATH  += $$PWD\
                $$PWD/../../defines/defines

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


HEADERS += \
    $$PWD/src/emb/checkcurrport.h \
    $$PWD/src/emb/conf2modem.h \
    $$PWD/src/emb/conf2modemhelper.h \
    $$PWD/src/emb/embeelimits.h \
    $$PWD/src/emb/ifaceexchange.h \
    $$PWD/src/emb/ifaceexchangeserializedtypes.h \
    $$PWD/src/emb/ifaceexchangetypes.h \
    $$PWD/src/emb/peredavatorpriority.h

SOURCES += \
    $$PWD/src/emb/checkcurrport.cpp \
    $$PWD/src/emb/conf2modem.cpp \
    $$PWD/src/emb/conf2modemhelper.cpp \
    $$PWD/src/emb/ifaceexchange.cpp \
    $$PWD/src/emb/ifaceexchangeserializedtypes.cpp