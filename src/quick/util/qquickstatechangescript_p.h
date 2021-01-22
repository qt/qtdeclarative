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

#ifndef QQUICKSTATEOPERATIONS_H
#define QQUICKSTATEOPERATIONS_H

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

#include "qquickstate_p.h"
#include <qqmlscriptstring.h>

QT_BEGIN_NAMESPACE

class QQuickStateChangeScriptPrivate;
class Q_AUTOTEST_EXPORT QQuickStateChangeScript : public QQuickStateOperation, public QQuickStateActionEvent
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuickStateChangeScript)

    Q_PROPERTY(QQmlScriptString script READ script WRITE setScript)
    Q_PROPERTY(QString name READ name WRITE setName)
    QML_NAMED_ELEMENT(StateChangeScript)

public:
    QQuickStateChangeScript(QObject *parent=nullptr);
    ~QQuickStateChangeScript();

    ActionList actions() override;

    EventType type() const override;

    QQmlScriptString script() const;
    void setScript(const QQmlScriptString &);

    QString name() const;
    void setName(const QString &);

    void execute() override;
};


QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickStateChangeScript)

#endif // QQUICKSTATEOPERATIONS_H
