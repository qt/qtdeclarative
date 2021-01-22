/****************************************************************************
**
** Copyright (C) 2016 BlackBerry Limited. All rights reserved.
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

#ifndef QQMLFILESELECTOR_H
#define QQMLFILESELECTOR_H

#include <QtCore/QObject>
#include <QtCore/QUrl>
#include <QtQml/QQmlEngine>
#include <QtQml/qtqmlglobal.h>

QT_BEGIN_NAMESPACE

class QFileSelector;
class QQmlFileSelectorPrivate;
class Q_QML_EXPORT QQmlFileSelector : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQmlFileSelector)
public:
    explicit QQmlFileSelector(QQmlEngine *engine, QObject *parent = nullptr);
    ~QQmlFileSelector() override;
    QFileSelector *selector() const Q_DECL_NOTHROW;
    void setSelector(QFileSelector *selector);
    void setExtraSelectors(QStringList &strings); // TODO Qt6: remove
    void setExtraSelectors(const QStringList &strings);
    static QQmlFileSelector* get(QQmlEngine*);

private:
    Q_DISABLE_COPY(QQmlFileSelector)
};

QT_END_NAMESPACE

#endif
