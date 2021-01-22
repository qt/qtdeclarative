/****************************************************************************
**
** Copyright (C) 2016 Jolla Ltd, author: <gunnar.sletta@jollamobile.com>
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

#ifndef QQUICKITEMGRABRESULT_H
#define QQUICKITEMGRABRESULT_H

#include <QtCore/QObject>
#include <QtCore/QSize>
#include <QtCore/QUrl>
#include <QtGui/QImage>
#include <QtQml/QJSValue>
#include <QtQml/qqml.h>
#include <QtQuick/qtquickglobal.h>

QT_BEGIN_NAMESPACE

class QImage;

class QQuickItemGrabResultPrivate;

class Q_QUICK_EXPORT QQuickItemGrabResult : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuickItemGrabResult)

    Q_PROPERTY(QImage image READ image CONSTANT)
    Q_PROPERTY(QUrl url READ url CONSTANT)
    QML_ANONYMOUS

public:
    QImage image() const;
    QUrl url() const;

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#if QT_DEPRECATED_SINCE(5, 15)
    QT_DEPRECATED_X("This overload is deprecated. Use the const member function instead")
    Q_INVOKABLE bool saveToFile(const QString &fileName);
#endif
#endif
    Q_INVOKABLE bool saveToFile(const QString &fileName) const;

protected:
    bool event(QEvent *) override;

Q_SIGNALS:
    void ready();

private Q_SLOTS:
    void setup();
    void render();

private:
    friend class QQuickItem;

    QQuickItemGrabResult(QObject *parent = nullptr);
};

QT_END_NAMESPACE

#endif
