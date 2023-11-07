// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "documenthandler.h"

#include <QFile>
#include <QFileInfo>
#include <QFileSelector>
#include <QMimeDatabase>
#include <QQmlFile>
#include <QQmlFileSelector>
#include <QQuickTextDocument>
#include <QTextCharFormat>
#include <QStringDecoder>
#include <QTextDocument>
#include <QDebug>

DocumentHandler::DocumentHandler(QObject *parent)
    : QObject(parent)
    , m_document(nullptr)
    , m_cursorPosition(-1)
    , m_selectionStart(0)
    , m_selectionEnd(0)
{
}

QQuickTextDocument *DocumentHandler::document() const
{
    return m_document;
}

void DocumentHandler::setDocument(QQuickTextDocument *document)
{
    if (document == m_document)
        return;

    if (m_document)
        disconnect(m_document->textDocument(), &QTextDocument::modificationChanged, this, &DocumentHandler::modifiedChanged);
    m_document = document;
    if (m_document)
        connect(m_document->textDocument(), &QTextDocument::modificationChanged, this, &DocumentHandler::modifiedChanged);
    emit documentChanged();
}

int DocumentHandler::cursorPosition() const
{
    return m_cursorPosition;
}

void DocumentHandler::setCursorPosition(int position)
{
    if (position == m_cursorPosition)
        return;

    m_cursorPosition = position;
    reset();
    emit cursorPositionChanged();
}

int DocumentHandler::selectionStart() const
{
    return m_selectionStart;
}

void DocumentHandler::setSelectionStart(int position)
{
    if (position == m_selectionStart)
        return;

    m_selectionStart = position;
    emit selectionStartChanged();
}

int DocumentHandler::selectionEnd() const
{
    return m_selectionEnd;
}

void DocumentHandler::setSelectionEnd(int position)
{
    if (position == m_selectionEnd)
        return;

    m_selectionEnd = position;
    emit selectionEndChanged();
}

QColor DocumentHandler::textColor() const
{
    QTextCursor cursor = textCursor();
    if (cursor.isNull())
        return QColor(Qt::black);
    QTextCharFormat format = cursor.charFormat();
    return format.foreground().color();
}

void DocumentHandler::setTextColor(const QColor &color)
{
    QTextCharFormat format;
    format.setForeground(QBrush(color));
    mergeFormatOnWordOrSelection(format);
    emit textColorChanged();
}

Qt::Alignment DocumentHandler::alignment() const
{
    QTextCursor cursor = textCursor();
    if (cursor.isNull())
        return Qt::AlignLeft;
    return textCursor().blockFormat().alignment();
}

void DocumentHandler::setAlignment(Qt::Alignment alignment)
{
    QTextBlockFormat format;
    format.setAlignment(alignment);
    QTextCursor cursor = textCursor();
    cursor.mergeBlockFormat(format);
    emit alignmentChanged();
}

QString DocumentHandler::fileName() const
{
    const QString filePath = QQmlFile::urlToLocalFileOrQrc(m_fileUrl);
    const QString fileName = QFileInfo(filePath).fileName();
    if (fileName.isEmpty())
        return QStringLiteral("untitled.txt");
    return fileName;
}

QString DocumentHandler::fileType() const
{
    return QFileInfo(fileName()).suffix();
}

QUrl DocumentHandler::fileUrl() const
{
    return m_fileUrl;
}

void DocumentHandler::load(const QUrl &fileUrl)
{
    if (fileUrl == m_fileUrl)
        return;

    QQmlEngine *engine = qmlEngine(this);
    if (!engine) {
        qWarning() << "load() called before DocumentHandler has QQmlEngine";
        return;
    }

    const QUrl path = engine->interceptUrl(fileUrl, QQmlAbstractUrlInterceptor::UrlString);
    const QString fileName = QQmlFile::urlToLocalFileOrQrc(path);
    if (QFile::exists(fileName)) {
        QMimeType mime = QMimeDatabase().mimeTypeForFile(fileName);
        QFile file(fileName);
        if (file.open(QFile::ReadOnly | QFile::Text)) {
            QByteArray data = file.readAll();
            if (QTextDocument *doc = textDocument()) {
                doc->setBaseUrl(path.adjusted(QUrl::RemoveFilename));
                doc->setModified(false);
                if (mime.inherits("text/markdown")) {
                    emit loaded(QString::fromUtf8(data), Qt::MarkdownText);
                } else {
                    auto encoding = QStringConverter::encodingForHtml(data);
                    if (encoding) {
                        QStringDecoder decoder(*encoding);
                        emit loaded(decoder(data), Qt::AutoText);
                    } else {
                        // fall back to utf8
                        emit loaded(QString::fromUtf8(data), Qt::AutoText);
                    }
                }
            }

            reset();
        }
    }

    m_fileUrl = fileUrl;
    emit fileUrlChanged();
}

void DocumentHandler::saveAs(const QUrl &fileUrl)
{
    QTextDocument *doc = textDocument();
    if (!doc)
        return;

    const QString filePath = fileUrl.toLocalFile();
    const bool isHtml = QFileInfo(filePath).suffix().contains(QLatin1String("htm"));
    QFile file(filePath);
    if (!file.open(QFile::WriteOnly | QFile::Truncate | (isHtml ? QFile::NotOpen : QFile::Text))) {
        emit error(tr("Cannot save: ") + file.errorString());
        return;
    }
    file.write((isHtml ? doc->toHtml() : doc->toPlainText()).toUtf8());
    file.close();

    if (fileUrl == m_fileUrl)
        return;

    m_fileUrl = fileUrl;
    emit fileUrlChanged();
}

void DocumentHandler::reset()
{
    emit alignmentChanged();
    emit textColorChanged();
    emit fontChanged();
}

QTextCursor DocumentHandler::textCursor() const
{
    QTextDocument *doc = textDocument();
    if (!doc)
        return QTextCursor();

    QTextCursor cursor = QTextCursor(doc);
    if (m_selectionStart != m_selectionEnd) {
        cursor.setPosition(m_selectionStart);
        cursor.setPosition(m_selectionEnd, QTextCursor::KeepAnchor);
    } else {
        cursor.setPosition(m_cursorPosition);
    }
    return cursor;
}

QTextDocument *DocumentHandler::textDocument() const
{
    if (!m_document)
        return nullptr;

    return m_document->textDocument();
}

void DocumentHandler::mergeFormatOnWordOrSelection(const QTextCharFormat &format)
{
    QTextCursor cursor = textCursor();
    if (!cursor.hasSelection())
        cursor.select(QTextCursor::WordUnderCursor);
    cursor.mergeCharFormat(format);
}

bool DocumentHandler::modified() const
{
    return m_document && m_document->textDocument()->isModified();
}

void DocumentHandler::setModified(bool m)
{
    if (m_document)
        m_document->textDocument()->setModified(m);
}

QFont DocumentHandler::font() const
{
    QTextCursor cursor = textCursor();
    if (cursor.isNull())
        return m_document->textDocument()->defaultFont();
    QTextCharFormat format = cursor.charFormat();
    return format.font();
}

void DocumentHandler::setFont(const QFont & font){

    QTextCursor cursor = textCursor();
    if (!cursor.isNull() && cursor.charFormat().font() == font)
        return;

    QTextCharFormat format;
    format.setFont(font);
    mergeFormatOnWordOrSelection(format);

    emit fontChanged();
}

bool DocumentHandler::bold() const
{
    const QTextCursor cursor = textCursor();
    if (cursor.isNull())
        return m_document->textDocument()->defaultFont().bold();
    return cursor.charFormat().font().bold();
}

void DocumentHandler::setBold(bool bold)
{
    const QTextCursor cursor = textCursor();
    if (!cursor.isNull() && cursor.charFormat().font().bold() == bold)
        return;

    QFont font = cursor.charFormat().font();
    font.setBold(bold);
    QTextCharFormat format;
    format.setFont(font);
    mergeFormatOnWordOrSelection(format);

    emit boldChanged();
}

bool DocumentHandler::underline() const
{
    const QTextCursor cursor = textCursor();
    if (cursor.isNull())
        return m_document->textDocument()->defaultFont().underline();
    return cursor.charFormat().font().underline();
}

void DocumentHandler::setUnderline(bool underline)
{
    const QTextCursor cursor = textCursor();
    if (!cursor.isNull() && cursor.charFormat().font().underline() == underline)
        return;

    QFont font = cursor.charFormat().font();
    font.setUnderline(underline);
    QTextCharFormat format;
    format.setFont(font);
    mergeFormatOnWordOrSelection(format);

    emit underlineChanged();
}

bool DocumentHandler::italic() const
{
    const QTextCursor cursor = textCursor();
    if (cursor.isNull())
        return m_document->textDocument()->defaultFont().italic();
    return cursor.charFormat().font().italic();
}

void DocumentHandler::setItalic(bool italic)
{
    const QTextCursor cursor = textCursor();
    if (!cursor.isNull() && cursor.charFormat().font().italic() == italic)
        return;

    QFont font = cursor.charFormat().font();
    font.setItalic(italic);
    QTextCharFormat format;
    format.setFont(font);
    mergeFormatOnWordOrSelection(format);

    emit italicChanged();
}

#include "moc_documenthandler.cpp"
