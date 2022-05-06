/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

#ifndef QQMLGUARDEDCONTEXTDATA_P_H
#define QQMLGUARDEDCONTEXTDATA_P_H

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

#include <QtQml/private/qqmlglobal_p.h>
#include <QtQml/private/qqmlcontextdata_p.h>

QT_BEGIN_NAMESPACE

class QQmlGuardedContextData
{
    Q_DISABLE_COPY(QQmlGuardedContextData);
public:
    QQmlGuardedContextData() = default;
    ~QQmlGuardedContextData() { unlink(); }

    QQmlGuardedContextData(QQmlGuardedContextData &&) = default;
    QQmlGuardedContextData &operator=(QQmlGuardedContextData &&) = default;

    QQmlGuardedContextData(QQmlRefPointer<QQmlContextData> data)
    {
        setContextData(std::move(data));
    }

    QQmlGuardedContextData &operator=(QQmlRefPointer<QQmlContextData> d)
    {
        setContextData(std::move(d));
        return *this;
    }

    QQmlRefPointer<QQmlContextData> contextData() const { return m_contextData; }
    void setContextData(QQmlRefPointer<QQmlContextData> contextData)
    {
        if (m_contextData.data() == contextData.data())
            return;
        unlink();

        if (contextData) {
            m_contextData = std::move(contextData);
            m_next = m_contextData->m_contextGuards;
            if (m_next)
                m_next->m_prev = &m_next;

            m_contextData->m_contextGuards = this;
            m_prev = &m_contextData->m_contextGuards;
        }
    }

    bool isNull() const { return !m_contextData; }

    operator const QQmlRefPointer<QQmlContextData> &() const { return m_contextData; }
    QQmlContextData &operator*() const { return m_contextData.operator*(); }
    QQmlContextData *operator->() const { return m_contextData.operator->(); }

    QQmlGuardedContextData *next() const { return m_next; }

    void reset()
    {
        m_contextData = nullptr;
        m_next = nullptr;
        m_prev = nullptr;
    }

private:
    void unlink()
    {
        if (m_prev) {
            *m_prev = m_next;
            if (m_next)
                m_next->m_prev = m_prev;
            reset();
        }
    }

    QQmlRefPointer<QQmlContextData> m_contextData;
    QQmlGuardedContextData  *m_next = nullptr;
    QQmlGuardedContextData **m_prev = nullptr;
};

QT_END_NAMESPACE

#endif // QQMLGUARDEDCONTEXTDATA_P_H
