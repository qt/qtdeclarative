/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**/
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
        int len = line.length();
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

QMLDOM_EXPORT class LineWriterOptions
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
    LineEndings lineEndings = LineEndings::Unix;
    TrailingSpace codeTrailingSpace = TrailingSpace::Remove;
    TrailingSpace commentTrailingSpace = TrailingSpace::Remove;
    TrailingSpace stringTrailingSpace = TrailingSpace::Preserve;
    FormatOptions formatOptions;
    Updates updateOptions = Update::Default;
    AttributesSequence attributesSequence = AttributesSequence::Normalize;
};
Q_DECLARE_OPERATORS_FOR_FLAGS(LineWriterOptions::Updates)

using PendingSourceLocationId = QAtomicInt;
class LineWriter;

QMLDOM_EXPORT class PendingSourceLocation
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

QMLDOM_EXPORT class LineWriter
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
    int m_lineNr;
    int m_columnNr; // columnNr (starts at 0) of committed data
    int m_lineUtf16Offset; // utf16 offset since last newline (what is typically stores as
                           // SourceLocation::startColumn
    int m_currentColumnNr; // current columnNr (starts at 0)
    int m_utf16Offset; // utf16 offset since start for committed data
    QString m_currentLine;
    LineWriterOptions m_options;
    PendingSourceLocationId m_lastSourceLocationId;
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
