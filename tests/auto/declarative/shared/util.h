/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QDECLARATIVETESTUTILS_H
#define QDECLARATIVETESTUTILS_H

#include <QtCore/qdir.h>

namespace QDeclarativeTestUtils
{
    /*
        Returns the path to some testdata file.

        We first check relative to the binary, and then look in the source tree.

        Note we are looking for a _directory_ which exists, but the _file_ itself need not exist,
        to support the case of finding a path to a testdata file which doesn't exist yet (i.e.
        a file we are about to create).
    */
    QString testdata(QString const& name, const char *sourceFile)
    {
        // Try to find it relative to the binary.
        QFileInfo relative = QDir(QCoreApplication::applicationDirPath()).filePath(QLatin1String("data/") + name);
        if (relative.dir().exists()) {
            return relative.absoluteFilePath();
        }

        // Else try to find it in the source tree
        QFileInfo from_source = QFileInfo(sourceFile).absoluteDir().filePath(QLatin1String("data/") + name);
        if (from_source.dir().exists()) {
            return from_source.absoluteFilePath();
        }

        qWarning("requested testdata %s could not be found (looked at: %s, %s)",
                 qPrintable(name),
                 qPrintable(relative.filePath()),
                 qPrintable(from_source.filePath())
                 );

        return QString();
    }
}

#define TESTDATA(name) QDeclarativeTestUtils::testdata(name, __FILE__)

#endif // QDECLARATIVETESTUTILS_H
