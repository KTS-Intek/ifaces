# Version=0.0.1
#Dependencies -
# type-converter
#


QT       += core network

linux-beagleboard-g++:{
   QT -= gui
}
CONFIG += c++11

android:{
#QT += androidextras
#SOURCES += snif-android-src/qtandroidftdi.cpp
#HEADERS += snif-android-src/qtandroidftdi.h
#DEFINES += ADDANDROIDFTDIUART=1
}
INCLUDEPATH  += $$PWD\
                $$PWD/../../defines/defines

HEADERS += \
    $$PWD/src/m2m-service/svahaserviceconnector.h \
    $$PWD/src/m2m-service/svahasocket.h \
    $$PWD/src/m2m-service/matildamessages.h

SOURCES += \
    $$PWD/src/m2m-service/svahaserviceconnector.cpp \
    $$PWD/src/m2m-service/svahasocket.cpp \
    $$PWD/src/m2m-service/matildamessages.cpp


