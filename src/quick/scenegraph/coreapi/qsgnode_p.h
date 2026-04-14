// Copyright (C) 2016 Klaralvdalens Datakonsult AB (KDAB)
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QSGNODE_P_H
#define QSGNODE_P_H

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

#include <private/qglobal_p.h>

#include "qsgnode.h"

QT_BEGIN_NAMESPACE

class Q_AUTOTEST_EXPORT QSGNodePrivate
{
public:
    QSGNodePrivate() : m_mutabilityGroup(0) {}
    virtual ~QSGNodePrivate() {}

    static quint8 mutabilityGroup(const QSGNode *node);
    static void setMutabilityGroupOfSubtree(QSGNode *node, quint8 mutabilityGroup);

#ifdef QSG_RUNTIME_DESCRIPTION
    static void setDescription(QSGNode *node, const QString &description) {
        node->d_ptr->descr= description;
    }
    static QString description(const QSGNode *node) {
        return node->d_ptr->descr;
    }
    QString descr;
#endif

    quint8 m_mutabilityGroup : 4;

    // Reserved: 4 bits
};

class QSGBasicGeometryNodePrivate : public QSGNodePrivate
{
public:
    QSGBasicGeometryNodePrivate() {}
};


class QSGGeometryNodePrivate: public QSGBasicGeometryNodePrivate
{
public:
    QSGGeometryNodePrivate() {}
};

QT_END_NAMESPACE

#endif // QSGNODE_P_H
