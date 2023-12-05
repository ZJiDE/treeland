// Copyright (C) 2023 Dingyuan Zhang <zhangdingyuan@uniontech.com>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "treelandhelper.h"

#include <WServer>
#include <WOutput>
#include <WSurfaceItem>
#include <QFile>
#include <qnamespace.h>
#include <qwcompositor.h>

#include <QRegularExpression>

extern "C" {
#define static
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_compositor.h>
#undef static
}

TreeLandHelper::TreeLandHelper(QObject *parent)
    : Helper(parent)
{}

bool TreeLandHelper::beforeDisposeEvent(WSeat *seat, QWindow *watched, QInputEvent *event)
{
    // TODO: shortcut
    if (event->type() == QEvent::KeyPress) {
        auto e = static_cast<QKeyEvent*>(event);
        emit keyEvent(e->key(), e->modifiers());
    }

    // Alt+Tab switcher
    // TODO: move to mid handle
    auto e = static_cast<QKeyEvent*>(event);
    bool isSwitcher = false;
    bool isPress = event->type() == QEvent::KeyPress;

    switch (e->key()) {
        case Qt::Key_Tab: {
            if (e->modifiers() == Qt::AltModifier) {
                if (m_switcherCurrentMode == Switcher::Hide) {
                    m_switcherCurrentMode = Switcher::Show;
                }
                else {
                    m_switcherCurrentMode = Switcher::Next;
                }

                isSwitcher = true;
            }
            else if (e->modifiers() == (Qt::AltModifier | Qt::ShiftModifier)) {
                if (m_switcherCurrentMode == Switcher::Hide) {
                    m_switcherCurrentMode = Switcher::Show;
                }
                else {
                    m_switcherCurrentMode = Switcher::Previous;
                }

                isSwitcher = true;
            }

            if (isSwitcher) {
                if (isPress) {
                    Q_EMIT switcherChanged(m_switcherCurrentMode);
                }
                return true;
            }
        }
        break;
        default: {
            if (m_switcherCurrentMode != Switcher::Hide) {
                m_switcherCurrentMode = Switcher::Hide;
                Q_EMIT switcherChanged(m_switcherCurrentMode);
            }
        }
        break;
    }


    return Helper::beforeDisposeEvent(seat, watched, event);
}

QString TreeLandHelper::socketFile() const
{
    return m_socketFile;
}

void TreeLandHelper::setSocketFile(const QString &socketFile)
{
    m_socketFile = socketFile;

    emit socketFileChanged();
}

QString TreeLandHelper::clientName(Waylib::Server::WSurface *surface) const
{
    wl_client *client = surface->handle()->handle()->resource->client;
    pid_t pid;
    uid_t uid;
    gid_t gid;
    wl_client_get_credentials(client, &pid, &uid, &gid);

    QString programName;
    QFile file(QString("/proc/%1/status").arg(pid));
    if (file.open(QFile::ReadOnly)) {
        programName = QString(file.readLine()).section(QRegularExpression("([\\t ]*:[\\t ]*|\\n)"),1,1);
    }

    qDebug() << "Program name for PID" << pid << "is" << programName;
    return programName;
}
