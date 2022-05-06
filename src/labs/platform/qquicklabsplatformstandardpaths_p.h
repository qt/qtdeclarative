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

#ifndef QQUICKLABSPLATFORMSTANDARDPATHS_P_H
#define QQUICKLABSPLATFORMSTANDARDPATHS_P_H

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

QT_BEGIN_NAMESPACE

class QQmlEngine;
class QJSEngine;

class QQuickLabsPlatformStandardPaths : public QObject
{
    Q_OBJECT
    Q_ENUMS(QStandardPaths::StandardLocation QStandardPaths::LocateOptions)

public:
    explicit QQuickLabsPlatformStandardPaths(QObject *parent = nullptr);

    static QObject *create(QQmlEngine *engine, QJSEngine *scriptEngine);

    Q_INVOKABLE static QString displayName(QStandardPaths::StandardLocation type);
    Q_INVOKABLE static QUrl findExecutable(const QString &executableName, const QStringList &paths = QStringList());
    Q_INVOKABLE static QUrl locate(QStandardPaths::StandardLocation type, const QString &fileName, QStandardPaths::LocateOptions options = QStandardPaths::LocateFile);
    Q_INVOKABLE static QList<QUrl> locateAll(QStandardPaths::StandardLocation type, const QString &fileName, QStandardPaths::LocateOptions options = QStandardPaths::LocateFile);
    Q_INVOKABLE static void setTestModeEnabled(bool testMode);
    Q_INVOKABLE static QList<QUrl> standardLocations(QStandardPaths::StandardLocation type);
    Q_INVOKABLE static QUrl writableLocation(QStandardPaths::StandardLocation type);

private:
    Q_DISABLE_COPY(QQuickLabsPlatformStandardPaths)
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickLabsPlatformStandardPaths)
Q_DECLARE_METATYPE(QStandardPaths::StandardLocation)
Q_DECLARE_METATYPE(QStandardPaths::LocateOptions)

#endif // QQUICKLABSPLATFORMSTANDARDPATHS_P_H
