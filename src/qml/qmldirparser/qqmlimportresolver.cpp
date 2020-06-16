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

#include "qqmlimportresolver_p.h"

QT_BEGIN_NAMESPACE

enum ImportVersion { FullyVersioned, PartiallyVersioned, Unversioned };

/*!
    Forms complete paths to a module, from a list of base paths,
    a module URI and version specification.

    For example, QtQml.Models 2.0:
    - base/QtQml/Models.2.0
    - base/QtQml.2.0/Models
    - base/QtQml/Models.2
    - base/QtQml.2/Models
    - base/QtQml/Models
*/
QStringList qQmlResolveImportPaths(QStringView uri, const QStringList &basePaths,
                                   QTypeRevision version)
{
    static const QLatin1Char Slash('/');
    static const QLatin1Char Backslash('\\');

    const QVector<QStringView> parts = uri.split(u'.', Qt::SkipEmptyParts);

    QStringList importPaths;
    // fully & partially versioned parts + 1 unversioned for each base path
    importPaths.reserve(2 * parts.count() + 1);

    auto versionString = [](QTypeRevision version, ImportVersion mode)
    {
        if (mode == FullyVersioned) {
            // extension with fully encoded version number (eg. MyModule.3.2)
            return QString::fromLatin1(".%1.%2").arg(version.majorVersion())
                    .arg(version.minorVersion());
        }
        if (mode == PartiallyVersioned) {
            // extension with encoded version major (eg. MyModule.3)
            return QString::fromLatin1(".%1").arg(version.majorVersion());
        }
        // else extension without version number (eg. MyModule)
        return QString();
    };

    auto joinStringRefs = [](const QVector<QStringView> &refs, const QChar &sep) {
        QString str;
        for (auto it = refs.cbegin(); it != refs.cend(); ++it) {
            if (it != refs.cbegin())
                str += sep;
            str += *it;
        }
        return str;
    };

    const ImportVersion initial = (version.hasMinorVersion())
            ? FullyVersioned
            : (version.hasMajorVersion() ? PartiallyVersioned : Unversioned);
    for (int mode = initial; mode <= Unversioned; ++mode) {
        const QString ver = versionString(version, ImportVersion(mode));

        for (const QString &path : basePaths) {
            QString dir = path;
            if (!dir.endsWith(Slash) && !dir.endsWith(Backslash))
                dir += Slash;

            // append to the end
            importPaths += dir + joinStringRefs(parts, Slash) + ver;

            if (mode != Unversioned) {
                // insert in the middle
                for (int index = parts.count() - 2; index >= 0; --index) {
                    importPaths += dir + joinStringRefs(parts.mid(0, index + 1), Slash)
                            + ver + Slash
                            + joinStringRefs(parts.mid(index + 1), Slash);
                }
            }
        }
    }

    return importPaths;
}

QT_END_NAMESPACE
