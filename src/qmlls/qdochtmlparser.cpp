// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qdochtmlparser_p.h>
#include <QtCore/qregularexpression.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

// An emprical value to avoid too much content
static constexpr qsizetype firstIndexOfParagraphTag = 400;

// A paragraph can start with <p><i>, or <p><tt>
// We need smallest value to use QString::indexOf
static constexpr auto lengthOfStartParagraphTag = qsizetype(std::char_traits<char>::length("<p>"));
static constexpr auto lengthOfEndParagraphTag = qsizetype(std::char_traits<char>::length("</p>"));

static QString getContentsByMarks(const QString &html, QString startMark, QString endMark)
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


static void stripAllHtml(QString *html)
{
    Q_ASSERT(html);
    html->remove(QRegularExpression("<.*?>"_L1));
}

/*! \internal
    \brief Process the string obtained from start mark to end mark.
    This is duplicated from QtC's Utils::HtmlExtractor, modified on top of it.
*/
static void processOutput(QString *html)
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
                html->indexOf(QLatin1String("<p><b>")) == index ||
                html->indexOf(QLatin1String("<p>[Conditional]</p>")) == index;

        if (shouldSkipFirstParagraph)
            index = html->indexOf(QLatin1String("</p>"), index) + lengthOfEndParagraphTag;
    };

    // Try to get the entire first paragraph, but if one is not found or if its opening
    // tag is not in the very beginning (using an empirical value as the limit)
    // the html is cleared out to avoid too much content.
    qsizetype index = html->indexOf(QLatin1String("<p>"));
    if (index != -1 && index < firstIndexOfParagraphTag) {
        skipFirstParagraphIfNeeded(index);
        qsizetype endIndex = html->indexOf(QLatin1String("</p>"), index + lengthOfStartParagraphTag);
        if (endIndex != -1) {
            *html = html->mid(index, endIndex - index);
        } else {
            html->clear();
        }
    } else {
        html->clear();
    }
}

class ExtractQmlType : public HtmlExtractor
{
public:
    QString extract(const QString &code, const QString &keyword, ExtractionMode mode) override;
};

class ExtractQmlProperty : public HtmlExtractor
{
public:
    QString extract(const QString &code, const QString &keyword, ExtractionMode mode) override;
};

class ExtractQmlMethodOrSignal : public HtmlExtractor
{
public:
    QString extract(const QString &code, const QString &keyword, ExtractionMode mode) override;
};

QString ExtractQmlType::extract(const QString &code, const QString &element, ExtractionMode mode)
{
    QString result;
    // Get brief description
    if (mode == ExtractionMode::Simplified) {
        result = getContentsByMarks(code, element + "-brief"_L1 , element);
        // Remove More...
        if (!result.isEmpty()) {
            const auto tailToRemove = "More..."_L1;
            const auto lastIndex = result.lastIndexOf(tailToRemove);
            if (lastIndex != -1)
                result.remove(lastIndex, tailToRemove.length());
        }
    } else {
        result = getContentsByMarks(code, element + "-description"_L1, element);
        // Remove header
        if (!result.isEmpty()) {
            const auto headerToRemove = "Detailed Description"_L1;
            const auto firstIndex = result.indexOf(headerToRemove);
            if (firstIndex != -1)
                result.remove(firstIndex, headerToRemove.length());
        }
    }

    stripAllHtml(&result);
    return result.trimmed();
}

QString ExtractQmlProperty::extract(const QString &code, const QString &keyword, ExtractionMode mode)
{
    QString result;
    // Qt 5.15 way of finding properties in doc
    QString startMark = QString::fromLatin1("<a name=\"%1-prop\">").arg(keyword);
    qsizetype startIndex = code.indexOf(startMark);
    if (startIndex == -1) {
        // if not found, try Qt6
        startMark = QString::fromLatin1(
                            "<td class=\"tblQmlPropNode\"><p>\n<span class=\"name\">%1</span>")
                            .arg(keyword);
        startIndex = code.indexOf(startMark);
    }

    if (startIndex != -1) {
        result = code.mid(startIndex + startMark.size());
        startIndex = result.indexOf(QLatin1String("<div class=\"qmldoc\"><p>"));
    } else {
        result = getContentsByMarks(code, keyword + "-prop"_L1, keyword );
        startIndex = result.indexOf(QLatin1String("<p>"));
    }

    if (startIndex == -1)
        return {};
    result = result.mid(startIndex);
    if (mode == ExtractionMode::Simplified)
        processOutput(&result);
    stripAllHtml(&result);
    return result.trimmed();
}

QString ExtractQmlMethodOrSignal::extract(const QString &code, const QString &keyword, ExtractionMode mode)
{
    // the case with <!-- $$$childAt[overload1]$$$childAtrealreal -->
    QString mark = QString::fromLatin1("$$$%1[overload1]$$$%1").arg(keyword);
    qsizetype startIndex = code.indexOf(mark);
    if (startIndex != -1) {
        startIndex = code.indexOf("-->"_L1, startIndex + mark.length());
        if (startIndex == -1)
            return {};
    } else {
        // it could be part of the method list
        mark = QString::fromLatin1("<span class=\"name\">%1</span>")
                .arg(keyword);
        startIndex = code.indexOf(mark);
        if (startIndex != -1)
            startIndex += mark.length();
        else
            return {};
    }

    startIndex = code.indexOf(QLatin1String("<div class=\"qmldoc\"><p>"), startIndex);
    if (startIndex == -1)
        return {};

    QString endMark = QString::fromLatin1("<!-- @@@");
    qsizetype endIndex = code.indexOf(endMark, startIndex);
    QString contents = code.mid(startIndex, endIndex);
    if (mode == ExtractionMode::Simplified)
        processOutput(&contents);
    stripAllHtml(&contents);
    return contents.trimmed();
}

ExtractDocumentation::ExtractDocumentation(QQmlJS::Dom::DomType domType)
{
    using namespace QQmlJS::Dom;
    switch (domType) {
    case DomType::QmlObject:
        m_extractor = std::make_unique<ExtractQmlType>();
        break;
    case DomType::Binding:
    case DomType::PropertyDefinition:
        m_extractor = std::make_unique<ExtractQmlProperty>();
        break;
    case DomType::MethodInfo:
        m_extractor = std::make_unique<ExtractQmlMethodOrSignal>();
        break;
    default:
        break;
    }
}

QString ExtractDocumentation::execute(const QString &code, const QString &keyword, HtmlExtractor::ExtractionMode mode)
{
    Q_ASSERT(m_extractor);
    return m_extractor->extract(code, keyword, mode);
}

QT_END_NAMESPACE
