/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Labs Platform module of the Qt Toolkit.
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
******************************************************************************/

#ifndef QQMLSTANDARDPATHS_P_H
#define QQMLSTANDARDPATHS_P_H

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
#include <QtCore/qstandardpaths.h>
#include <QtCore/qurl.h>
#include <QtQml/qqml.h>
#include <QtQmlCore/private/qqmlcoreglobal_p.h>

QT_BEGIN_NAMESPACE

class QQmlEngine;
class QJSEngine;

class Q_QMLCORE_PRIVATE_EXPORT QQmlStandardPaths : public QObject
{
    Q_OBJECT
    QML_SINGLETON
    QML_NAMED_ELEMENT(StandardPaths)
    QML_ADDED_IN_VERSION(6, 2)
    QML_EXTENDED_NAMESPACE(QStandardPaths)

public:
    explicit QQmlStandardPaths(QObject *parent = nullptr);

    Q_INVOKABLE QString displayName(QStandardPaths::StandardLocation type) const;
    Q_INVOKABLE QUrl findExecutable(const QString &executableName, const QStringList &paths = QStringList()) const;
    Q_INVOKABLE QUrl locate(QStandardPaths::StandardLocation type, const QString &fileName,
        QStandardPaths::LocateOptions options = QStandardPaths::LocateFile) const;
    Q_INVOKABLE QList<QUrl> locateAll(QStandardPaths::StandardLocation type, const QString &fileName,
        QStandardPaths::LocateOptions options = QStandardPaths::LocateFile) const;
    Q_INVOKABLE QList<QUrl> standardLocations(QStandardPaths::StandardLocation type) const;
    Q_INVOKABLE QUrl writableLocation(QStandardPaths::StandardLocation type) const;
};

QT_END_NAMESPACE

#endif // QQMLSTANDARDPATHS_P_H
