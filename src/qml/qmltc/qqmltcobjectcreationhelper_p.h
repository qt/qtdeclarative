/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#ifndef QQMLTCOBJECTCREATIONHELPER_P_H
#define QQMLTCOBJECTCREATIONHELPER_P_H

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

#include <array>

QT_BEGIN_NAMESPACE

/*!
    \internal

    (Kind of) type-erased  object creation utility that can be used throughout
    the generated C++ code. By nature it shows relative data to the current QML
    document and allows to get and set object pointers.
 */
class QQmltcObjectCreationHelper
{
    QObject **m_data = nullptr; // QObject* array
    [[maybe_unused]] const qsizetype m_size = 0; // size of m_data array, exists for bounds checking
    const qsizetype m_offset = 0; // global offset into m_data array
    const qsizetype m_nonRoot = 1; // addresses the "+ 1" in QQmltcObjectCreationBase::m_objects

    qsizetype offset() const { return m_offset + m_nonRoot; }

public:
    /*!
        Constructs initial "view" from basic data. Supposed to only be called
        once from QQmltcObjectCreationBase.
    */
    QQmltcObjectCreationHelper(QObject **data, qsizetype size)
        : m_data(data), m_size(size), m_nonRoot(0 /* root object */)
    {
    }

    /*!
        Constructs new "view" from \a base view, adding \a localOffset to the
        offset of that base.
    */
    QQmltcObjectCreationHelper(const QQmltcObjectCreationHelper *base, qsizetype localOffset)
        : m_data(base->m_data), m_size(base->m_size), m_offset(base->m_offset + localOffset)
    {
        Q_ASSERT(m_nonRoot == 1); // sanity check - a sub-creator is for non-root object
    }

    template<typename T>
    T *get(qsizetype i) const
    {
        Q_ASSERT(m_data);
        Q_ASSERT(i >= 0 && i + offset() < m_size);
        Q_ASSERT(qobject_cast<T *>(m_data[i + offset()]) != nullptr);
        // Note: perform cheap cast as we know *exactly* the real type of the
        // object
        return static_cast<T *>(m_data[i + offset()]);
    }

    void set(qsizetype i, QObject *object)
    {
        Q_ASSERT(m_data);
        Q_ASSERT(i >= 0 && i + offset() < m_size);
        Q_ASSERT(m_data[i + offset()] == nullptr); // prevent accidental resets
        m_data[i + offset()] = object;
    }
};

/*!
    \internal

    Base helper for qmltc-generated types that linearly stores pointers to all
    the to-be-created objects for fast access during object creation.
 */
template<typename QmltcGeneratedType>
class QQmltcObjectCreationBase
{
    // Note: +1 for the document root itself
    std::array<QObject *, QmltcGeneratedType::q_qmltc_typeCount + 1> m_objects = {};

public:
    QQmltcObjectCreationHelper view()
    {
        return QQmltcObjectCreationHelper(m_objects.data(), m_objects.size());
    }
};

QT_END_NAMESPACE

#endif // QQMLTCOBJECTCREATIONHELPER_P_H
