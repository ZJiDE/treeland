// Copyright (C) 2023 WenHao Peng <pengwenhao@uniontech.com>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "personalizationmanager.h"

#include "server-protocol.h"

#include <wxdgshell.h>
#include <wxdgsurface.h>

#include <qwcompositor.h>
#include <qwdisplay.h>
#include <qwsignalconnector.h>
#include <qwxdgshell.h>

#include <QDebug>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QStandardPaths>

#include <sys/socket.h>
#include <unistd.h>

extern "C" {
#define static
#include <wlr/types/wlr_xdg_shell.h>
#undef static
}

DCORE_USE_NAMESPACE

static QuickPersonalizationManager *PERSONALIZATION_MANAGER = nullptr;
static QQuickItem *PERSONALIZATION_IMAGE = nullptr;

#define DEFAULT_WALLPAPER "qrc:/desktop.webp"

static QString outputName(const WOutput *w_output)
{
    if (!w_output)
        return QString();

    wlr_output *output = w_output->nativeHandle();
    if (!output)
        return QString();

    return output->name;
}

void QuickPersonalizationManager::updateCacheWallpaperPath(uid_t uid)
{
    QString cache_location = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    m_cacheDirectory = cache_location + QString("/wallpaper/%1/").arg(uid);
    m_settingFile = m_cacheDirectory + "wallpaper.ini";

    QSettings settings(m_settingFile, QSettings::IniFormat);
    m_iniMetaData = settings.value("metadata").toString();
}

QString QuickPersonalizationManager::readWallpaperSettings(const QString &group, WOutput *w_output)
{
    if (m_settingFile.isEmpty())
        return DEFAULT_WALLPAPER;

    QSettings settings(m_settingFile, QSettings::IniFormat);

    QString value;
    QString output_name = outputName(w_output);
    if (output_name.isEmpty())
        return DEFAULT_WALLPAPER;
    else {
        value = settings.value(group + "/" + output_name, DEFAULT_WALLPAPER).toString();
    }

    return QString("file://%1").arg(value);
}

void QuickPersonalizationManager::saveWallpaperSettings(
    const QString &current, personalization_wallpaper_context_v1 *context)
{
    if (m_settingFile.isEmpty() || context == nullptr)
        return;

    QSettings settings(m_settingFile, QSettings::IniFormat);

    if (context->options & PERSONALIZATION_WALLPAPER_CONTEXT_V1_OPTIONS_BACKGROUND) {
        settings.setValue(QString("background/%s").arg(context->output_name), current);
        settings.setValue(QString("background/%s/tone").arg(context->output_name), context->isdark);
    }

    if (context->options & PERSONALIZATION_WALLPAPER_CONTEXT_V1_OPTIONS_LOCKSCREEN) {
        settings.setValue(QString("lockscreen/%s").arg(context->output_name), current);
        settings.setValue(QString("background/%s/tone").arg(context->output_name), context->isdark);
    }

    settings.setValue("metadata", context->meta_data);
    m_iniMetaData = context->meta_data;
}

QuickPersonalizationManager::QuickPersonalizationManager(QObject *parent)
    : WQuickWaylandServerInterface(parent)
    , m_cursorConfig(DConfig::create("org.deepin.Treeland", "org.deepin.Treeland", QString()))
{
    if (PERSONALIZATION_MANAGER) {
        qFatal("There are multiple instances of QuickPersonalizationManager");
    }

    PERSONALIZATION_MANAGER = this;
}

WServerInterface *QuickPersonalizationManager::create()
{
    m_manager = treeland_personalization_manager_v1::create(server()->handle());
    connect(m_manager,
            &treeland_personalization_manager_v1::windowContextCreated,
            this,
            &QuickPersonalizationManager::onWindowContextCreated);
    connect(m_manager,
            &treeland_personalization_manager_v1::wallpaperContextCreated,
            this,
            &QuickPersonalizationManager::onWallpaperContextCreated);
    connect(m_manager,
            &treeland_personalization_manager_v1::cursorContextCreated,
            this,
            &QuickPersonalizationManager::onCursorContextCreated);
    return new WServerInterface(m_manager, m_manager->global);
}

void QuickPersonalizationManager::onWindowContextCreated(personalization_window_context_v1 *context)
{
    connect(context,
            &personalization_window_context_v1::backgroundTypeChanged,
            this,
            &QuickPersonalizationManager::onBackgroundTypeChanged);
}

void QuickPersonalizationManager::onWallpaperContextCreated(
    personalization_wallpaper_context_v1 *context)
{
    connect(context,
            &personalization_wallpaper_context_v1::commit,
            this,
            &QuickPersonalizationManager::onWallpaperCommit);
    connect(context,
            &personalization_wallpaper_context_v1::getWallpapers,
            this,
            &QuickPersonalizationManager::onGetWallpapers);
}

void QuickPersonalizationManager::onCursorContextCreated(personalization_cursor_context_v1 *context)
{
    connect(context,
            &personalization_cursor_context_v1::commit,
            this,
            &QuickPersonalizationManager::onCursorCommit);
    connect(context,
            &personalization_cursor_context_v1::get_theme,
            this,
            &QuickPersonalizationManager::onGetCursorTheme);
    connect(context,
            &personalization_cursor_context_v1::get_size,
            this,
            &QuickPersonalizationManager::onGetCursorSize);
}

QString QuickPersonalizationManager::saveImage(personalization_wallpaper_context_v1 *context,
                                               const QString prefix)
{
    if (!context || context->fd == -1 || context->output_name.isEmpty())
        return QString();

    QFile src_file;
    if (!src_file.open(context->fd, QIODevice::ReadOnly))
        return QString();

    QByteArray data = src_file.readAll();
    src_file.close();

    QString dest_path = m_cacheDirectory + prefix + "_" + context->output_name;

    QDir dir(m_cacheDirectory);
    if (!dir.exists()) {
        dir.mkpath(m_cacheDirectory);
    }

    QFile dest_file(dest_path);
    if (dest_file.open(QIODevice::WriteOnly)) {
        dest_file.write(data);
        dest_file.close();

        saveWallpaperSettings(dest_path, context);
        return dest_path;
    }

    return QString();
}

void QuickPersonalizationManager::onWallpaperCommit(personalization_wallpaper_context_v1 *context)
{
    if (context->options & PERSONALIZATION_WALLPAPER_CONTEXT_V1_OPTIONS_BACKGROUND) {
        QString background = saveImage(context, "background");
        if (!background.isEmpty()) {
            Q_EMIT backgroundChanged(context->output_name, context->isdark);
        }
    }

    if (context->options & PERSONALIZATION_WALLPAPER_CONTEXT_V1_OPTIONS_LOCKSCREEN) {
        QString lockscreen = saveImage(context, "lockscreen");
        if (!lockscreen.isEmpty()) {
            Q_EMIT backgroundChanged(context->output_name, context->isdark);
        }
    }
}

void QuickPersonalizationManager::onCursorCommit(personalization_cursor_context_v1 *context)
{
    if (!context)
        return;

    if (m_cursorConfig == nullptr || !m_cursorConfig->isValid()) {
        context->verfity(false);
        return;
    }

    if (context->size > 0)
        setCursorSize(QSize(context->size, context->size));

    if (!context->theme.isEmpty())
        setCursorTheme(context->theme);

    context->verfity(true);
}

void QuickPersonalizationManager::onGetCursorTheme(personalization_cursor_context_v1 *context)
{
    if (!context)
        return;

    if (m_cursorConfig == nullptr || !m_cursorConfig->isValid())
        return;

    context->set_theme(cursorTheme());
}

void QuickPersonalizationManager::onGetCursorSize(personalization_cursor_context_v1 *context)
{
    if (!context)
        return;

    if (m_cursorConfig == nullptr)
        return;

    context->set_size(cursorSize().width());
}

void QuickPersonalizationManager::onGetWallpapers(personalization_wallpaper_context_v1 *context)
{
    if (!context)
        return;

    QDir dir(m_cacheDirectory);
    if (!dir.exists())
        return;

    context->set_meta_data(m_iniMetaData);
}

void QuickPersonalizationManager::onBackgroundTypeChanged(
    personalization_window_context_v1 *context)
{
    Q_EMIT backgroundTypeChanged(WSurface::fromHandle(context->surface), context->background_type);
}

uid_t QuickPersonalizationManager::userId()
{
    return m_userId;
}

void QuickPersonalizationManager::setUserId(uid_t uid)
{
    m_userId = uid;
    updateCacheWallpaperPath(uid);
    Q_EMIT userIdChanged(uid);
}

QString QuickPersonalizationManager::cursorTheme()
{
    QString value = m_cursorConfig->value("CursorThemeName").toString();
    return value;
}

void QuickPersonalizationManager::setCursorTheme(const QString &name)
{
    m_cursorConfig->setValue("CursorThemeName", name);

    Q_EMIT cursorThemeChanged(name);
}

QSize QuickPersonalizationManager::cursorSize()
{
    int size = m_cursorConfig->value("CursorSize").toInt();
    return QSize(size, size);
}

void QuickPersonalizationManager::setCursorSize(const QSize &size)
{
    m_cursorConfig->setValue("CursorSize", size.width());

    Q_EMIT cursorSizeChanged(size);
}

QString QuickPersonalizationManager::background(WOutput *w_output)
{
    return readWallpaperSettings("background", w_output);
}

QString QuickPersonalizationManager::lockscreen(WOutput *w_output)
{
    return readWallpaperSettings("lockscreen", w_output);
}

QuickPersonalizationManagerAttached::QuickPersonalizationManagerAttached(
    WSurface *target, QuickPersonalizationManager *manager)
    : QObject(manager)
    , m_target(target)
    , m_manager(manager)
{
    connect(m_manager,
            &QuickPersonalizationManager::backgroundTypeChanged,
            this,
            [this](WSurface *surface, uint32_t type) {
                if (m_target == surface) {
                    m_backgroundType = static_cast<BackgroundType>(type);
                    Q_EMIT backgroundTypeChanged();
                }
            });
}

QuickPersonalizationManagerAttached::QuickPersonalizationManagerAttached(
    QQuickItem *target, QuickPersonalizationManager *manager)
    : QObject(manager)
    , m_target(target)
    , m_manager(manager)
{
    if (PERSONALIZATION_IMAGE != nullptr) {
        qFatal("Must set image once!");
    }
    PERSONALIZATION_IMAGE = target;
}

QQuickItem *QuickPersonalizationManagerAttached::backgroundImage() const
{
    return PERSONALIZATION_IMAGE;
}

QuickPersonalizationManagerAttached *QuickPersonalizationManager::qmlAttachedProperties(
    QObject *target)
{
    if (auto *surface = qobject_cast<WXdgSurface *>(target)) {
        return new QuickPersonalizationManagerAttached(surface->surface(), PERSONALIZATION_MANAGER);
    }

    if (auto *image = qobject_cast<QQuickItem *>(target)) {
        return new QuickPersonalizationManagerAttached(image, PERSONALIZATION_MANAGER);
    }
    return nullptr;
}
