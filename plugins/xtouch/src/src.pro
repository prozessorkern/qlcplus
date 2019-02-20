include(../../../variables.pri)
include(../../../coverage.pri)

TEMPLATE = lib
LANGUAGE = C++
TARGET   = xtouch

QT      += network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG      += plugin
INCLUDEPATH += ../../interfaces
DEPENDPATH  += ../../interfaces

win32:QMAKE_LFLAGS += -shared

# This must be after "TARGET = " and before target installation so that
# install_name_tool can be run before target installation
macx:include(../../../platforms/macos/nametool.pri)

target.path = $$INSTALLROOT/$$PLUGINDIR
INSTALLS   += target

HEADERS += ../../interfaces/qlcioplugin.h
HEADERS +=  xtouchplugin.h \
            xtouchcontroller.h \
            configurextouch.h

FORMS += configurextouch.ui

SOURCES += ../../interfaces/qlcioplugin.cpp
SOURCES +=  xtouchplugin.cpp \
            xtouchcontroller.cpp \
            configurextouch.cpp

unix:!macx {
    metainfo.path   = $$METAINFODIR
    metainfo.files += org.qlcplus.QLCPlus.artnet.metainfo.xml
    INSTALLS       += metainfo
}
