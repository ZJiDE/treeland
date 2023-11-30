/***************************************************************************
* Copyright (c) 2013 Abdurrahman AVCI <abdurrahmanavci@gmail.com>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the
* Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
***************************************************************************/

#ifndef SDDM_USERMODEL_H
#define SDDM_USERMODEL_H

#include <QAbstractListModel>
#include <QQmlEngine>
#include <QHash>

struct UserModelPrivate;

class UserModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int lastIndex READ lastIndex CONSTANT)
    Q_PROPERTY(QString lastUser READ lastUser CONSTANT)
    Q_PROPERTY(int count READ rowCount CONSTANT)
    Q_PROPERTY(int disableAvatarsThreshold READ disableAvatarsThreshold CONSTANT)
    Q_PROPERTY(bool containsAllUsers READ containsAllUsers CONSTANT)
    QML_ELEMENT
public:
    enum UserRoles {
        NameRole = Qt::UserRole + 1,
        RealNameRole,
        HomeDirRole,
        IconRole,
        NeedsPasswordRole,
        LoginedRole,
        IdentityRole
    };
    Q_ENUM(UserRoles)

    UserModel(const UserModel&) = delete;
    UserModel& operator=(const UserModel&) = delete;
    UserModel(bool needAllUsers = true,QObject *parent = nullptr);
    ~UserModel() override;

    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;
    [[nodiscard]] int lastIndex() const;
    [[nodiscard]] QString lastUser() const;
    [[nodiscard]] int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] Q_INVOKABLE QVariant get(const QString &username) const;
    [[nodiscard]] Q_INVOKABLE QVariant get(int index) const;
    void updateUserLoginState(const QString &username, bool logined);
    [[nodiscard]] static int disableAvatarsThreshold();
    [[nodiscard]] bool containsAllUsers() const;

private:
    UserModelPrivate *d {nullptr};
};

#endif // USERMODEL_H
