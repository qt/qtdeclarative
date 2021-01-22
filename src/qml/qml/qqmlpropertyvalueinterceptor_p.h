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

#ifndef QQMLPROPERTYVALUEINTERCEPTOR_P_H
#define QQMLPROPERTYVALUEINTERCEPTOR_P_H

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

#include <private/qtqmlglobal_p.h>
#include <private/qqmlpropertyindex_p.h>
#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE

class QQmlProperty;
class Q_QML_PRIVATE_EXPORT QQmlPropertyValueInterceptor
{
public:
    QQmlPropertyValueInterceptor();
    virtual ~QQmlPropertyValueInterceptor();
    virtual void setTarget(const QQmlProperty &property) = 0;
    virtual void write(const QVariant &value) = 0;

private:
    friend class QQmlInterceptorMetaObject;

    QQmlPropertyIndex m_propertyIndex;
    QQmlPropertyValueInterceptor *m_next;
};

#define QQmlPropertyValueInterceptor_iid "org.qt-project.Qt.QQmlPropertyValueInterceptor"

Q_DECLARE_INTERFACE(QQmlPropertyValueInterceptor, QQmlPropertyValueInterceptor_iid)

QT_END_NAMESPACE

#endif // QQMLPROPERTYVALUEINTERCEPTOR_P_H
