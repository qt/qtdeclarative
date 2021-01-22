/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
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

#ifndef QQMLABSTRACTDELEGATECOMPONENT_P_H
#define QQMLABSTRACTDELEGATECOMPONENT_P_H

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

#include <private/qtqmlmodelsglobal_p.h>
#include <private/qqmlcomponentattached_p.h>
#include <qqmlcomponent.h>

QT_REQUIRE_CONFIG(qml_delegate_model);

QT_BEGIN_NAMESPACE

// TODO: consider making QQmlAbstractDelegateComponent public API
class QQmlAdaptorModel;
class Q_QMLMODELS_PRIVATE_EXPORT QQmlAbstractDelegateComponent : public QQmlComponent
{
    Q_OBJECT
    QML_NAMED_ELEMENT(AbstractDelegateComponent)
    QML_UNCREATABLE("Cannot create instance of abstract class AbstractDelegateComponent.")

public:
    QQmlAbstractDelegateComponent(QObject *parent = nullptr);
    ~QQmlAbstractDelegateComponent() override;

    virtual QQmlComponent *delegate(QQmlAdaptorModel *adaptorModel, int row, int column = 0) const = 0;

signals:
    void delegateChanged();

protected:
    QVariant value(QQmlAdaptorModel *adaptorModel,int row, int column, const QString &role) const;
};

QT_END_NAMESPACE

#endif // QQMLABSTRACTDELEGATECOMPONENT_P_H
