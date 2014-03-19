#####################################################################
# Automatically generated by qmake (2.01a) Mon Nov 15 23:01:55 2010
######################################################################

include(../plugins.pri)

#unix: !no-pkg-cfg:system(pkg-config --exists taglib) {
#    CONFIG += link_pkgconfig
#    PKGCONFIG += taglib
#    message(Use system taglib)
#} else {
    include($$PROJECTROOT/src/3rdparty/taglib/taglib.pri)
    INCLUDEPATH +=  $$PROJECTROOT/src/3rdparty/taglib \
                    $$PROJECTROOT/src/3rdparty/taglib/mpeg \
                    $$PROJECTROOT/src/3rdparty/taglib/id3v2/ \
                    $$PROJECTROOT/src/3rdparty/taglib/id3v2/frames/
    message(Use taglib from 3rdparty)
#}

QT += sql

HEADERS += musicplugin.h \
           musicparser.h \
           tagreader.h \
    lastfmprovider.h

SOURCES += musicplugin.cpp \
           musicparser.cpp \
           tagreader.cpp \
    lastfmprovider.cpp

OTHER_FILES += music.json
