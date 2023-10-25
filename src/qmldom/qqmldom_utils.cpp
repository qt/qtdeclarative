// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmldom_utils_p.h"
#include <QtCore/qdir.h>
#include <QtCore/qdiriterator.h>
#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(QQmlJSDomImporting, "qt.qqmljsdom.importing")

namespace QQmlJS {
namespace Dom {

using namespace Qt::StringLiterals;

QStringList resourceFilesFromBuildFolders(const QStringList &buildFolders)
{
    QStringList result;
    for (const QString &path : buildFolders) {
        QDir dir(path);
        if (!dir.cd(u".rcc"_s))
            continue;

        QDirIterator it(dir.canonicalPath(), QStringList{ u"*.qrc"_s }, QDir::Files,
                        QDirIterator::Subdirectories);
        while (it.hasNext()) {
            result.append(it.next());
        }
    }
    return result;
}

} // namespace Dom
}; // namespace QQmlJS

QT_END_NAMESPACE
