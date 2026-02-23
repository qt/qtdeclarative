// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default
#ifndef QQMLPREVIEWBINDINGPATCHCONTEXT_P_H
#define QQMLPREVIEWBINDINGPATCHCONTEXT_P_H

#include <private/qduplicatetracker_p.h>
#include <private/qqmlanybinding_p.h>
#include <private/qqmlcontextdata_p.h>
#include <private/qqmldata_p.h>
#include <private/qqmlpreviewdiff_p.h>
#include <private/qqmlvmemetaobject_p.h>
#include <private/qv4executablecompilationunit_p.h>

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

QT_BEGIN_NAMESPACE

namespace QQmlPreview {

class BindingPatchContext
{
public:
    BindingPatchContext(QObject *object) : m_object(object), m_ddata(QQmlData::get(object)) { }

    void reset();

private:
    void resetBinding(const QMetaObject *metaObject, const QV4::CompiledData::Binding *binding,
                      const QString &defaultPropName,
                      const QQmlRefPointer<QV4::ExecutableCompilationUnit> &oldUnit);
    void resetBindings(const QMetaObject *metaObject,
                       const QQmlRefPointer<QV4::ExecutableCompilationUnit> &oldUnit, int cuIndex);

    QObject *m_object = nullptr;
    QQmlData *m_ddata = nullptr;
    QDuplicateTracker<int> m_handledProperties;
};

} // namespace QQmlPreview

QT_END_NAMESPACE

#endif
