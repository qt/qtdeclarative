// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#ifndef QQMLTCCOMMANDLINEUTILS_P_H
#define QQMLTCCOMMANDLINEUTILS_P_H

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

#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

namespace QQmltc {

QString parseUrlArgument(const QString &arg);
QString loadUrl(const QString &url);
QString getImplicitImportDirectory(const QString &url);

} // namespace QQmltc

QT_END_NAMESPACE

#endif // QQMLTCCOMMANDLINEUTILS_P_H
