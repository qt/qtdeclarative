// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLCOMPONENTATTACHED_P_H
#define QQMLCOMPONENTATTACHED_P_H

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

#include <QtQml/qqml.h>
#include <private/qtqmlglobal_p.h>
#include <QtCore/QObject>

QT_BEGIN_NAMESPACE


// implemented in qqmlcomponent.cpp
class Q_QML_PRIVATE_EXPORT QQmlComponentAttached : public QObject
{
    Q_OBJECT

    // Used as attached object for QQmlComponent. We want qqmlcomponentattached_p.h to be #include'd
    // when registering QQmlComponent, but we cannot #include it from qqmlcomponent.h. Therefore we
    // force an anonymous type registration here.
    QML_ANONYMOUS
    QML_ADDED_IN_VERSION(2, 0)
    Q_CLASSINFO("QML.OmitFromQmlTypes", "true")
public:
    QQmlComponentAttached(QObject *parent = nullptr);
    ~QQmlComponentAttached();

    void insertIntoList(QQmlComponentAttached **listHead)
    {
        m_prev = listHead;
        m_next = *listHead;
        *listHead = this;
        if (m_next)
            m_next->m_prev = &m_next;
    }

    void removeFromList()
    {
        *m_prev = m_next;
        if (m_next)
            m_next->m_prev = m_prev;
        m_next = nullptr;
        m_prev = nullptr;
    }

    QQmlComponentAttached *next() const { return m_next; }

Q_SIGNALS:
    void completed();
    void destruction();

private:
    QQmlComponentAttached **m_prev;
    QQmlComponentAttached *m_next;
};

QT_END_NAMESPACE

#endif // QQMLCOMPONENTATTACHED_P_H
