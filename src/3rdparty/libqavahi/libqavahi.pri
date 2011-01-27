QT += network
LIBS += -lavahi-common -lavahi-client

maemo5 {
    # Build the avahi-qt package statically for maemo5 since avahi-qt4
    # package doesn't seem to be available for fremantle
    DEFINES += QT4
    SOURCES += $$PWD/avahi-qt/qt-watch.cpp
    HEADERS += $$PWD/avahi-qt/qt-watch.h
} else {
    LIBS += -lavahi-qt4
}

# Input
SOURCES += $$PWD/qavahiservicepublisher.cpp $$PWD/qavahiservicebrowsermodel.cpp
HEADERS += $$PWD/qavahiservicepublisher.h $$PWD/qavahiservicebrowsermodel.h

