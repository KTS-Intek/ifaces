# Version=0.0.1
#Dependencies -
# type-converter
#
linux-beagleboard-g++:{
   QT -= gui
}

android:{
#QT += androidextras
#SOURCES += snif-android-src/qtandroidftdi.cpp
#HEADERS += snif-android-src/qtandroidftdi.h
#DEFINES += ADDANDROIDFTDIUART=1
}
INCLUDEPATH  += $$PWD\
                $$PWD/../../defines/defines

HEADERS += \
    $$PWD/src/m2m-service/m2mconnectionbasetypes.h \
    $$PWD/src/m2m-service/m2mconnectiondefines.h \
#    $$PWD/src/m2m-service/m2mconnector.h \
#    $$PWD/src/m2m-service/m2mconnectorbase.h \
    $$PWD/src/m2m-service/m2mtcpsockettypes.h \
    $$PWD/src/m2m-service/svahaserviceconnector.h \
    $$PWD/src/m2m-service/svahasocket.h \
    $$PWD/src/m2m-service/matildamessages.h
# \
#    $$PWD/src/m2m-service/m2mtcpsocket.h \
#    $$PWD/src/m2m-service/m2mtcpsocketbase.h

SOURCES += \
#    $$PWD/src/m2m-service/m2mconnector.cpp \
#    $$PWD/src/m2m-service/m2mconnectorbase.cpp \
    $$PWD/src/m2m-service/svahaserviceconnector.cpp \
    $$PWD/src/m2m-service/svahasocket.cpp \
    $$PWD/src/m2m-service/matildamessages.cpp
# \
#     $$PWD/src/m2m-service/m2mtcpsocket.cpp \
#    $$PWD/src/m2m-service/m2mtcpsocketbase.cpp


