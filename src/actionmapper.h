/** This file is part of Qt Media Hub**

Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).*
All rights reserved.

Contact:  Nokia Corporation qmh-development@qt-project.org

You may use this file under the terms of the BSD license
as follows:

Redistribution and use in source and binary forms, with or
without modification, are permitted provided that the following
conditions are met:
* Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.

* Neither the name of Nokia Corporation and its Subsidiary(-ies)
nor the names of its contributors may be used to endorse or promote
products derived from this software without specific prior
written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. **/

#ifndef ACTIONMAPPER_H
#define ACTIONMAPPER_H

#include <QObject>
#include <QDebug>
#include <QWeakPointer>

#ifdef QT5
#include <QtQml>
#else
#include <QtDeclarative>
#endif

class GlobalSettings;

class ActionMapper : public QObject
{
    Q_OBJECT
    Q_ENUMS(Action)
    Q_PROPERTY(QString map READ map WRITE setMap)
public:
    enum Action {
        Null = -1,
        Left,
        Up,
        Right,
        Down,
        Enter,
        Menu,
        Context,
        ContextualUp,
        ContextualDown,
        MediaPlayPause,
        MediaStop,
        MediaPrevious,
        MediaNext,
        Back,
        VolumeUp,
        VolumeDown
    };

    ActionMapper(GlobalSettings *settings, QObject *parent = 0);

    Q_INVOKABLE QStringList availableMaps() const;
    QString map() const { return m_mapName; }
    void setMap(const QString &map);

    void setRecipient(QObject *recipient);

public slots:
    void takeAction(qlonglong action) { takeAction(static_cast<Action>(action)); }
    void takeAction(Action action);
    void processKey(qlonglong key);

protected:
    bool eventFilter(QObject *obj, QEvent *event);

private:
    bool loadMapFromDisk(const QString &mapFilePath);
    void populateMap();

    static void setupQtKeyMap();
    static QHash<Action, Qt::Key> s_actionToQtKeyMap;

    QPointer<QObject> m_recipient;
    bool m_generatedEvent;
    bool m_skipGeneratedEvent;
    QString m_mapName;
    QString m_mapPath;
    QHash<int, Action> m_actionMap;
    GlobalSettings *m_settings;
};

QML_DECLARE_TYPE(ActionMapper)

#endif
