include($$PROJECTROOT/src/core-common.pri)

TEMPLATE = app
TARGET = ../../hub/qtmediahub
DEPENDPATH += .
INCLUDEPATH += $${PROJECTROOT}/src/

TEMP_DIR = $$PWD/.tmp
OBJECTS_DIR = $$TEMP_DIR/.obj
MOC_DIR = $$TEMP_DIR/.moc
RCC_DIR = $$TEMP_DIR/.rcc

include($${PROJECTROOT}/src/3rdparty/qtsingleapplication/qtsingleapplication.pri)

Qt -= gui

# Input
SOURCES += main.cpp \
           mainwindow.cpp \
           skinselector.cpp

HEADERS += mainwindow.h \
           skinselector.h

RESOURCES = main.qrc

unix:!symbian {
    maemo5 {
        target.path = $$PREFIX/bin
    } else {
        target.path = $$PREFIX/bin
    }
    INSTALLS += target
}
