/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
