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

#ifndef QQMLPROXYMETAOBJECT_P_H
#define QQMLPROXYMETAOBJECT_P_H

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

#include <private/qmetaobjectbuilder_p.h>
#include "qqml.h"

#include <QtCore/QMetaObject>
#include <QtCore/QObject>

#include <private/qobject_p.h>

QT_BEGIN_NAMESPACE


class QQmlProxyMetaObject : public QDynamicMetaObjectData
{
public:
    enum ProxyType {
        ProxyIsExtension,
        ProxyIsObject,
    };
    struct ProxyData {
        typedef QObject *(*CreateFunc)(QObject *);
        QMetaObject *metaObject = nullptr;
        CreateFunc createFunc = nullptr; // function to create the proxy object

        // precalculated offsets of metaObject member
        int propertyOffset = 0;
        int methodOffset = 0;

        // cached offsets of metaObject's origin (metaObject is a clone of
        // origin). unlike offsets above, these might not be available later
        // during proxy calls since we lose the origin of a non-extension proxy
        int originPropertyOffset = 0;
        int originMethodOffset = 0;

        ProxyType type = ProxyIsExtension;
    };

    QQmlProxyMetaObject(QObject *, QList<ProxyData> *);
    ~QQmlProxyMetaObject();

    static constexpr int extensionObjectId(int id) noexcept
    {
        Q_ASSERT(id >= 0);
        Q_ASSERT(id <= MaxExtensionCount); // MaxExtensionCount is a valid index
        return ExtensionObjectId | id;
    }

protected:
    int metaCall(QObject *o, QMetaObject::Call _c, int _id, void **_a) override;
    QMetaObject *toDynamicMetaObject(QObject *) override;

private:
    QObject *getProxy(int index);

    QList<ProxyData> *metaObjects;
    QObject **proxies;

    QDynamicMetaObjectData *parent;
    QMetaObject *metaObject;
    QObject *object;

    // ExtensionObjectId acts as a flag for whether we should interpret a
    // QMetaObject::CustomCall as a call to fetch the extension object (see
    // QQmlProxyMetaObject::metaCall()). MaxExtensionCount is a limit on how
    // many extensions we can query via such mechanism
    enum : int {
        MaxExtensionCount = 127, // magic number so that low bits are all '1'
        ExtensionObjectId = ~MaxExtensionCount,
    };
};

QT_END_NAMESPACE

#endif // QQMLPROXYMETAOBJECT_P_H

