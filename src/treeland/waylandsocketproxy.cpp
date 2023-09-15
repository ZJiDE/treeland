#include "waylandsocketproxy.h"

#include <wsocket.h>

WAYLIB_SERVER_USE_NAMESPACE

namespace TreeLand {
void WaylandSocketProxy::newSocket(const QString &username, int fd)
{
    auto socket = std::make_shared<WSocket>(true);
    m_userWaylandSocket[username] = socket;
    socket->create(fd, false);

    emit socketCreated(socket);
}

QString WaylandSocketProxy::user(std::shared_ptr<Waylib::Server::WSocket> socket) const {
    return m_userWaylandSocket.key(socket);
}

void WaylandSocketProxy::deleteSocket(const QString &username)
{
    if (m_userWaylandSocket.count(username)) {
        auto socket = m_userWaylandSocket.value(username);
        m_userWaylandSocket.remove(username);
        emit socketDeleted(socket);
    }
}

void WaylandSocketProxy::activateUser(const QString &username)
{
    for (auto it = m_userWaylandSocket.begin(); it != m_userWaylandSocket.end(); ++it) {
        it.value()->setEnabled(it.key() == username);
    }

    emit userActivated(username);
}
}  // namespace TreeLand
