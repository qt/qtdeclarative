// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qdochtmlparser_p.h>
#include <QtCore/qregularexpression.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

namespace {  //anonymous

// An emprical value to avoid too much content
static constexpr qsizetype firstIndexOfParagraphTag = 400;

// A paragraph can start with <p><i>, or <p><tt>
// We need smallest value to use QString::indexOf
static constexpr auto lengthOfSmallestOpeningTag = qsizetype(std::char_traits<char>::length("<p><i>"));
static constexpr auto lengthOfStartParagraphTag = qsizetype(std::char_traits<char>::length("<p>"));
static constexpr auto lengthOfEndParagraphTag = qsizetype(std::char_traits<char>::length("</p>"));
static constexpr auto lengthOfPeriod = qsizetype(std::char_traits<char>::length("."));

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

/*! \internal
    \brief Process the string obtained from start mark to end mark.
    This is duplicated from QtC's Utils::HtmlExtractor, modified on top of it.
*/
void processOutput(QString *html)
{
    Q_ASSERT(html);
    if (html->isEmpty())
        return;

    // Do not write the first paragraph in case it has extra tags below.
    // <p><i>This is only used on the Maemo platform.</i></p>
    // or: <p><tt>This is used on Windows only.</tt></p>
    // or: <p>[Conditional]</p>
    const auto skipFirstParagraphIfNeeded = [html](qsizetype &index){
        const bool shouldSkipFirstParagraph = html->indexOf(QLatin1String("<p><i>")) == index ||
                html->indexOf(QLatin1String("<p><tt>")) == index ||
                html->indexOf(QLatin1String("<p>[Conditional]</p>")) == index;

        if (shouldSkipFirstParagraph)
            index = html->indexOf(QLatin1String("<p>"), index + lengthOfSmallestOpeningTag);
    };

    // Try to get the entire first paragraph, but if one is not found or if its opening
    // tag is not in the very beginning (using an empirical value as the limit)
    // the html is cleared out to avoid too much content.
    qsizetype index = html->indexOf(QLatin1String("<p>"));
    if (index != -1 && index < firstIndexOfParagraphTag) {
        skipFirstParagraphIfNeeded(index);
        index = html->indexOf(QLatin1String("</p>"), index + lengthOfStartParagraphTag);
        if (index != -1) {
            // Most paragraphs end with a period, but there are cases without punctuation
            // and cases like this: <p>This is a description. Example:</p>
            const auto period = html->lastIndexOf(QLatin1Char('.'), index);
            if (period != -1) {
                html->truncate(period + lengthOfPeriod);
                html->append(QLatin1String("</p>"));
            } else {
                html->truncate(index + lengthOfEndParagraphTag);
            }
        } else {
            html->clear();
        }
    } else {
        html->clear();
    }
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
    // Qt 5.15 way of finding properties in doc
    QString startMark = QString::fromLatin1("<a name=\"%1-prop\">").arg(element);
    qsizetype startIndex = m_code.indexOf(startMark);
    if (startIndex == -1) {
        // if not found, try Qt6
        startMark = QString::fromLatin1(
                            "<td class=\"tblQmlPropNode\"><p>\n<span class=\"name\">%1</span>")
                            .arg(element);
        startIndex = m_code.indexOf(startMark);
        if (startIndex == -1)
            return {};
    }

    QString contents = m_code.mid(startIndex + startMark.size());
    startIndex = contents.indexOf(QLatin1String("<div class=\"qmldoc\"><p>"));
    if (startIndex == -1)
        return {};

    contents = contents.mid(startIndex);
    if (mode == ExtractionMode::Simplified)
        processOutput(&contents);
    return contents;
}

QT_END_NAMESPACE
