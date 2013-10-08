QT += widgets

OBJECTS_DIR = obj
MOC_DIR = moc
RCC_DIR = rcc

INCLUDEPATH += ../Common/
INCLUDEPATH += src/
INCLUDEPATH += src/Views/
INCLUDEPATH += src/Controllers/
INCLUDEPATH += src/Models/

HEADERS += \
    ../Common/PluginInterface.h \
    src/Views/MainWindow.h \
    src/Views/GraphView.h \
    src/Views/MainWidget.h \
    src/Controllers/MainController.h \
    src/Views/GraphViewAbstract.h \
    src/Views/GraphProperties.h

SOURCES += src/main.cpp \
    src/Views/MainWindow.cpp \
    src/Views/GraphView.cpp \
    src/Views/MainWidget.cpp \
    src/Controllers/MainController.cpp \
    src/Views/GraphViewAbstract.cpp

CONFIG(debug, debug|release) {
    DESTDIR = ../../Binaries/Debug
} else {
    DESTDIR = ../../Binaries/Release
}
