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

#ifndef QQMLPARSERSTATUS_H
#define QQMLPARSERSTATUS_H

#include <QtQml/qtqmlglobal.h>
#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE


class Q_QML_EXPORT QQmlParserStatus
{
public:
    QQmlParserStatus();
    virtual ~QQmlParserStatus();

    virtual void classBegin()=0;
    virtual void componentComplete()=0;

private:
    friend class QQmlComponent;
    friend class QQmlComponentPrivate;
    friend class QQmlEnginePrivate;
    friend class QQmlObjectCreator;
    QQmlParserStatus **d;
};

#define QQmlParserStatus_iid "org.qt-project.Qt.QQmlParserStatus"
Q_DECLARE_INTERFACE(QQmlParserStatus, QQmlParserStatus_iid)

QT_END_NAMESPACE

#endif // QQMLPARSERSTATUS_H
