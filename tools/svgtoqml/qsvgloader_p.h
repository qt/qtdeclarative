// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QSVGQMLWRITER_P_H
#define QSVGQMLWRITER_P_H

#include <QtCore/qtconfigmacros.h>
#include <QtCore/qflags.h>

QT_BEGIN_NAMESPACE

class QTextStream;
class QSvgTinyDocument;
class QString;
class QQuickItem;

class QSvgQmlWriter
{
public:
    enum GeneratorFlag {
        OptimizePaths = 0x01,
        CurveRenderer = 0x02,
        OutlineStrokeMode = 0x04
    };
    Q_DECLARE_FLAGS(GeneratorFlags, GeneratorFlag);
    static QQuickItem *loadSVG(const QSvgTinyDocument *doc, const QString &outFileName, GeneratorFlags flags, const QString &typeName, QQuickItem *parentItem, const QString &commentString);
};
Q_DECLARE_OPERATORS_FOR_FLAGS(QSvgQmlWriter::GeneratorFlags);

QT_END_NAMESPACE

#endif // QSVGQMLWRITER_P_H
