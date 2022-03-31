/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
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

#ifndef QMINIMALFLATSET_P_H
#define QMINIMALFLATSET_P_H

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

#include <QtQuick/qtquickglobal.h>

#include <QtCore/qcontainerfwd.h>
#include <QtCore/private/qglobal_p.h>

//#define QMINIMAL_FLAT_SET_DEBUG
#ifdef QMINIMAL_FLAT_SET_DEBUG
# include <QtCore/qscopeguard.h>
# include <QtCore/qdebug.h>
# define QMINIMAL_FLAT_SET_PRINT_AT_END \
    const auto sg = qScopeGuard([&] { qDebug() << this << *this; });
#else
# define QMINIMAL_FLAT_SET_PRINT_AT_END
#endif

#include <algorithm> // for std::lower_bound

QT_BEGIN_NAMESPACE

/*
    This is a minimal version of a QFlatSet, the std::set version of QFlatMap.
    Like QFlatMap, it has linear insertion and removal, not logarithmic, like
    real QMap and std::set, so it's only a good container if you either have
    very few entries or lots, but with separate setup and lookup stages.
    Because a full QFlatSet would be 10x the work on writing this minimal one,
    we keep it here for now. When more users pop up and the class has matured a
    bit, we can consider moving it alongside QFlatMap in QtCore.
*/

template <typename T, typename Container = QList<T>>
class QMinimalFlatSet
{
    Container c;
public:
    // compiler-generated default ctor is ok!
    // compiler-generated copy/move ctor/assignment operators are ok!
    // compiler-generated dtor is ok!

    using const_iterator = typename Container::const_iterator;
    using iterator = const_iterator;
    using const_reverse_iterator = typename Container::const_reverse_iterator;
    using reverse_iterator = const_reverse_iterator;
    using value_type = T;

    iterator begin() const { return c.cbegin(); }
    iterator end() const { return c.cend(); }
    iterator cbegin() const { return begin(); }
    iterator cend() const { return cend(); }

    reverse_iterator rbegin() const { return c.crbegin(); }
    reverse_iterator rend() const { return c.crend(); }
    reverse_iterator crbegin() const { return rbegin(); }
    reverse_iterator crend() const { return rend(); }

    void clear() {
        QMINIMAL_FLAT_SET_PRINT_AT_END
        c.clear();
    }
    auto size() const { return c.size(); }
    auto count() const { return size(); }
    bool isEmpty() const { return size() == 0; }

    std::pair<iterator, bool> insert(value_type &&v)
    {
        QMINIMAL_FLAT_SET_PRINT_AT_END
        const auto r = lookup(v);
        if (r.exists)
            return {r.it, false};
        else
            return {c.insert(r.it, std::move(v)), true};
    }

    std::pair<iterator, bool> insert(const value_type &v)
    {
        QMINIMAL_FLAT_SET_PRINT_AT_END
        const auto r = lookup(v);
        if (r.exists)
            return {r.it, false};
        else
            return {c.insert(r.it, v), true};
    }

    void erase(const value_type &v)
    {
        QMINIMAL_FLAT_SET_PRINT_AT_END
        const auto r = lookup(v);
        if (r.exists)
            c.erase(r.it);
    }
    void remove(const value_type &v) { erase(v); }

    bool contains(const value_type &v) const
    {
        return lookup(v).exists;
    }

    const Container &values() const & { return c; }
    Container values() && { return std::move(c); }

private:
    auto lookup(const value_type &v) const
    {
        struct R {
            iterator it;
            bool exists;
        };

        const auto it = std::lower_bound(c.cbegin(), c.cend(), v);
        return R{it, it != c.cend() && !(v < *it)};
    }

#ifdef QMINIMAL_FLAT_SET_DEBUG
    friend QDebug operator<<(QDebug dbg, const QMinimalFlatSet &set)
    {
        const QDebugStateSaver saver(dbg);
        dbg.nospace() << "QMinimalFlatSet{";
        for (auto &e : set)
            dbg << e << ", ";
        return dbg << "}";
    }
#endif
};

QT_END_NAMESPACE

#endif // QMINIMALFLATSET_P_H
