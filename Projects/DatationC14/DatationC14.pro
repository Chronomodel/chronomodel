QT       += widgets

TARGET = DatationC14
TEMPLATE = lib
DEFINES += DATATIONC14_LIBRARY

# Do not create output with symlinks:
QMAKE_LN_SHLIB = :

CONFIG(debug, debug|release) {
    DESTDIR = ../../Binaries/Debug
} else {
    DESTDIR = ../../Binaries/Release
}
OBJECTS_DIR = obj
MOC_DIR = moc
RCC_DIR = rcc

INCLUDEPATH += src/
INCLUDEPATH += ../Common/

SOURCES += src/DatationC14.cpp

HEADERS += ../Common/PluginInterface.h\
        src/DatationC14.h\
        src/datationc14_global.h

unix:!symbian {
    maemo5 {
        target.path = /opt/usr/lib
    } else {
        target.path = /usr/lib
    }
    INSTALLS += target
}
