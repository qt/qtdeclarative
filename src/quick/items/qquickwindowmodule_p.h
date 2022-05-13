// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKWINDOWMODULE_H
#define QQUICKWINDOWMODULE_H

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

#include <private/qtquickglobal_p.h>
#include <qquickwindow.h>
#include <qqmlparserstatus.h>
#include <private/qquickwindowattached_p.h>

QT_BEGIN_NAMESPACE

class QQuickWindowQmlImplPrivate;

struct QWindowForeign
{
    Q_GADGET
    QML_FOREIGN(QWindow)
    QML_ANONYMOUS
    QML_ADDED_IN_VERSION(2, 1)
};

class Q_QUICK_PRIVATE_EXPORT QQuickWindowQmlImpl : public QQuickWindow, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

    Q_PROPERTY(bool visible READ isVisible WRITE setVisible NOTIFY visibleChanged)
    Q_PROPERTY(QWindow::Visibility visibility READ visibility WRITE setVisibility NOTIFY
                       visibilityChanged)
    Q_PROPERTY(QObject *screen READ screen WRITE setScreen NOTIFY screenChanged REVISION(2, 3))
    QML_ATTACHED(QQuickWindowAttached)
    QML_NAMED_ELEMENT(Window)
    QML_ADDED_IN_VERSION(2, 1)

public:
    QQuickWindowQmlImpl(QWindow *parent = nullptr);

    void setVisible(bool visible);
    void setVisibility(QWindow::Visibility visibility);

    QObject *screen() const;
    void setScreen(QObject *screen);

    static QQuickWindowAttached *qmlAttachedProperties(QObject *object);

Q_SIGNALS:
    void visibleChanged(bool arg);
    void visibilityChanged(QWindow::Visibility visibility);
    Q_REVISION(2, 3) void screenChanged();

protected:
    void classBegin() override;
    void componentComplete() override;

    QQuickWindowQmlImpl(QQuickWindowQmlImplPrivate &dd, QWindow *parent);

private Q_SLOTS:
    void setWindowVisibility();

private:
    bool transientParentVisible();

private:
    Q_DISABLE_COPY(QQuickWindowQmlImpl)
    Q_DECLARE_PRIVATE(QQuickWindowQmlImpl)
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickWindowQmlImpl)

#endif
