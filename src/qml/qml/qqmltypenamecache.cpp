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

#include "qqmltypenamecache_p.h"

#include "qqmlengine_p.h"

QT_BEGIN_NAMESPACE

QQmlTypeNameCache::QQmlTypeNameCache(const QQmlImports &importCache)
    : m_imports(importCache)
{
}

QQmlTypeNameCache::~QQmlTypeNameCache()
{
}

void QQmlTypeNameCache::add(const QHashedString &name, const QUrl &url, const QHashedString &nameSpace)
{
    if (nameSpace.length() != 0) {
        QQmlImportRef *i = m_namedImports.value(nameSpace);
        Q_ASSERT(i != nullptr);
        i->compositeSingletons.insert(name, url);
        return;
    }

    if (m_anonymousCompositeSingletons.contains(name))
        return;

    m_anonymousCompositeSingletons.insert(name, url);
}

void QQmlTypeNameCache::add(const QHashedString &name, int importedScriptIndex, const QHashedString &nameSpace)
{
    QQmlImportRef import;
    import.scriptIndex = importedScriptIndex;
    import.m_qualifier = name;

    if (nameSpace.length() != 0) {
        QQmlImportRef *i = m_namedImports.value(nameSpace);
        Q_ASSERT(i != nullptr);
        m_namespacedImports[i].insert(name, import);
        return;
    }

    if (m_namedImports.contains(name))
        return;

    m_namedImports.insert(name, import);
}

QT_END_NAMESPACE

