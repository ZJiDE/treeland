// Copyright (C) 2023 Dingyuan Zhang <lxz@mkacg.com>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

#include <QObject>
#include <QMap>

#include <memory>

namespace Waylib::Server {
    class WSocket;
}

namespace TreeLand {
class WaylandSocketProxy : public QObject {
    Q_OBJECT
public:
    using QObject::QObject;

    void newSocket(const QString &username, int fd);
    void deleteSocket(const QString &username);

    void activateUser(const QString &username);

    QString user(std::shared_ptr<Waylib::Server::WSocket> socket) const;

Q_SIGNALS:
    void socketCreated(std::shared_ptr<Waylib::Server::WSocket> socket);
    void socketDeleted(std::shared_ptr<Waylib::Server::WSocket> socket);
    void userActivated(const QString &username);

private:
    QMap<QString, std::shared_ptr<Waylib::Server::WSocket>> m_userWaylandSocket;
};
}
