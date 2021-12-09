/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QMLLINT_P_H
#define QMLLINT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include <QtQmlCompiler/private/qqmljslogger_p.h>
#include <QtQmlCompiler/private/qqmljsimporter_p.h>

#include <QtCore/qjsonarray.h>
#include <QtCore/qstring.h>
#include <QtCore/qmap.h>
#include <QtCore/qscopedpointer.h>

QT_BEGIN_NAMESPACE

class QQmlLinter
{
public:
    QQmlLinter(const QStringList &importPaths, bool useAbsolutePath = false);

    bool lintFile(const QString &filename, const QString *fileContents, const bool silent,
                  QJsonArray *json, const QStringList &qmlImportPaths,
                  const QStringList &qmldirFiles, const QStringList &resourceFiles,
                  const QMap<QString, QQmlJSLogger::Option> &options);

    const QQmlJSLogger *logger() const { return m_logger.get(); }

private:
    bool m_useAbsolutePath;
    QQmlJSImporter m_importer;
    QScopedPointer<QQmlJSLogger> m_logger;
};

QT_END_NAMESPACE

#endif // QMLLINT_P_H
