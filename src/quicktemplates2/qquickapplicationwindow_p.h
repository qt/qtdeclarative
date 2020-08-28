/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Templates 2 module of the Qt Toolkit.
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

#ifndef QQUICKAPPLICATIONWINDOW_P_H
#define QQUICKAPPLICATIONWINDOW_P_H

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

#include <QtQuick/private/qquickwindowmodule_p.h>
#include <QtQuickTemplates2/private/qtquicktemplates2global_p.h>
#include <QtGui/qfont.h>
#include <QtGui/qpalette.h>
#include <QtCore/qlocale.h>

QT_BEGIN_NAMESPACE

class QQuickApplicationWindowPrivate;
class QQuickApplicationWindowAttached;
class QQuickApplicationWindowAttachedPrivate;

class Q_QUICKTEMPLATES2_PRIVATE_EXPORT QQuickApplicationWindow : public QQuickWindowQmlImpl
{
    Q_OBJECT
    Q_PROPERTY(QQuickItem *background READ background WRITE setBackground NOTIFY backgroundChanged FINAL)
    Q_PROPERTY(QQuickItem *contentItem READ contentItem CONSTANT FINAL)
    Q_PRIVATE_PROPERTY(QQuickApplicationWindow::d_func(), QQmlListProperty<QObject> contentData READ contentData FINAL)
    Q_PROPERTY(QQuickItem *activeFocusControl READ activeFocusControl NOTIFY activeFocusControlChanged FINAL)
    Q_PROPERTY(QQuickItem *header READ header WRITE setHeader NOTIFY headerChanged FINAL)
    Q_PROPERTY(QQuickItem *footer READ footer WRITE setFooter NOTIFY footerChanged FINAL)
    Q_PROPERTY(QFont font READ font WRITE setFont RESET resetFont NOTIFY fontChanged FINAL)
    Q_PROPERTY(QLocale locale READ locale WRITE setLocale RESET resetLocale NOTIFY localeChanged FINAL)
    // 2.3 (Qt 5.10)
    Q_PROPERTY(QQuickItem *menuBar READ menuBar WRITE setMenuBar NOTIFY menuBarChanged FINAL REVISION(2, 3))
    // 2.14 (Qt 6)
    Q_PRIVATE_PROPERTY(QQuickApplicationWindow::d_func(), QQuickPalette *palette READ palette WRITE setPalette RESET resetPalette NOTIFY paletteChanged REVISION(2, 3))
    Q_CLASSINFO("DeferredPropertyNames", "background")
    Q_CLASSINFO("DefaultProperty", "contentData")
    QML_NAMED_ELEMENT(ApplicationWindow)
    QML_ADDED_IN_VERSION(2, 0)

public:
    explicit QQuickApplicationWindow(QWindow *parent = nullptr);
    ~QQuickApplicationWindow();

    static QQuickApplicationWindowAttached *qmlAttachedProperties(QObject *object);

    QQuickItem *background() const;
    void setBackground(QQuickItem *background);

    QQuickItem *contentItem() const;

    QQuickItem *activeFocusControl() const;

    QQuickItem *header() const;
    void setHeader(QQuickItem *header);

    QQuickItem *footer() const;
    void setFooter(QQuickItem *footer);

    QFont font() const;
    void setFont(const QFont &font);
    void resetFont();

    QLocale locale() const;
    void setLocale(const QLocale &locale);
    void resetLocale();

    QQuickItem *menuBar() const;
    void setMenuBar(QQuickItem *menuBar);

Q_SIGNALS:
    void backgroundChanged();
    void activeFocusControlChanged();
    void headerChanged();
    void footerChanged();
    void fontChanged();
    void localeChanged();
    Q_REVISION(2, 3) void menuBarChanged();

protected:
    bool isComponentComplete() const;
    void classBegin() override;
    void componentComplete() override;
    void resizeEvent(QResizeEvent *event) override;

private:
    Q_DISABLE_COPY(QQuickApplicationWindow)
    Q_DECLARE_PRIVATE(QQuickApplicationWindow)
    Q_PRIVATE_SLOT(d_func(), void _q_updateActiveFocus())
};

class Q_QUICKTEMPLATES2_PRIVATE_EXPORT QQuickApplicationWindowAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQuickApplicationWindow *window READ window NOTIFY windowChanged FINAL)
    Q_PROPERTY(QQuickItem *contentItem READ contentItem NOTIFY contentItemChanged FINAL)
    Q_PROPERTY(QQuickItem *activeFocusControl READ activeFocusControl NOTIFY activeFocusControlChanged FINAL)
    Q_PROPERTY(QQuickItem *header READ header NOTIFY headerChanged FINAL)
    Q_PROPERTY(QQuickItem *footer READ footer NOTIFY footerChanged FINAL)
    Q_PROPERTY(QQuickItem *menuBar READ menuBar NOTIFY menuBarChanged FINAL) // REVISION(2, 3)

public:
    explicit QQuickApplicationWindowAttached(QObject *parent = nullptr);

    QQuickApplicationWindow *window() const;
    QQuickItem *contentItem() const;
    QQuickItem *activeFocusControl() const;
    QQuickItem *header() const;
    QQuickItem *footer() const;
    QQuickItem *menuBar() const;

Q_SIGNALS:
    void windowChanged();
    void contentItemChanged();
    void activeFocusControlChanged();
    void headerChanged();
    void footerChanged();
    // 2.3 (Qt 5.10)
    /*Q_REVISION(2, 3)*/ void menuBarChanged();

private:
    Q_DISABLE_COPY(QQuickApplicationWindowAttached)
    Q_DECLARE_PRIVATE(QQuickApplicationWindowAttached)
};

struct QWindowForeign2
{
    Q_GADGET
    QML_ANONYMOUS
    QML_FOREIGN(QWindow)
    QML_ADDED_IN_VERSION(2, 0)
};

struct QQuickWindowForeign
{
    Q_GADGET
    QML_ANONYMOUS
    QML_FOREIGN(QQuickWindow)
    QML_ADDED_IN_VERSION(2, 0)
};

struct QQuickWindowQmlImplForeign
{
    Q_GADGET
    QML_ANONYMOUS
    QML_FOREIGN(QQuickWindowQmlImpl)
    QML_ADDED_IN_VERSION(2, 2)
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickApplicationWindow)
QML_DECLARE_TYPEINFO(QQuickApplicationWindow, QML_HAS_ATTACHED_PROPERTIES)

#endif // QQUICKAPPLICATIONWINDOW_P_H
