/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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
#ifndef QV4IDENTIFIERHASH_P_H
#define QV4IDENTIFIERHASH_P_H

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

#include <qstring.h>
#include <private/qv4global_p.h>
#include <private/qv4propertykey_p.h>

QT_BEGIN_NAMESPACE

namespace QV4 {

struct IdentifierHashEntry;
struct IdentifierHashData;
struct Q_QML_PRIVATE_EXPORT IdentifierHash
{
    IdentifierHash() = default;
    IdentifierHash(ExecutionEngine *engine);
    IdentifierHash(const IdentifierHash &other);
    ~IdentifierHash();
    IdentifierHash &operator=(const IdentifierHash &other);

    bool isEmpty() const { return !d; }

    int count() const;

    void detach();

    void add(const QString &str, int value);
    void add(Heap::String *str, int value);

    int value(const QString &str) const;
    int value(String *str) const;
    QString findId(int value) const;

private:
    inline IdentifierHashEntry *addEntry(PropertyKey i);
    inline const IdentifierHashEntry *lookup(PropertyKey identifier) const;
    inline const IdentifierHashEntry *lookup(const QString &str) const;
    inline const IdentifierHashEntry *lookup(String *str) const;
    inline const PropertyKey toIdentifier(const QString &str) const;
    inline const PropertyKey toIdentifier(Heap::String *str) const;

    IdentifierHashData *d = nullptr;
};

} // namespace QV4

QT_END_NAMESPACE

#endif // QV4_IDENTIFIERHASH_P_H
