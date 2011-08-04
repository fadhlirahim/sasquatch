/****************************************************************************

This file is part of the QtMediaHub project on http://www.gitorious.org.

Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).*
All rights reserved.

Contact:  Nokia Corporation (qt-info@nokia.com)**

You may use this file under the terms of the BSD license as follows:

"Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
* Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
* Neither the name of Nokia Corporation and its Subsidiary(-ies) nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."

****************************************************************************/

#include "frontend.h"
#include "mediaserver.h"

#include <QtGui>
#include <QDebug>

#ifdef QMH_AVAHI
#include "qavahiservicebrowsermodel.h"
#else
#include "staticservicebrowsermodel.h"
#endif

#ifdef SCENEGRAPH
#include <QSGItem>
#include <QSGView>
#else
#include <QtDeclarative>
#endif

#ifndef NO_DBUS
#include <QDBusError>
#include <QDBusConnection>
#endif

#ifdef GL
#include <QGLFormat>
#endif

#ifdef GLVIEWPORT
#include <QGLWidget>
#endif

#include "mainwindow.h"

#include "qmh-config.h"
#include "dirmodel.h"
#include "media/playlist.h"
#include "file.h"
#include "media/mediamodel.h"
#include "media/mediascanner.h"
#include "actionmapper.h"
#include "rpc/rpcconnection.h"
#include "declarativeview.h"
#include "libraryinfo.h"
#include "qmldebugging.h"
#include "actionmapper.h"
#include "trackpad.h"
#include "devicemanager.h"
#include "powermanager.h"
#include "rpc/mediaplayerrpc.h"
#include "customcursor.h"
#include "httpserver/httpserver.h"
#include "skinselector.h"

#ifndef NO_DBUS
static void registerObjectWithDbus(QObject *object)
{
    bool objectRegistration =
            QDBusConnection::sessionBus().registerObject("/", object,
                                                         QDBusConnection::ExportScriptableSlots|QDBusConnection::ExportScriptableSignals);
    if (!objectRegistration)
        qDebug() << "Can't seem to register object with dbus service:" << QDBusConnection::sessionBus().lastError().message();
}
#endif

class FrontendPrivate : public QObject
{
    Q_OBJECT
public:
    FrontendPrivate(Frontend *p);
    ~FrontendPrivate();

public slots:
    QWidget *loadQmlSkin(const QUrl &url);

    void discoverSkins();

    void toggleFullScreen();
    void showFullScreen();
    void showNormal();
    void activateWindow();

    void grow();
    void shrink();

    void resetUI();

    void selectSkin();

    void handleDirChanged(const QString &dir);

public:
    void enableRemoteControlMode(bool enable);

    const QRect defaultGeometry;
    bool overscanWorkAround;
    bool attemptingFullScreen;

    bool dbusRegistration;
    bool remoteControlMode;
    MediaServer *mediaServer;
    DeviceManager *deviceManager;
    PowerManager *powerManager;
    MediaPlayerRpc *mediaPlayerRpc;
    RpcConnection *connection;

    ActionMapper *actionMapper;
    Trackpad *trackpad;
    QMap<QString, Skin *> skins;
    Skin *currentSkin;
    MainWindow *mainWindow;
    QDeclarativeContext *rootContext;
    QSystemTrayIcon *systray;
    QFileSystemWatcher pathMonitor;
    QAbstractItemModel *targetsModel;
    Frontend *q;
};

FrontendPrivate::FrontendPrivate(Frontend *p)
    : QObject(p),
      defaultGeometry(0, 0, 1080, 720),
      overscanWorkAround(Config::isEnabled("overscan", false)),
      attemptingFullScreen(Config::isEnabled("fullscreen", true)),
      dbusRegistration(false),
      remoteControlMode(true),
      mediaServer(0),
      deviceManager(0),
      powerManager(0),
      mediaPlayerRpc(0),
      connection(0),
      trackpad(0),
      mainWindow(0),
      rootContext(0),
      targetsModel(0),
      q(p)
{
#ifndef NO_DBUS
    dbusRegistration = QDBusConnection::sessionBus().registerService("com.qtmediahub");
    if (!dbusRegistration) {
        qDebug() << "Can't seem to register dbus service:" << QDBusConnection::sessionBus().lastError().message();
    }
#endif

    QPixmapCache::setCacheLimit(Config::value("cacheSize", 0)*1024);

#ifdef GL
    QGLFormat format;
    format.setSampleBuffers(true);
    format.setSwapInterval(1);
    QGLFormat::setDefaultFormat(format);
#endif //GL

    QString dejavuPath(LibraryInfo::resourcePath() % "/3rdparty/dejavu-fonts-ttf-2.32/ttf/");
    if (QDir(dejavuPath).exists()) {
        QFontDatabase::addApplicationFont(dejavuPath % "DejaVuSans.ttf");
        QFontDatabase::addApplicationFont(dejavuPath % "DejaVuSans-Bold.ttf");
        QFontDatabase::addApplicationFont(dejavuPath % "DejaVuSans-Oblique.ttf");
        QFontDatabase::addApplicationFont(dejavuPath % "DejaVuSans-BoldOblique.ttf");
        QApplication::setFont(QFont("DejaVu Sans"));
    }

    qApp->setOverrideCursor(Qt::BlankCursor);

    qRegisterMetaType<QModelIndex>();

    //Declarative is a hard dependency at present in any case
    // register dataproviders to QML
    qmlRegisterUncreatableType<ActionMapper>("ActionMapper", 1, 0, "ActionMapper", "For enums. For methods use actionmap global variable");
    qmlRegisterType<DirModel>("DirModel", 1, 0, "DirModel");
    qmlRegisterType<File>("File", 1, 0, "File");
    qmlRegisterType<Playlist>("Playlist", 1, 0, "Playlist");
    qmlRegisterType<MediaModel>("MediaModel", 1, 0, "MediaModel");
    qmlRegisterType<RpcConnection>("RpcConnection", 1, 0, "RpcConnection");

    connect(&pathMonitor, SIGNAL(directoryChanged(QString)), this, SLOT(handleDirChanged(QString)));
    foreach (const QString &skinPath, LibraryInfo::skinPaths()) {
        if (QDir(skinPath).exists())
            pathMonitor.addPath(skinPath);
    }

    QList<QAction*> actions;
    QAction *selectSkinAction = new QAction(tr("Select skin"), this);
    QAction *quitAction = new QAction(tr("Quit"), this);
    connect(selectSkinAction, SIGNAL(triggered()), this, SLOT(selectSkin()));
    connect(quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));
    actions.append(selectSkinAction);
    actions.append(quitAction);

    if (Config::isEnabled("systray", true)) {
        systray = new QSystemTrayIcon(QIcon(":/images/petite-ganesh-22x22.jpg"), this);
        systray->setVisible(true);
        QMenu *contextMenu = new QMenu;
        contextMenu->addActions(actions);
        systray->setContextMenu(contextMenu);
    }

    discoverSkins();
}

FrontendPrivate::~FrontendPrivate()
{
    Config::setEnabled("fullscreen", attemptingFullScreen);
    Config::setValue("skin", currentSkin->name());
    Config::setEnabled("overscan", overscanWorkAround);

    Config::setValue("desktop-id", qApp->desktop()->screenNumber(mainWindow));

    if (!attemptingFullScreen)
        Config::setValue("window-geometry", mainWindow->geometry());
    else if (overscanWorkAround)
        Config::setValue("overscan-geometry", mainWindow->geometry());

    delete mainWindow;
}

static void optimizeWidgetAttributes(QWidget *widget, bool transparent = false)
{
    widget->setAttribute(Qt::WA_OpaquePaintEvent);
    widget->setAutoFillBackground(false);
    if (transparent && Config::isEnabled("shine-through", false))
        widget->setAttribute(Qt::WA_TranslucentBackground);
    else
        widget->setAttribute(Qt::WA_NoSystemBackground);
}

static void optimizeGraphicsViewAttributes(QGraphicsView *view)
{
    if (Config::isEnabled("smooth-scaling", false))
        view->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::TextAntialiasing);

    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setFrameStyle(0);
    view->setOptimizationFlags(QGraphicsView::DontSavePainterState);
    view->scene()->setItemIndexMethod(QGraphicsScene::NoIndex);
}

QWidget *FrontendPrivate::loadQmlSkin(const QUrl &targetUrl)
{
    QPixmapCache::clear();

#ifdef SCENEGRAPH
    QSGView *declarativeWidget = new QSGView;
    declarativeWidget->setResizeMode(QSGView::SizeRootObjectToView);
#else
    DeclarativeView *declarativeWidget = new DeclarativeView;

    optimizeWidgetAttributes(declarativeWidget);
    optimizeGraphicsViewAttributes(declarativeWidget);
    declarativeWidget->setResizeMode(QDeclarativeView::SizeRootObjectToView);

    if (Config::isEnabled("use-gl", true)) {
#ifdef GLVIEWPORT
        QGLWidget *viewport = new QGLWidget(declarativeWidget);
        //Why do I have to set this here?
        //Why can't I set it in the MainWindow?
        optimizeWidgetAttributes(viewport, false);

        declarativeWidget->setViewport(viewport);
#endif //GLVIEWPORT
        declarativeWidget->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    } else {
        declarativeWidget->setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
    }
#endif //SCENEGRAPH

    QDeclarativeEngine *engine = declarativeWidget->engine();
    QObject::connect(engine, SIGNAL(quit()), qApp, SLOT(quit()));

    QDeclarativePropertyMap *runtime = new QDeclarativePropertyMap(declarativeWidget);
    if (!remoteControlMode) {
        runtime->insert("mediaScanner", qVariantFromValue(static_cast<QObject *>(mediaServer->mediaScanner())));
        runtime->insert("httpServer", qVariantFromValue(static_cast<QObject *>(mediaServer->httpServer())));
        actionMapper->setRecipient(declarativeWidget);
        trackpad->setRecipient(declarativeWidget);
        runtime->insert("actionMapper", qVariantFromValue(static_cast<QObject *>(actionMapper)));
        runtime->insert("trackpad", qVariantFromValue(static_cast<QObject *>(trackpad)));
        runtime->insert("mediaPlayerRpc", qVariantFromValue(static_cast<QObject *>(mediaPlayerRpc)));
        runtime->insert("deviceManager", qVariantFromValue(static_cast<QObject *>(deviceManager)));
        runtime->insert("powerManager", qVariantFromValue(static_cast<QObject *>(powerManager)));
    }
    runtime->insert("config", qVariantFromValue(static_cast<QObject *>(Config::instance())));
    runtime->insert("frontend", qVariantFromValue(static_cast<QObject *>(q)));
    runtime->insert("window", qVariantFromValue(static_cast<QObject *>(mainWindow)));
    runtime->insert("view", qVariantFromValue(static_cast<QObject *>(declarativeWidget)));
    runtime->insert("cursor", qVariantFromValue(static_cast<QObject *>(new CustomCursor(declarativeWidget))));
    runtime->insert("skin", qVariantFromValue(static_cast<QObject *>(currentSkin)));
    declarativeWidget->rootContext()->setContextProperty("runtime", runtime);

    engine->addImportPath(LibraryInfo::basePath() % "/imports");
    engine->addImportPath(currentSkin->path());

    rootContext = declarativeWidget->rootContext();
    declarativeWidget->installEventFilter(q); // track idleness
    declarativeWidget->setSource(targetUrl);

    return declarativeWidget;
}

void FrontendPrivate::showFullScreen()
{
    attemptingFullScreen = true;
    overscanWorkAround = Config::isEnabled("overscan", false);

    if (overscanWorkAround) {
        QRect geometry = Config::value("overscan-geometry", defaultGeometry);
        geometry.moveCenter(qApp->desktop()->availableGeometry().center());

        mainWindow->setGeometry(geometry);
        mainWindow->setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
        mainWindow->setWindowState(Qt::WindowNoState);
        mainWindow->show();
    } else {
        mainWindow->showFullScreen();
    }

    QTimer::singleShot( 1, this, SLOT(activateWindow()));
}

void FrontendPrivate::showNormal()
{
    attemptingFullScreen = overscanWorkAround = false;

    mainWindow->setWindowFlags(Qt::Window);
    mainWindow->setGeometry(Config::value("window-geometry", defaultGeometry));
    mainWindow->showNormal();

    QTimer::singleShot( 1, this, SLOT(activateWindow()));
}

void FrontendPrivate::activateWindow()
{
    //Invoking this by adding it to the even queue doesn't work?
    mainWindow->activateWindow();
}

void FrontendPrivate::grow()
{
    if (!attemptingFullScreen)
        return;

    const QRect newGeometry = mainWindow->geometry().adjusted(-1,-1,1,1);

    const QSize desktopSize = qApp->desktop()->screenGeometry(mainWindow).size();
    if ((newGeometry.width() > desktopSize.width())
            || (newGeometry.height() > desktopSize.height())) {
        Config::setEnabled("overscan", false);
        showFullScreen();
    } else {
        mainWindow->setGeometry(newGeometry);
    }
}

void FrontendPrivate::shrink()
{
    if (!attemptingFullScreen)
        return;

    if (!overscanWorkAround) {
        Config::setEnabled("overscan");
        showFullScreen();
    }
    mainWindow->setGeometry(mainWindow->geometry().adjusted(1,1,-1,-1));
}

void FrontendPrivate::resetUI()
{
    if (QDeclarativeView *declarativeWidget = qobject_cast<QDeclarativeView*>(mainWindow->centralWidget())) {
        QObject *coreObject = declarativeWidget->rootObject();
        QMetaObject::invokeMethod(coreObject, "reset");
    }
}

void FrontendPrivate::toggleFullScreen()
{
    if (attemptingFullScreen) {
        Config::setValue("overscan-geometry", mainWindow->geometry());
        showNormal();
    } else {
        Config::setValue("window-geometry", mainWindow->geometry());
        showFullScreen();
    }
}

void FrontendPrivate::discoverSkins()
{
    qDeleteAll(skins.values());
    skins.clear();

    foreach (const QString &skinPath, LibraryInfo::skinPaths()) {
        QStringList potentialSkins = QDir(skinPath).entryList(QDir::Dirs | QDir::NoDotAndDotDot);

        foreach(const QString &currentPath, potentialSkins) {
            const QString prospectivePath = skinPath % "/" % currentPath;
            if (Skin *skin = Skin::createSkin(prospectivePath, this))
                skins.insert(skin->name(), skin);
        }
    }

    if (skins.isEmpty()) {
        qWarning() << "No skins are found in your skin paths"<< endl \
                   << "If you don't intend to run this without skins"<< endl \
                   << "Please read the INSTALL doc available here:" \
                   << "http://gitorious.org/qtmediahub/qtmediahub-core/blobs/master/INSTALL" \
                   << "for further details";
    } else {
        QStringList sl;
        foreach(Skin *skin, skins)
            sl.append(skin->name());
        qDebug() << "Available skins:" << sl.join(",");
    }
}

void FrontendPrivate::selectSkin()
{
    SkinSelector *skinSelector = new SkinSelector(q, mainWindow);
    skinSelector->setAttribute(Qt::WA_DeleteOnClose);
    skinSelector->exec();
}

void FrontendPrivate::handleDirChanged(const QString &dir)
{
    if (LibraryInfo::skinPaths().contains(dir)) {
        qWarning() << "Changes in skin path, repopulating skins";
        discoverSkins();
    }
}

Frontend::Frontend(QObject *p)
    : QObject(p),
      d(new FrontendPrivate(this)) 
{
    d->mainWindow = new MainWindow;
    optimizeWidgetAttributes(d->mainWindow, true);

    connect(d->mainWindow, SIGNAL(grow()), d, SLOT(grow()));
    connect(d->mainWindow, SIGNAL(shrink()), d, SLOT(shrink()));
    connect(d->mainWindow, SIGNAL(resetUI()), d, SLOT(resetUI()));
    connect(d->mainWindow, SIGNAL(toggleFullScreen()), d, SLOT(toggleFullScreen()));
}

Frontend::~Frontend()
{
    delete d;
    d = 0;
}

bool Frontend::transforms() const
{
#ifdef GL
    return (QGLFormat::hasOpenGL() && Config::isEnabled("transforms", true));
#else
    return false;
#endif
}

void Frontend::show()
{
    if (d->attemptingFullScreen) {
        d->showFullScreen();
    } else {
        d->showNormal();
    }
}

bool Frontend::setSkin(const QString &name)
{
    Skin *newSkin = d->skins.value(name);
    if (!newSkin) {
        newSkin = d->skins.value(Config::value("default-skin", "confluence").toString());
    }

    if (!newSkin) {
        qDebug() << "Failed to set skin:" << name;
        return false;
    }

    return setSkin(newSkin);
}

bool Frontend::setSkin(Skin *skin)
{
    QSize nativeResolution = qApp->desktop()->screenGeometry().size();
    QString nativeResolutionString = Config::value("native-res-override", QString("%1x%2").arg(nativeResolution.width()).arg(nativeResolution.height()));

    QUrl url = skin->urlForResolution(nativeResolutionString, Config::value("fallback-resolution", "default").toString());
    if (!url.isValid()) {
        qWarning() << "Error loading skin " << skin->name();
        return false;
    }

    if (skin->type(url) != Skin::Qml)
        return false;

    d->currentSkin = skin;
    d->enableRemoteControlMode(skin->isRemoteControl());
    mainWindow()->setCentralWidget(d->loadQmlSkin(url));
    return true;
}

void Frontend::addImportPath(const QString &path)
{
    if (QFile::exists(path))
        d->rootContext->engine()->addImportPath(path);
}

QList<Skin *> Frontend::skins() const
{
    return d->skins.values();
}

MainWindow *Frontend::mainWindow() const
{
    return d->mainWindow;
}

QObject *Frontend::targetsModel() const
{
    if (!d->targetsModel) {
#ifdef QMH_AVAHI
        if (Config::isEnabled("avahi", true)) {
            QAvahiServiceBrowserModel *model = new QAvahiServiceBrowserModel(const_cast<Frontend *>(this));
            model->setAutoResolve(true);
            QAvahiServiceBrowserModel::Options options = QAvahiServiceBrowserModel::NoOptions;
            if (Config::isEnabled("avahi-hide-ipv6"), true)
                options |= QAvahiServiceBrowserModel::HideIPv6;
            if (Config::isEnabled("avahi-hide-local", true) && !Config::isEnabled("testing", false))
                options |= QAvahiServiceBrowserModel::HideLocal;
            model->browse("_qtmediahub._tcp", options);
            d->targetsModel = model;
        }
#else
        d->targetsModel = new StaticServiceBrowserModel(const_cast<Frontend *>(this));
#endif
    }
    return d->targetsModel;
}

QStringList Frontend::findApplications() const
{
    QStringList apps;
    foreach(const QString &appSearchPath, LibraryInfo::applicationPaths()) {
        QStringList subdirs = QDir(appSearchPath).entryList(QDir::AllDirs | QDir::NoDotAndDotDot);
        foreach(const QString &subdir, subdirs)  {
            QString appPath(appSearchPath + '/' + subdir + '/');
            QString fileName(appPath + "qmhmanifest.qml"); // look for apps/x/qmhmanifest.qml
            QFile prospectiveFile(fileName);
            if (prospectiveFile.exists())
                apps << (QDir(appPath).absolutePath() + '/');
        }
    }
    return apps;
}

void FrontendPrivate::enableRemoteControlMode(bool enable)
{
    if (remoteControlMode == enable)
        return;

    remoteControlMode = enable;

    if (enable) {
        delete deviceManager;
        deviceManager = 0;
        delete powerManager;
        powerManager = 0;

        connection->unregisterObject(mediaPlayerRpc);
        delete mediaPlayerRpc;
        mediaPlayerRpc = 0;

        connection->unregisterObject(trackpad);
        delete trackpad;
        trackpad = 0;

        connection->unregisterObject(actionMapper);
        delete actionMapper;
        actionMapper = 0;

        delete connection;

        delete mediaServer;
        mediaServer = 0;

        return;
    }

    mediaServer = new MediaServer(this);
    mediaPlayerRpc = new MediaPlayerRpc(this);
    connection = new RpcConnection(RpcConnection::Server, QHostAddress::Any, 1234, this);
    trackpad = new Trackpad(this);
    actionMapper = new ActionMapper(this, LibraryInfo::basePath());

    connection->registerObject(actionMapper);
    connection->registerObject(mediaPlayerRpc);
    connection->registerObject(trackpad);

#ifndef NO_DBUS
//Segmentation fault on mac!
    if (QDBusConnection::systemBus().isConnected()) {
        deviceManager = new DeviceManager(this);
        powerManager = new PowerManager(this);
    }

    if (dbusRegistration) {
        ::registerObjectWithDbus(mediaPlayerRpc);
    }
#endif
}

void Frontend::openUrlExternally(const QUrl & url) const
{
    QDesktopServices::openUrl(url);
}

QString Frontend::resourcePath() const
{
    return LibraryInfo::resourcePath();
}

#include "frontend.moc"
