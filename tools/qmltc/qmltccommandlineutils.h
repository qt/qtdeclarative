// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#ifndef QMLTCCOMMANDLINEUTILS_H
#define QMLTCCOMMANDLINEUTILS_H

#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

QString parseUrlArgument(const QString &arg);
QString loadUrl(const QString &url);
QString getImplicitImportDirectory(const QString &url);

QT_END_NAMESPACE

#endif // QMLTCCOMMANDLINEUTILS_H
