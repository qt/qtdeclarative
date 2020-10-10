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
**/
#include "qqmldomexternalitems_p.h"

#include "qqmldomtop_p.h"

#include <QtQml/private/qqmljslexer_p.h>
#include <QtQml/private/qqmljsparser_p.h>
#include <QtQml/private/qqmljsengine_p.h>
#include <QtQml/private/qqmljsastvisitor_p.h>
#include <QtQml/private/qqmljsast_p.h>
#include <QtCore/QDir>
#include <QtCore/QScopeGuard>
#include <QtCore/QFileInfo>

#include <algorithm>

QT_BEGIN_NAMESPACE

namespace QQmlJS {
namespace Dom {

ExternalOwningItem::ExternalOwningItem(QString filePath, QDateTime lastDataUpdateAt, Path path, int derivedFrom):
    OwningItem(derivedFrom, lastDataUpdateAt), m_canonicalFilePath(filePath), m_path(path)
{}

ExternalOwningItem::ExternalOwningItem(const ExternalOwningItem &o):
    OwningItem(o), m_canonicalFilePath(o.m_canonicalFilePath),
  m_path(o.m_path), m_isValid(o.m_isValid)
{}

QString ExternalOwningItem::canonicalFilePath(const DomItem &) const
{
    return m_canonicalFilePath;
}

QString ExternalOwningItem::canonicalFilePath() const
{
    return m_canonicalFilePath;
}

Path ExternalOwningItem::canonicalPath(const DomItem &) const
{
    return m_path;
}

Path ExternalOwningItem::canonicalPath() const
{
    return m_path;
}

} // end namespace Dom
} // end namespace QQmlJS

QT_END_NAMESPACE
