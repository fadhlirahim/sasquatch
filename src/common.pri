# This is a common file for the corelib, plugins and main
# Add with care

isEmpty(PROJECTROOT) {
    message("Building without a rational PROJECTROOT is undefined")
    error("Did you run configure?")
}

DEFINES += GL
DEFINES += QT5

QT += network sql quick
# widgets #QDirModel from widgets!

TEMP_DIR = .tmp
OBJECTS_DIR = $$TEMP_DIR/.obj
MOC_DIR = $$TEMP_DIR/.moc

isEmpty(PREFIX) {
    #Use our fuzzy heuristic logic for prefix setting
    mac {
        PREFIX=$$PROJECTROOT
    }
    maemo5 {
        PREFIX=/opt/usr/
    }
    harmattan {
        PREFIX=/opt/qtmediahub/
    }
    isEmpty(PREFIX) {
        PREFIX=/usr/local/
    }
}

DEFINES += "QMH_PREFIX=\\\"$$PREFIX\\\""
DEFINES += "QMH_PROJECTROOT=\\\"$$PROJECTROOT\\\""
DEFINES += "QMH_DBUS_SERVICENAME=\\\"com.nokia.qtmediahub\\\""
DEFINES += "QMH_PLAYER_DBUS_SERVICENAME=\\\"com.nokia.qtmediahub.player\\\""

PKG_CONFIG_PATH=$$(PKG_CONFIG_PATH)
mac:isEmpty(PKG_CONFIG_PATH) {
    message(pkg-config usage on mac requires:)
    message(export PKG_CONFIG_PATH=/opt/local/lib/pkgconfig)
}

!contains(QT_CONFIG, dbus) {
    #This catches dbus free Qt builds
    CONFIG += no-dbus
}

no-dbus {
    message(Disabling dbus support)
    DEFINES += NO_DBUS
}

CONFIG(debug, debug|release):!qt5:CONFIG += declarative_debug

# Include JS debugger library if QMLJSDEBUGGER_PATH is set
!isEmpty(QMLJSDEBUGGER_PATH) {
    include($$QMLJSDEBUGGER_PATH/qmljsdebugger-lib.pri)
} else {
    DEFINES -= QMLJSDEBUGGER
}
