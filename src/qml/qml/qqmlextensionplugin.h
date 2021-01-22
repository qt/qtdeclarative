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

#ifndef QQMLEXTENSIONPLUGIN_H
#define QQMLEXTENSIONPLUGIN_H

#include <QtCore/qplugin.h>
#include <QtCore/QUrl>
#include <QtQml/qqmlextensioninterface.h>

#if defined(Q_CC_GHS)
#  define GHS_PRAGMA(S) _Pragma(#S)
#  define GHS_KEEP_REFERENCE(S) GHS_PRAGMA(ghs reference S ##__Fv)
#else
#  define GHS_KEEP_REFERENCE(S)
#endif

QT_BEGIN_NAMESPACE

class QQmlEngine;
class QQmlExtensionPluginPrivate;

class Q_QML_EXPORT QQmlExtensionPlugin
    : public QObject
    , public QQmlExtensionInterface
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQmlExtensionPlugin)
    Q_INTERFACES(QQmlExtensionInterface)
    Q_INTERFACES(QQmlTypesExtensionInterface)
public:
    explicit QQmlExtensionPlugin(QObject *parent = nullptr);
    ~QQmlExtensionPlugin() override;

    QUrl baseUrl() const;

    void registerTypes(const char *uri) override = 0;
    void initializeEngine(QQmlEngine *engine, const char *uri) override;

private:
    Q_DISABLE_COPY(QQmlExtensionPlugin)
};

class Q_QML_EXPORT QQmlEngineExtensionPlugin
        : public QObject
        , public QQmlEngineExtensionInterface
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(QQmlEngineExtensionPlugin)
    Q_INTERFACES(QQmlEngineExtensionInterface)
public:
    explicit QQmlEngineExtensionPlugin(QObject *parent = nullptr);
    ~QQmlEngineExtensionPlugin() override;
    void initializeEngine(QQmlEngine *engine, const char *uri) override;
};

QT_END_NAMESPACE

#endif // QQMLEXTENSIONPLUGIN_H
