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

#ifndef QQUICKAPPLICATION_P_H
#define QQUICKAPPLICATION_P_H

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

#include <QtCore/QObject>
#include <QtGui/QFont>
#include <qqml.h>
#include <QtQml/private/qqmlglobal_p.h>
#include <private/qtquickglobal_p.h>
#include "../items/qquickscreen_p.h"

QT_BEGIN_NAMESPACE

class Q_AUTOTEST_EXPORT QQuickApplication : public QQmlApplication
{
    Q_OBJECT
    Q_PROPERTY(bool active READ active NOTIFY activeChanged) // deprecated, use 'state' instead
    Q_PROPERTY(Qt::LayoutDirection layoutDirection READ layoutDirection NOTIFY layoutDirectionChanged)
    Q_PROPERTY(bool supportsMultipleWindows READ supportsMultipleWindows CONSTANT)
    Q_PROPERTY(Qt::ApplicationState state READ state NOTIFY stateChanged)
    Q_PROPERTY(QFont font READ font CONSTANT)
    Q_PROPERTY(QString displayName READ displayName WRITE setDisplayName NOTIFY displayNameChanged)
    Q_PROPERTY(QQmlListProperty<QQuickScreenInfo> screens READ screens NOTIFY screensChanged)

    QML_NAMED_ELEMENT(Application)
    QML_UNCREATABLE("Application is an abstract class.")

public:
    explicit QQuickApplication(QObject *parent = nullptr);
    virtual ~QQuickApplication();
    bool active() const;
    Qt::LayoutDirection layoutDirection() const;
    bool supportsMultipleWindows() const;
    Qt::ApplicationState state() const;
    QFont font() const;
    QQmlListProperty<QQuickScreenInfo> screens();
    QString displayName() const;
    void setDisplayName(const QString &displayName);

Q_SIGNALS:
    void activeChanged();
    void displayNameChanged();
    void layoutDirectionChanged();
    void stateChanged(Qt::ApplicationState state);
    void screensChanged();

private Q_SLOTS:
    void updateScreens();

private:
    Q_DISABLE_COPY(QQuickApplication)
    QVector<QQuickScreenInfo *> m_screens;
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickApplication)

#endif // QQUICKAPPLICATION_P_H
