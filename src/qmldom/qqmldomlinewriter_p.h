// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLDOMLINEWRITER_P
#define QQMLDOMLINEWRITER_P

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

#include "qqmldom_global.h"
#include "qqmldomstringdumper_p.h"

#include <QtQml/private/qqmljssourcelocation_p.h>
#include <QtCore/QObject>
#include <QtCore/QAtomicInt>
#include <QtCore/QMap>
#include <functional>

QT_BEGIN_NAMESPACE
namespace QQmlJS {
namespace Dom {

class IndentInfo
{
public:
    QStringView string;
    QStringView trailingString;
    int nNewlines = 0;
    int column = 0;

    IndentInfo(QStringView line, int tabSize, int initialColumn = 0)
    {
        string = line;
        int fixup = 0;
        if (initialColumn < 0) // we do not want % of negative numbers
            fixup = (-initialColumn + tabSize - 1) / tabSize * tabSize;
        column = initialColumn + fixup;
        const QChar tab = QLatin1Char('\t');
        int iStart = 0;
        int len = line.size();
        for (int i = 0; i < len; i++) {
            if (line[i] == tab)
                column = ((column / tabSize) + 1) * tabSize;
            else if (line[i] == QLatin1Char('\n')
                     || (line[i] == QLatin1Char('\r')
                         && (i + 1 == len || line[i + 1] != QLatin1Char('\n')))) {
                iStart = i + 1;
                ++nNewlines;
                column = 0;
            } else if (!line[i].isLowSurrogate())
                column++;
        }
        column -= fixup;
        trailingString = line.mid(iStart);
    }
};

class QMLDOM_EXPORT FormatOptions
{
public:
    int tabSize = 4;
    int indentSize = 4;
    bool useTabs = false;
};

class QMLDOM_EXPORT LineWriterOptions
{
    Q_GADGET
public:
    enum class LineEndings { Unix, Windows, OldMacOs };
    Q_ENUM(LineEndings)
    enum class TrailingSpace { Preserve, Remove };
    Q_ENUM(TrailingSpace)
    enum class Update { None = 0, Expressions = 0x1, Locations = 0x2, All = 0x3, Default = All };
    Q_ENUM(Update)
    Q_DECLARE_FLAGS(Updates, Update)
    enum class AttributesSequence { Normalize, Preserve };
    Q_ENUM(AttributesSequence)

    int maxLineLength = -1;
    int strongMaxLineExtra = 20;
    int minContentLength = 10;
#if defined (Q_OS_WIN)
    LineEndings lineEndings = LineEndings::Windows;
#else
    LineEndings lineEndings = LineEndings::Unix;
#endif
    TrailingSpace codeTrailingSpace = TrailingSpace::Remove;
    TrailingSpace commentTrailingSpace = TrailingSpace::Remove;
    TrailingSpace stringTrailingSpace = TrailingSpace::Preserve;
    FormatOptions formatOptions;
    Updates updateOptions = Update::Default;
    AttributesSequence attributesSequence = AttributesSequence::Normalize;
    bool objectsSpacing = false;
    bool functionsSpacing = false;
};
Q_DECLARE_OPERATORS_FOR_FLAGS(LineWriterOptions::Updates)

using PendingSourceLocationId = int;
using PendingSourceLocationIdAtomic = QAtomicInt;
class LineWriter;

class QMLDOM_EXPORT PendingSourceLocation
{
    Q_GADGET
public:
    quint32 utf16Start() const;
    quint32 utf16End() const;
    void changeAtOffset(quint32 offset, qint32 change, qint32 colChange, qint32 lineChange);
    void commit();
    PendingSourceLocationId id;
    SourceLocation value;
    SourceLocation *toUpdate = nullptr;
    std::function<void(SourceLocation)> updater = nullptr;
    bool open = true;
};

class QMLDOM_EXPORT LineWriter
{
    Q_GADGET
public:
    enum class TextAddType {
        Normal,
        Extra,
        Newline,
        NewlineSplit,
        NewlineExtra,
        PartialCommit,
        Eof
    };

    LineWriter(SinkF innerSink, QString fileName,
               const LineWriterOptions &options = LineWriterOptions(), int lineNr = 0,
               int columnNr = 0, int utf16Offset = 0, QString currentLine = QString());
    std::function<void(QStringView)> sink()
    {
        return [this](QStringView s) { this->write(s); };
    }

    virtual ~LineWriter() { }

    QList<SinkF> innerSinks() { return m_innerSinks; }
    void addInnerSink(SinkF s) { m_innerSinks.append(s); }
    LineWriter &ensureNewline(int nNewlines = 1, TextAddType t = TextAddType::Extra);
    LineWriter &ensureSpace(TextAddType t = TextAddType::Extra);
    LineWriter &ensureSpace(QStringView space, TextAddType t = TextAddType::Extra);

    LineWriter &newline()
    {
        write(u"\n");
        return *this;
    }
    LineWriter &space()
    {
        write(u" ");
        return *this;
    }
    LineWriter &write(QStringView v, TextAddType tType = TextAddType::Normal);
    LineWriter &write(QStringView v, SourceLocation *toUpdate)
    {
        auto pLoc = startSourceLocation(toUpdate);
        write(v);
        endSourceLocation(pLoc);
        return *this;
    }
    void commitLine(QString eol, TextAddType t = TextAddType::Normal, int untilChar = -1);
    void flush();
    void eof(bool ensureNewline = true);
    SourceLocation committedLocation() const;
    PendingSourceLocationId startSourceLocation(SourceLocation *);
    PendingSourceLocationId startSourceLocation(std::function<void(SourceLocation)>);
    void endSourceLocation(PendingSourceLocationId);
    quint32 counter() const { return m_counter; }
    int addTextAddCallback(std::function<bool(LineWriter &, TextAddType)> callback);
    bool removeTextAddCallback(int i) { return m_textAddCallbacks.remove(i); }
    int addNewlinesAutospacerCallback(int nLines);
    void handleTrailingSpace(LineWriterOptions::TrailingSpace s);
    void setLineIndent(int indentAmount);
    QString fileName() const { return m_fileName; }
    const QString &currentLine() const { return m_currentLine; }
    const LineWriterOptions &options() const { return m_options; }
    virtual void lineChanged() { }
    virtual void reindentAndSplit(QString eol, bool eof = false);
    virtual void willCommit() { }

private:
    Q_DISABLE_COPY_MOVE(LineWriter)
protected:
    void changeAtOffset(quint32 offset, qint32 change, qint32 colChange, qint32 lineChange);
    QString eolToWrite() const;
    SourceLocation currentSourceLocation() const;
    int column(int localIndex);
    void textAddCallback(TextAddType t);

    QList<SinkF> m_innerSinks;
    QString m_fileName;
    int m_lineNr = 0;
    int m_columnNr = 0; // columnNr (starts at 0) of committed data
    int m_lineUtf16Offset = 0; // utf16 offset since last newline (what is typically stores as
                               // SourceLocation::startColumn
    int m_currentColumnNr = 0; // current columnNr (starts at 0)
    int m_utf16Offset = 0; // utf16 offset since start for committed data
    QString m_currentLine;
    LineWriterOptions m_options;
    PendingSourceLocationIdAtomic m_lastSourceLocationId;
    QMap<PendingSourceLocationId, PendingSourceLocation> m_pendingSourceLocations;
    QAtomicInt m_lastCallbackId;
    QMap<int, std::function<bool(LineWriter &, TextAddType)>> m_textAddCallbacks;
    quint32 m_counter = 0;
    quint32 m_committedEmptyLines = 0x7FFFFFFF;
    bool m_reindent = true;
};

} // namespace Dom
} // namespace QQmlJS
QT_END_NAMESPACE
#endif
