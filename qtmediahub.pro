######################################################################
# Automatically generated by qmake (2.01a) Fri Jul 10 14:53:32 2009
######################################################################

TEMPLATE = app
TARGET = 
DEPENDPATH += .
INCLUDEPATH += .
TEMP_DIR = tmp
OBJECTS_DIR = $$TEMP_DIR/.obj
MOC_DIR = $$TEMP_DIR/.moc


# Input
SOURCES += src/main.cpp \
    src/frontend.cpp \
    src/backend.cpp \
    src/qmlextensions/customcursor.cpp

QT += declarative

HEADERS += src/frontend.h \
    src/backend.h \
    src/qmlextensions/customcursor.h

glgs {
    CONFIG += gl
    message(Using the OpenGL graphics system)
    DEFINES += GLGS
}

glviewport {
    CONFIG += gl
    message(Using an OpenGL viewport)
    DEFINES += GLVIEWPORT
}

gl {
    DEFINES += GL
    QT += opengl
} else {
    message(Not using GL acceleration)
}

OTHER_FILES += \
    notes.txt
