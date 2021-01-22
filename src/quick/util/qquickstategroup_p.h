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

#ifndef QQUICKSTATEGROUP_H
#define QQUICKSTATEGROUP_H

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

QT_BEGIN_NAMESPACE

class QQuickStateGroupPrivate;
class Q_QUICK_PRIVATE_EXPORT QQuickStateGroup : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    Q_DECLARE_PRIVATE(QQuickStateGroup)

    Q_PROPERTY(QString state READ state WRITE setState NOTIFY stateChanged)
    Q_PROPERTY(QQmlListProperty<QQuickState> states READ statesProperty DESIGNABLE false)
    Q_PROPERTY(QQmlListProperty<QQuickTransition> transitions READ transitionsProperty DESIGNABLE false)
    QML_NAMED_ELEMENT(StateGroup)

public:
    QQuickStateGroup(QObject * = nullptr);
    virtual ~QQuickStateGroup();

    QString state() const;
    void setState(const QString &);

    QQmlListProperty<QQuickState> statesProperty();
    QList<QQuickState *> states() const;

    QQmlListProperty<QQuickTransition> transitionsProperty();

    QQuickState *findState(const QString &name) const;
    void removeState(QQuickState *state);

    void classBegin() override;
    void componentComplete() override;
Q_SIGNALS:
    void stateChanged(const QString &);

private:
    friend class QQuickState;
    friend class QQuickStatePrivate;
    bool updateAutoState();
    void stateAboutToComplete();
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickStateGroup)

#endif // QQUICKSTATEGROUP_H
