// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#ifndef QQMLJSTYPEREADER_P_H
#define QQMLJSTYPEREADER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include <QtQml/private/qqmljsastfwd_p.h>
#include <QtQml/private/qqmljsdiagnosticmessage_p.h>

#include <QtCore/qset.h>
#include <QtCore/qsharedpointer.h>

QT_BEGIN_NAMESPACE

class QQmlJSImporter;
class QQmlJSScope;

namespace QQmlJS {
using TypeReader = std::function<void(QQmlJSImporter *importer, const QString &filePath,
                                      const QSharedPointer<QQmlJSScope> &scopeToPopulate)>;
void defaultTypeReader(QQmlJSImporter *importer, const QString &filePath,
                       const QSharedPointer<QQmlJSScope> &scope);
} // namespace QQmlJS

QT_END_NAMESPACE

#endif // QQMLJSTYPEREADER_P_H
