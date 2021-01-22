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

#ifndef QQMLPLATFORM_P_H
#define QQMLPLATFORM_P_H

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
#include <qqml.h>
#include <private/qtqmlglobal_p.h>

QT_BEGIN_NAMESPACE

class Q_QML_PRIVATE_EXPORT QQmlPlatform : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString os READ os CONSTANT)
    Q_PROPERTY(QString pluginName READ pluginName CONSTANT)

public:
    explicit QQmlPlatform(QObject *parent = nullptr);
    virtual ~QQmlPlatform();

    static QString os();
    QString pluginName() const;

private:
    Q_DISABLE_COPY(QQmlPlatform)
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQmlPlatform)

#endif // QQMLPLATFORM_P_H
