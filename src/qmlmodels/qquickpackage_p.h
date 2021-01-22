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

#ifndef QQUICKPACKAGE_H
#define QQUICKPACKAGE_H

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

#include <qqml.h>
#include <QtQmlModels/private/qtqmlmodelsglobal_p.h>

QT_REQUIRE_CONFIG(qml_delegate_model);

QT_BEGIN_NAMESPACE

class QQuickPackagePrivate;
class QQuickPackageAttached;
class Q_AUTOTEST_EXPORT QQuickPackage : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuickPackage)

    Q_CLASSINFO("DefaultProperty", "data")
    QML_NAMED_ELEMENT(Package)
    QML_ADDED_IN_MINOR_VERSION(14)
    QML_ATTACHED(QQuickPackageAttached)
    Q_PROPERTY(QQmlListProperty<QObject> data READ data)

public:
    QQuickPackage(QObject *parent=nullptr);
    virtual ~QQuickPackage();

    QQmlListProperty<QObject> data();

    QObject *part(const QString & = QString());
    bool hasPart(const QString &);

    static QQuickPackageAttached *qmlAttachedProperties(QObject *);
};

class QQuickPackageAttached : public QObject
{
Q_OBJECT
Q_PROPERTY(QString name READ name WRITE setName)
public:
    QQuickPackageAttached(QObject *parent);
    virtual ~QQuickPackageAttached();

    QString name() const;
    void setName(const QString &n);

    static QHash<QObject *, QQuickPackageAttached *> attached;
private:
    QString _name;
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickPackage)

#endif // QQUICKPACKAGE_H
