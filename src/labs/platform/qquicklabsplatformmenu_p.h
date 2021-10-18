/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#ifndef QQUICKLABSPLATFORMMENU_P_H
#define QQUICKLABSPLATFORMMENU_P_H

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

#include <QtCore/qobject.h>
#include <QtCore/qurl.h>
#include <QtGui/qfont.h>
#include <QtGui/qpa/qplatformmenu.h>
#include <QtQml/qqmlparserstatus.h>
#include <QtQml/qqmllist.h>
#include <QtQml/qqml.h>

#include "qquicklabsplatformicon_p.h"

QT_BEGIN_NAMESPACE

class QIcon;
class QWindow;
class QQuickItem;
class QPlatformMenu;
class QQmlV4Function;
class QQuickLabsPlatformMenuBar;
class QQuickLabsPlatformMenuItem;
class QQuickLabsPlatformIconLoader;
class QQuickLabsPlatformSystemTrayIcon;

class QQuickLabsPlatformMenu : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(QQmlListProperty<QObject> data READ data FINAL)
    Q_PROPERTY(QQmlListProperty<QQuickLabsPlatformMenuItem> items READ items NOTIFY itemsChanged FINAL)
    Q_PROPERTY(QQuickLabsPlatformMenuBar *menuBar READ menuBar NOTIFY menuBarChanged FINAL)
    Q_PROPERTY(QQuickLabsPlatformMenu *parentMenu READ parentMenu NOTIFY parentMenuChanged FINAL)
    Q_PROPERTY(QQuickLabsPlatformSystemTrayIcon *systemTrayIcon READ systemTrayIcon NOTIFY systemTrayIconChanged FINAL)
    Q_PROPERTY(QQuickLabsPlatformMenuItem *menuItem READ menuItem CONSTANT FINAL)
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged FINAL)
    Q_PROPERTY(bool visible READ isVisible WRITE setVisible NOTIFY visibleChanged FINAL)
    Q_PROPERTY(int minimumWidth READ minimumWidth WRITE setMinimumWidth NOTIFY minimumWidthChanged FINAL)
    Q_PROPERTY(QPlatformMenu::MenuType type READ type WRITE setType NOTIFY typeChanged FINAL)
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged FINAL)
    Q_PROPERTY(QFont font READ font WRITE setFont NOTIFY fontChanged FINAL)
    Q_PROPERTY(QQuickLabsPlatformIcon icon READ icon WRITE setIcon NOTIFY iconChanged FINAL REVISION(1, 1))
    Q_ENUMS(QPlatformMenu::MenuType)
    Q_CLASSINFO("DefaultProperty", "data")

public:
    explicit QQuickLabsPlatformMenu(QObject *parent = nullptr);
    ~QQuickLabsPlatformMenu();

    QPlatformMenu *handle() const;
    QPlatformMenu *create();
    void destroy();
    void sync();

    QQmlListProperty<QObject> data();
    QQmlListProperty<QQuickLabsPlatformMenuItem> items();

    QQuickLabsPlatformMenuBar *menuBar() const;
    void setMenuBar(QQuickLabsPlatformMenuBar *menuBar);

    QQuickLabsPlatformMenu *parentMenu() const;
    void setParentMenu(QQuickLabsPlatformMenu *menu);

    QQuickLabsPlatformSystemTrayIcon *systemTrayIcon() const;
    void setSystemTrayIcon(QQuickLabsPlatformSystemTrayIcon *icon);

    QQuickLabsPlatformMenuItem *menuItem() const;

    bool isEnabled() const;
    void setEnabled(bool enabled);

    bool isVisible() const;
    void setVisible(bool visible);

    int minimumWidth() const;
    void setMinimumWidth(int width);

    QPlatformMenu::MenuType type() const;
    void setType(QPlatformMenu::MenuType type);

    QString title() const;
    void setTitle(const QString &title);

    QFont font() const;
    void setFont(const QFont &font);

    QQuickLabsPlatformIcon icon() const;
    void setIcon(const QQuickLabsPlatformIcon &icon);

    Q_INVOKABLE void addItem(QQuickLabsPlatformMenuItem *item);
    Q_INVOKABLE void insertItem(int index, QQuickLabsPlatformMenuItem *item);
    Q_INVOKABLE void removeItem(QQuickLabsPlatformMenuItem *item);

    Q_INVOKABLE void addMenu(QQuickLabsPlatformMenu *menu);
    Q_INVOKABLE void insertMenu(int index, QQuickLabsPlatformMenu *menu);
    Q_INVOKABLE void removeMenu(QQuickLabsPlatformMenu *menu);

    Q_INVOKABLE void clear();

public Q_SLOTS:
    void open(QQmlV4Function *args);
    void close();

Q_SIGNALS:
    void aboutToShow();
    void aboutToHide();

    void itemsChanged();
    void menuBarChanged();
    void parentMenuChanged();
    void systemTrayIconChanged();
    void titleChanged();
    void enabledChanged();
    void visibleChanged();
    void minimumWidthChanged();
    void fontChanged();
    void typeChanged();
    Q_REVISION(2, 1) void iconChanged();

protected:
    void classBegin() override;
    void componentComplete() override;

    QQuickLabsPlatformIconLoader *iconLoader() const;

    QWindow *findWindow(QQuickItem *target, QPoint *offset) const;

    static void data_append(QQmlListProperty<QObject> *property, QObject *object);
    static qsizetype data_count(QQmlListProperty<QObject> *property);
    static QObject *data_at(QQmlListProperty<QObject> *property, qsizetype index);
    static void data_clear(QQmlListProperty<QObject> *property);

    static void items_append(QQmlListProperty<QQuickLabsPlatformMenuItem> *property, QQuickLabsPlatformMenuItem *item);
    static qsizetype items_count(QQmlListProperty<QQuickLabsPlatformMenuItem> *property);
    static QQuickLabsPlatformMenuItem *items_at(QQmlListProperty<QQuickLabsPlatformMenuItem> *property, qsizetype index);
    static void items_clear(QQmlListProperty<QQuickLabsPlatformMenuItem> *property);

private Q_SLOTS:
    void updateIcon();

private:
    void unparentSubmenus();

    bool m_complete;
    bool m_enabled;
    bool m_visible;
    int m_minimumWidth;
    QPlatformMenu::MenuType m_type;
    QString m_title;
    QFont m_font;
    QList<QObject *> m_data;
    QList<QQuickLabsPlatformMenuItem *> m_items;
    QQuickLabsPlatformMenuBar *m_menuBar;
    QQuickLabsPlatformMenu *m_parentMenu;
    QQuickLabsPlatformSystemTrayIcon *m_systemTrayIcon;
    mutable QQuickLabsPlatformMenuItem *m_menuItem;
    mutable QQuickLabsPlatformIconLoader *m_iconLoader;
    QPlatformMenu *m_handle;
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickLabsPlatformMenu)
Q_DECLARE_METATYPE(QPlatformMenu::MenuType)

#endif // QQUICKLABSPLATFORMMENU_P_H
