// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qdochtmlparser_p.h>
#include <QtCore/qregularexpression.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

namespace {  //anonymous

QString getContentsByMarks(const QString &html, QString startMark, QString endMark)
{
    startMark.prepend("$$$"_L1);
    endMark.prepend("<!-- @@@"_L1);

    QString contents;
    qsizetype start = html.indexOf(startMark);
    if (start != -1) {
        start = html.indexOf("-->"_L1, start);
        if (start != -1) {
            qsizetype end = html.indexOf(endMark, start);
            if (end != -1) {
                start += qsizetype(std::char_traits<char>::length("-->"));
                contents = html.mid(start, end - start);
            }
        }
    }
    return contents;
}


void stripAllHtml(QString *html)
{
    Q_ASSERT(html);
    html->remove(QRegularExpression("<.*?>"_L1));
}

}

QDocHtmlExtractor::QDocHtmlExtractor(const QString &code) : m_code{ code }
{
}

QString QDocHtmlExtractor::extract(const QString &elementName, ElementType type, ExtractionMode mode)
{
    if (elementName.isEmpty())
        return {};

    QString result;
    switch (type) {
    case ElementType::QmlType:
        result = parseForQmlType(elementName, mode);
        break;

    case ElementType::QmlProperty:
        result = parseForQmlProperty(elementName, mode);
        break;

    default:
        return {};
    }

    stripAllHtml(&result);

    // Also remove leading and trailing whitespaces
    return result.trimmed();
}

QString QDocHtmlExtractor::parseForQmlType(const QString &element, ExtractionMode mode)
{
    QString result;
    // Get brief description
    if (mode == QDocHtmlExtractor::ExtractionMode::Simplified) {
        result = getContentsByMarks(m_code, element + "-brief"_L1 , element);
        // Remove More...
        if (!result.isEmpty()) {
            const auto tailToRemove = "More..."_L1;
            const auto lastIndex = result.lastIndexOf(tailToRemove);
            if (lastIndex != -1)
                result.remove(lastIndex, tailToRemove.length());
        }
    } else {
        result = getContentsByMarks(m_code, element + "-description"_L1, element);
        // Remove header
        if (!result.isEmpty()) {
            const auto headerToRemove = "Detailed Description"_L1;
            const auto firstIndex = result.indexOf(headerToRemove);
            if (firstIndex != -1)
                result.remove(firstIndex, headerToRemove.length());
        }
    }

    return result;
}

QString QDocHtmlExtractor::parseForQmlProperty(const QString &element, ExtractionMode mode)
{
    Q_UNUSED(element);
    Q_UNUSED(mode);
    Q_UNIMPLEMENTED();
    return {};
}

QT_END_NAMESPACE
