// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmldomlinewriter_p.h"
#include <QtCore/QCoreApplication>
#include <QtCore/QRegularExpression>

QT_BEGIN_NAMESPACE
namespace QQmlJS {
namespace Dom {

quint32 PendingSourceLocation::utf16Start() const
{
    return value.offset;
}

quint32 PendingSourceLocation::utf16End() const
{
    return value.offset + value.length;
}

void PendingSourceLocation::changeAtOffset(quint32 offset, qint32 change, qint32 colChange,
                                           qint32 lineChange)
{
    if (offset < utf16Start()) {
        if (change < 0 && offset - change >= utf16Start()) {
            int c1 = offset - utf16Start();
            int c2 = offset - change - utf16Start();
            change = c1;
            if (value.length < quint32(c2))
                value.length = 0;
            else
                value.length -= c2;
        }
        value.offset += change;
        value.startColumn += colChange;
        value.startLine += lineChange;
    } else if (offset < utf16End()) {
        if (change < 0 && offset - change > utf16End())
            change = offset - utf16End();
        value.length += change;
    }
}

void PendingSourceLocation::commit()
{
    if (toUpdate)
        *toUpdate = value;
    if (updater)
        updater(value);
}

LineWriter::LineWriter(SinkF innerSink, QString fileName, const LineWriterOptions &options,
                       int lineNr, int columnNr, int utf16Offset, QString currentLine)
    : m_innerSinks({ innerSink }),
      m_fileName(fileName),
      m_lineNr(lineNr),
      m_columnNr(columnNr),
      m_currentColumnNr(columnNr),
      m_utf16Offset(utf16Offset),
      m_currentLine(currentLine),
      m_options(options)
{
}

LineWriter &LineWriter::ensureNewline(int nNewline, TextAddType t)
{
    int nToAdd = nNewline;
    if (nToAdd <= 0)
        return *this;
    if (m_currentLine.trimmed().isEmpty()) {
        --nToAdd;
        if (m_committedEmptyLines >= unsigned(nToAdd))
            return *this;
        nToAdd -= m_committedEmptyLines;
    }
    for (int i = 0; i < nToAdd; ++i)
        write(u"\n", t);
    return *this;
}

LineWriter &LineWriter::ensureSpace(TextAddType t)
{
    if (!m_currentLine.isEmpty() && !m_currentLine.at(m_currentLine.size() - 1).isSpace())
        write(u" ", t);
    return *this;
}

LineWriter &LineWriter::ensureSpace(QStringView space, TextAddType t)
{
    int tabSize = m_options.formatOptions.tabSize;
    IndentInfo ind(space, tabSize);
    auto cc = counter();
    if (ind.nNewlines > 0)
        ensureNewline(ind.nNewlines, t);
    if (cc != counter() || m_currentLine.isEmpty()
        || !m_currentLine.at(m_currentLine.size() - 1).isSpace())
        write(ind.trailingString, t);
    else {
        int len = m_currentLine.size();
        int i = len;
        while (i != 0 && m_currentLine.at(i - 1).isSpace())
            --i;
        QStringView trailingSpace = QStringView(m_currentLine).mid(i, len - i);
        int trailingSpaceStartColumn =
                IndentInfo(QStringView(m_currentLine).mid(0, i), tabSize, m_columnNr).column;
        IndentInfo indExisting(trailingSpace, tabSize, trailingSpaceStartColumn);
        if (trailingSpaceStartColumn != 0)
            ind = IndentInfo(space, tabSize, trailingSpaceStartColumn);
        if (i == 0) {
            if (indExisting.column < ind.column) {
                qint32 utf16Change = ind.trailingString.size() - trailingSpace.size();
                m_currentColumnNr += ind.trailingString.size() - trailingSpace.size();
                m_currentLine.replace(
                        i, len - i, ind.trailingString.toString()); // invalidates most QStringViews
                changeAtOffset(i, utf16Change, utf16Change, 0);
                lineChanged();
            }
        } else if (indExisting.column < ind.column) { // use just spaces if not at start of a line
            write(QStringLiteral(u" ").repeated(ind.column - indExisting.column), t);
        }
    }
    return *this;
}

QString LineWriter::eolToWrite() const
{
    switch (m_options.lineEndings) {
    case LineWriterOptions::LineEndings::Unix:
        return QStringLiteral(u"\n");
    case LineWriterOptions::LineEndings::Windows:
        return QStringLiteral(u"\r\n");
    case LineWriterOptions::LineEndings::OldMacOs:
        return QStringLiteral(u"\r");
    }
    Q_ASSERT(false);
    return QStringLiteral(u"\n");
}

template<typename String, typename ...Args>
static QRegularExpressionMatch matchHelper(QRegularExpression &re, String &&s, Args &&...args)
{
    return re.matchView(s, args...);
}

LineWriter &LineWriter::write(QStringView v, TextAddType tAdd)
{
    QString eol;
    // split multiple lines
    static QRegularExpression eolRe(QLatin1String(
            "(\r?\n|\r)")); // does not support split of \r and \n for windows style line endings
    QRegularExpressionMatch m = matchHelper(eolRe, v);
    if (m.hasMatch()) {
        // add line by line
        auto i = m.capturedStart(1);
        auto iEnd = m.capturedEnd(1);
        eol = eolToWrite();
        // offset change (eol used vs input) cannot affect things,
        // because we cannot have already opened or closed a PendingSourceLocation
        if (iEnd < v.size()) {
            write(v.mid(0, iEnd));
            m = matchHelper(eolRe, v, iEnd);
            while (m.hasMatch()) {
                write(v.mid(iEnd, m.capturedEnd(1) - iEnd));
                iEnd = m.capturedEnd(1);
                m = matchHelper(eolRe, v, iEnd);
            }
            if (iEnd < v.size())
                write(v.mid(iEnd, v.size() - iEnd));
            return *this;
        }
        QStringView toAdd = v.mid(0, i);
        if (!toAdd.trimmed().isEmpty())
            textAddCallback(tAdd);
        m_counter += i;
        m_currentLine.append(toAdd);
        m_currentColumnNr +=
                IndentInfo(toAdd, m_options.formatOptions.tabSize, m_currentColumnNr).column;
        lineChanged();
    } else {
        if (!v.trimmed().isEmpty())
            textAddCallback(tAdd);
        m_counter += v.size();
        m_currentLine.append(v);
        m_currentColumnNr +=
                IndentInfo(v, m_options.formatOptions.tabSize, m_currentColumnNr).column;
        lineChanged();
    }
    if (!eol.isEmpty()
        || (m_options.maxLineLength > 0 && m_currentColumnNr > m_options.maxLineLength)) {
        reindentAndSplit(eol);
    }
    return *this;
}

void LineWriter::flush()
{
    if (m_currentLine.size() > 0)
        commitLine(QString());
}

void LineWriter::eof(bool shouldEnsureNewline)
{
    if (shouldEnsureNewline)
        ensureNewline();
    reindentAndSplit(QString(), true);
}

SourceLocation LineWriter::committedLocation() const
{
    return SourceLocation(m_utf16Offset, 0, m_lineNr, m_lineUtf16Offset);
}

PendingSourceLocationId LineWriter::startSourceLocation(SourceLocation *toUpdate)
{
    PendingSourceLocation res;
    res.id = ++m_lastSourceLocationId;
    res.value = currentSourceLocation();
    res.toUpdate = toUpdate;
    m_pendingSourceLocations.insert(res.id, res);
    return res.id;
}

PendingSourceLocationId LineWriter::startSourceLocation(std::function<void(SourceLocation)> updater)
{
    PendingSourceLocation res;
    res.id = ++m_lastSourceLocationId;
    res.value = currentSourceLocation();
    res.updater = updater;
    m_pendingSourceLocations.insert(res.id, res);
    return res.id;
}

void LineWriter::endSourceLocation(PendingSourceLocationId slId)
{
    if (m_pendingSourceLocations.contains(slId)) {
        auto &pLoc = m_pendingSourceLocations[slId];
        if (!pLoc.open) {
            qWarning() << "Trying to close already closed PendingSourceLocation" << int(slId);
        }
        pLoc.open = false;
        pLoc.value.length = m_utf16Offset + m_currentLine.size() - pLoc.value.offset;
    } else {
        qWarning() << "Trying to close non existing PendingSourceLocation" << int(slId);
    }
}

int LineWriter::addTextAddCallback(std::function<bool(LineWriter &, TextAddType)> callback)
{
    int nextId = ++m_lastCallbackId;
    Q_ASSERT(nextId != 0);
    if (callback)
        m_textAddCallbacks.insert(nextId, callback);
    return nextId;
}

int LineWriter::addNewlinesAutospacerCallback(int nLines)
{
    return addTextAddCallback([nLines](LineWriter &self, TextAddType t) {
        if (t == TextAddType::Normal) {
            quint32 c = self.counter();
            QString spacesToPreserve;
            bool spaceOnly = QStringView(self.m_currentLine).trimmed().isEmpty();
            if (spaceOnly && !self.m_currentLine.isEmpty())
                spacesToPreserve = self.m_currentLine;
            self.ensureNewline(nLines, LineWriter::TextAddType::Extra);
            if (self.counter() != c && !spacesToPreserve.isEmpty())
                self.write(spacesToPreserve, TextAddType::Extra);
            return false;
        } else {
            return true;
        }
    });
}

void LineWriter::setLineIndent(int indentAmount)
{
    int startNonSpace = 0;
    while (startNonSpace < m_currentLine.size() && m_currentLine.at(startNonSpace).isSpace())
        ++startNonSpace;
    int oldColumn = column(startNonSpace);
    if (indentAmount >= 0) {
        QString indent;
        if (m_options.formatOptions.useTabs) {
            indent = QStringLiteral(u"\t").repeated(indentAmount / m_options.formatOptions.tabSize)
                    + QStringLiteral(u" ").repeated(indentAmount % m_options.formatOptions.tabSize);
        } else {
            indent = QStringLiteral(u" ").repeated(indentAmount);
        }
        if (indent != m_currentLine.mid(0, startNonSpace)) {
            quint32 colChange = indentAmount - oldColumn;
            m_currentColumnNr += colChange;
            qint32 oChange = indent.size() - startNonSpace;
            m_currentLine = indent + m_currentLine.mid(startNonSpace);
            m_currentColumnNr = column(m_currentLine.size());
            lineChanged();
            changeAtOffset(m_utf16Offset, oChange, oChange, 0);
        }
    }
}

void LineWriter::handleTrailingSpace(LineWriterOptions::TrailingSpace trailingSpace)
{
    switch (trailingSpace) {
    case LineWriterOptions::TrailingSpace::Preserve:
        break;
    case LineWriterOptions::TrailingSpace::Remove: {
        int lastNonSpace = m_currentLine.size();
        while (lastNonSpace > 0 && m_currentLine.at(lastNonSpace - 1).isSpace())
            --lastNonSpace;
        if (lastNonSpace != m_currentLine.size()) {
            qint32 oChange = lastNonSpace - m_currentLine.size();
            m_currentLine = m_currentLine.mid(0, lastNonSpace);
            changeAtOffset(m_utf16Offset + lastNonSpace, oChange, oChange, 0);
            m_currentColumnNr =
                    column(m_currentLine.size()); // to be extra accurate in the potential split
            lineChanged();
        }
    } break;
    }
}

void LineWriter::reindentAndSplit(QString eol, bool eof)
{
    // maybe write out
    if (!eol.isEmpty() || eof) {
        handleTrailingSpace(m_options.codeTrailingSpace);
        commitLine(eol);
    }
}

SourceLocation LineWriter::currentSourceLocation() const
{
    return SourceLocation(m_utf16Offset + m_currentLine.size(), 0, m_lineNr,
                          m_lineUtf16Offset + m_currentLine.size());
}

void LineWriter::changeAtOffset(quint32 offset, qint32 change, qint32 colChange, qint32 lineChange)
{
    auto iEnd = m_pendingSourceLocations.end();
    auto i = m_pendingSourceLocations.begin();
    while (i != iEnd) {
        i.value().changeAtOffset(offset, change, colChange, lineChange);
        ++i;
    }
}

int LineWriter::column(int index)
{
    if (index > m_currentLine.size())
        index = m_currentLine.size();
    IndentInfo iInfo(QStringView(m_currentLine).mid(0, index), m_options.formatOptions.tabSize,
                     m_columnNr);
    return iInfo.column;
}

void LineWriter::textAddCallback(LineWriter::TextAddType t)
{
    if (m_textAddCallbacks.isEmpty())
        return;
    int iNow = (--m_textAddCallbacks.end()).key() + 1;
    while (true) {
        auto it = m_textAddCallbacks.lowerBound(iNow);
        if (it == m_textAddCallbacks.begin())
            break;
        --it;
        iNow = it.key();
        if (!it.value()(*this, t))
            m_textAddCallbacks.erase(it);
    }
}

void LineWriter::commitLine(QString eol, TextAddType tType, int untilChar)
{
    if (untilChar == -1)
        untilChar = m_currentLine.size();
    bool isSpaceOnly = QStringView(m_currentLine).mid(0, untilChar).trimmed().isEmpty();
    bool isEmptyNewline = !eol.isEmpty() && isSpaceOnly;
    quint32 endCommit = m_utf16Offset + untilChar;
    // update position, lineNr,...
    // write out
    for (SinkF &sink : m_innerSinks)
        sink(m_currentLine.mid(0, untilChar));
    m_utf16Offset += untilChar;
    if (!eol.isEmpty()) {
        m_utf16Offset += eol.size();
        for (SinkF &sink : m_innerSinks)
            sink(eol);
        ++m_lineNr;
        int oldCol = column(untilChar);
        m_columnNr = 0;
        m_lineUtf16Offset = 0;
        changeAtOffset(m_utf16Offset, 0, -oldCol, 1);
    } else {
        m_columnNr = column(untilChar);
        m_lineUtf16Offset += untilChar;
    }
    if (untilChar == m_currentLine.size()) {
        willCommit();
        m_currentLine.clear();
    } else {
        QString nextLine = m_currentLine.mid(untilChar);
        m_currentLine = m_currentLine.mid(0, untilChar);
        lineChanged();
        willCommit();
        m_currentLine = nextLine;
    }
    lineChanged();
    m_currentColumnNr = column(m_currentLine.size());
    TextAddType notifyType = tType;
    switch (tType) {
    case TextAddType::Normal:
        if (eol.isEmpty())
            notifyType = TextAddType::PartialCommit;
        else
            notifyType = TextAddType::Newline;
        break;
    case TextAddType::Extra:
        if (eol.isEmpty())
            notifyType = TextAddType::NewlineExtra;
        else
            notifyType = TextAddType::PartialCommit;
        break;
    case TextAddType::Newline:
    case TextAddType::NewlineSplit:
    case TextAddType::NewlineExtra:
    case TextAddType::PartialCommit:
    case TextAddType::Eof:
        break;
    }
    if (isEmptyNewline)
        ++m_committedEmptyLines;
    else if (!isSpaceOnly)
        m_committedEmptyLines = 0;
    // commit finished pending
    auto iEnd = m_pendingSourceLocations.end();
    auto i = m_pendingSourceLocations.begin();
    while (i != iEnd) {
        auto &pLoc = i.value();
        if (!pLoc.open && pLoc.utf16End() <= endCommit) {
            pLoc.commit();
            i = m_pendingSourceLocations.erase(i);
        } else {
            ++i;
        }
    }
    // notify
    textAddCallback(notifyType);
}

} // namespace Dom
} // namespace QQmlJS
QT_END_NAMESPACE

#include "moc_qqmldomlinewriter_p.cpp"
