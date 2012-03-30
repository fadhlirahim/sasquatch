QMAKE_CACHE=$${OUT_PWD}/.qmake.cache
BASE_NAME=$$PWD

# Detect stale .qmake.cache
exists($$QMAKE_CACHE) {
    !equals(PROJECTROOT, $$BASE_NAME): system(rm $$QMAKE_CACHE)
}

linux* {
    # If you change the defaults here, fix src/common.pri
    message(We set these variables in the toplevel .qmake.cache file)
    message(that qmake ascends to in perpetuity)
    message()
    message(Linux specific options: ('*' indicates that option is enabled by default))
    message(Use '-config no-<option>' to disable or '-config <option>' to enable)
    message(glviewport* - Render using a GL Viewport)
    message(glgs - Render using GL graphics system (Avoid like the plague!))
}

isEmpty(PROJECTROOT) {
    message()
    system(echo "PROJECTROOT=$$BASE_NAME" > $$QMAKE_CACHE)
#$$LITERAL_HASH is not jiving in system( calls fine in echo(
    system(echo "false: CONFIG+=no-pkg-cfg no-dbus" >> $$QMAKE_CACHE)
    system(echo "CONFIG-=qt_framework" >> $$QMAKE_CACHE)
    system(echo "CONFIG+=cache-exists" >> $$QMAKE_CACHE)
    message(Cache created)
    message(Please rerun qmake)
    system(echo "equals\\(QT_MAJOR_VERSION, \\\"5\\\"\\) CONFIG+=qt5" >> $$QMAKE_CACHE)
}

