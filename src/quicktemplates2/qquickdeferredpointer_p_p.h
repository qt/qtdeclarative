/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Templates 2 module of the Qt Toolkit.
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

#ifndef QQUICKDEFERREDPOINTER_P_P_H
#define QQUICKDEFERREDPOINTER_P_P_H

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

#include <QtCore/qglobal.h>
#include <QtQml/private/qbipointer_p.h>
#include <QtQml/private/qqmlcomponent_p.h>

QT_BEGIN_NAMESPACE

class QQuickUntypedDeferredPointer
{
    Q_DISABLE_COPY_MOVE(QQuickUntypedDeferredPointer)
public:
    QQmlComponentPrivate::DeferredState *deferredState() const
    {
        return value.isT1() ? nullptr : &value.asT2()->state;
    }

    void clearDeferredState()
    {
        if (value.isT1())
            return;
        value.clearFlag();
        DeferredState *state = value.asT2();
        value = state->value;
        delete state;
    }

    bool wasExecuted() const { return value.isT1() && value.flag(); }
    void setExecuted()
    {
        Q_ASSERT(value.isT1());
        value.setFlag();
    }

    bool isExecuting() const { return value.isT2() && value.flag(); }
    bool setExecuting(bool b)
    {
        if (b) {
            if (value.isT2()) {
                // Not our state. Set the flag, but leave it alone.
                value.setFlag();
                return false;
            }

            value = new DeferredState {
                QQmlComponentPrivate::DeferredState(),
                value.asT1()
            };
            value.setFlag();
            return true;
        }

        if (value.isT2()) {
            value.clearFlag();
            return true;
        }

        return false;
    }

protected:
    QQuickUntypedDeferredPointer() = default;
    QQuickUntypedDeferredPointer(void *v) : value(v)
    {
        Q_ASSERT(value.isT1());
        Q_ASSERT(!value.flag());
    }

    QQuickUntypedDeferredPointer &operator=(void *v)
    {
        if (value.isT1())
            value = v;
        else
            value.asT2()->value = v;
        return *this;
    }

    ~QQuickUntypedDeferredPointer()
    {
        if (value.isT2())
            delete value.asT2();
    }

    void *data() const { return value.isT1() ? value.asT1() : value.asT2()->value; }

private:
    struct DeferredState
    {
        QQmlComponentPrivate::DeferredState state;
        void *value = nullptr;
    };

    QBiPointer<void, DeferredState> value;
};

template<typename T>
class QQuickDeferredPointer : public QQuickUntypedDeferredPointer
{
    Q_DISABLE_COPY_MOVE(QQuickDeferredPointer)
public:
    QQuickDeferredPointer() = default;
    ~QQuickDeferredPointer() = default;

    QQuickDeferredPointer(T *v) : QQuickUntypedDeferredPointer(v) {}
    QQuickDeferredPointer<T> &operator=(T *o) {
        QQuickUntypedDeferredPointer::operator=(o);
        return *this;
    }

    T *data() const { return static_cast<T *>(QQuickUntypedDeferredPointer::data()); }
    operator bool() const { return data() != nullptr; }
    operator T*() const { return data(); }
    T *operator*() const { return data(); }
    T *operator->() const { return data(); }
};

QT_END_NAMESPACE

#endif // QQUICKDEFERREDPOINTER_P_P_H
