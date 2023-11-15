// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QSVGQMLWRITER_P_H
#define QSVGQMLWRITER_P_H

#include <QtCore/qtconfigmacros.h>

QT_BEGIN_NAMESPACE

class QTextStream;
class QSvgTinyDocument;
class QString;
class QQuickItem;

class QSvgQmlWriter
{
public:
    static QQuickItem *loadSVG(const QSvgTinyDocument *doc, const QString &outFileName, const QString &typeName, QQuickItem *parentItem = nullptr);
};

QT_END_NAMESPACE

#endif // QSVGQMLWRITER_P_H
