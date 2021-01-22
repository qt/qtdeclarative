/****************************************************************************
**
** Copyright (C) 2016 Research In Motion.
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

#ifndef QQMLAPPLICATIONENGINE_H
#define QQMLAPPLICATIONENGINE_H

#include <QtQml/qqmlengine.h>

#include <QtCore/qurl.h>
#include <QtCore/qobject.h>
#include <QtCore/qlist.h>

QT_BEGIN_NAMESPACE

class QQmlApplicationEnginePrivate;
class Q_QML_EXPORT QQmlApplicationEngine : public QQmlEngine
{
    Q_OBJECT
public:
    QQmlApplicationEngine(QObject *parent = nullptr);
    QQmlApplicationEngine(const QUrl &url, QObject *parent = nullptr);
    QQmlApplicationEngine(const QString &filePath, QObject *parent = nullptr);
    ~QQmlApplicationEngine() override;

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QList<QObject*> rootObjects(); // ### Qt 6: remove
#endif
    QList<QObject*> rootObjects() const;

public Q_SLOTS:
    void load(const QUrl &url);
    void load(const QString &filePath);
    void setInitialProperties(const QVariantMap &initialProperties);
    void loadData(const QByteArray &data, const QUrl &url = QUrl());

Q_SIGNALS:
    void objectCreated(QObject *object, const QUrl &url);

private:
    Q_DISABLE_COPY(QQmlApplicationEngine)
    Q_PRIVATE_SLOT(d_func(), void _q_loadTranslations())
    Q_DECLARE_PRIVATE(QQmlApplicationEngine)
};

QT_END_NAMESPACE

#endif
