// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QDOCHTMLEXTRACTOR_P_H
#define QDOCHTMLEXTRACTOR_P_H

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

#include <QString>

QT_BEGIN_NAMESPACE

class QDocHtmlExtractor
{
public:
    enum class ExtractionMode : char { Simplified, Extended };
    enum class ElementType : char {
        QmlType,
        QmlProperty,
        QmlMethod,
        QmlSignal
    };

    struct Element {
        QString name;
        ElementType type;
    };

    QDocHtmlExtractor(const QString &code);
    QString extract(const Element &element, ExtractionMode extractionMode);

private:
    QString parseForQmlType(const QString &element, ExtractionMode mode);
    QString parseForQmlProperty(const QString &element, ExtractionMode mode = ExtractionMode::Simplified);
    QString parseForQmlMethodOrSignal(const QString &functionName, ExtractionMode mode);

    const QString &m_code;
};

QT_END_NAMESPACE

#endif // QDOCHTMLEXTRACTOR_P_H
