/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

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

class Q_QUICK_PRIVATE_EXPORT QQuickWindowQmlImpl : public QQuickWindow, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

    Q_PROPERTY(bool visible READ isVisible WRITE setVisible NOTIFY visibleChanged)
    Q_PROPERTY(Visibility visibility READ visibility WRITE setVisibility NOTIFY visibilityChanged)
    Q_PROPERTY(QObject *screen READ screen WRITE setScreen NOTIFY screenChanged REVISION 3)
    QML_ATTACHED(QQuickWindowAttached)

public:
    QQuickWindowQmlImpl(QWindow *parent = nullptr);

    void setVisible(bool visible);
    void setVisibility(Visibility visibility);

    QObject *screen() const;
    void setScreen(QObject *screen);

    static QQuickWindowAttached *qmlAttachedProperties(QObject *object);

Q_SIGNALS:
    void visibleChanged(bool arg);
    void visibilityChanged(QWindow::Visibility visibility);
    Q_REVISION(3) void screenChanged();

protected:
    void classBegin() override;
    void componentComplete() override;

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
