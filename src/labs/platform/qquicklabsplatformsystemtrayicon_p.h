/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Labs Platform module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQUICKLABSPLATFORMSYSTEMTRAYICON_P_H
#define QQUICKLABSPLATFORMSYSTEMTRAYICON_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qurl.h>
#include <QtCore/qrect.h>
#include <QtGui/qpa/qplatformsystemtrayicon.h>
#include <QtQml/qqmlparserstatus.h>
#include <QtQml/qqml.h>

#include "qquicklabsplatformicon_p.h"

QT_REQUIRE_CONFIG(systemtrayicon);

QT_BEGIN_NAMESPACE

class QQuickLabsPlatformMenu;
class QQuickLabsPlatformIconLoader;

class QQuickLabsPlatformSystemTrayIcon : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(bool available READ isAvailable CONSTANT FINAL)
    Q_PROPERTY(bool supportsMessages READ supportsMessages CONSTANT FINAL)
    Q_PROPERTY(bool visible READ isVisible WRITE setVisible NOTIFY visibleChanged FINAL)
    Q_PROPERTY(QString tooltip READ tooltip WRITE setTooltip NOTIFY tooltipChanged FINAL)
    Q_PROPERTY(QQuickLabsPlatformMenu *menu READ menu WRITE setMenu NOTIFY menuChanged FINAL)
    Q_PROPERTY(QRect geometry READ geometry NOTIFY geometryChanged FINAL REVISION(1, 1))
    Q_PROPERTY(QQuickLabsPlatformIcon icon READ icon WRITE setIcon NOTIFY iconChanged FINAL REVISION(1, 1))
    Q_ENUMS(QPlatformSystemTrayIcon::ActivationReason QPlatformSystemTrayIcon::MessageIcon)

public:
    explicit QQuickLabsPlatformSystemTrayIcon(QObject *parent = nullptr);
    ~QQuickLabsPlatformSystemTrayIcon();

    QPlatformSystemTrayIcon *handle() const;

    bool isAvailable() const;
    bool supportsMessages() const;

    bool isVisible() const;
    void setVisible(bool visible);

    QString tooltip() const;
    void setTooltip(const QString &tooltip);

    QQuickLabsPlatformMenu *menu() const;
    void setMenu(QQuickLabsPlatformMenu *menu);

    QRect geometry() const;

    QQuickLabsPlatformIcon icon() const;
    void setIcon(const QQuickLabsPlatformIcon &icon);

public Q_SLOTS:
    void show();
    void hide();

    void showMessage(const QString &title, const QString &message,
                     QPlatformSystemTrayIcon::MessageIcon iconType = QPlatformSystemTrayIcon::Information, int msecs = 10000);

Q_SIGNALS:
    void activated(QPlatformSystemTrayIcon::ActivationReason reason);
    void messageClicked();
    void visibleChanged();
    void tooltipChanged();
    void menuChanged();
    Q_REVISION(2, 1) void geometryChanged();
    Q_REVISION(2, 1) void iconChanged();

protected:
    void init();
    void cleanup();

    void classBegin() override;
    void componentComplete() override;

    QQuickLabsPlatformIconLoader *iconLoader() const;

private Q_SLOTS:
    void updateIcon();

private:
    bool m_complete;
    bool m_visible;
    QString m_tooltip;
    QQuickLabsPlatformMenu *m_menu;
    mutable QQuickLabsPlatformIconLoader *m_iconLoader;
    QPlatformSystemTrayIcon *m_handle;
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickLabsPlatformSystemTrayIcon)
Q_DECLARE_METATYPE(QPlatformSystemTrayIcon::ActivationReason)
Q_DECLARE_METATYPE(QPlatformSystemTrayIcon::MessageIcon)

#endif // QQUICKLABSPLATFORMSYSTEMTRAYICON_P_H
