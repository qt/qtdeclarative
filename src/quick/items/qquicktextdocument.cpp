// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquicktextdocument.h"
#include "qquicktextdocument_p.h"

#include "qquicktextedit_p.h"

#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlfile.h>
#include <QtQml/qqmlinfo.h>
#include <QtQuick/private/qquickpixmap_p.h>

#include <QtCore/qfile.h>
#include <QtCore/qpointer.h>

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(lcTextDoc, "qt.quick.textdocument")

using namespace Qt::StringLiterals;

/*!
    \qmltype TextDocument
    \nativetype QQuickTextDocument
    \inqmlmodule QtQuick
    \brief A wrapper around TextEdit's backing QTextDocument.
    \preliminary

    To load text into the document, set the \l source property. If the user then
    modifies the text and wants to save the same document, call \l save() to save
    it to the same source again (only if \l {QUrl::isLocalFile()}{it's a local file}).
    Or call \l saveAs() to save it to a different file.

    This class cannot be instantiated in QML, but is available from \l TextEdit::textDocument.

    \note All loading and saving is done synchronously for now.
    This may block the UI if the \l source is a slow network drive.
    This may be improved in future versions of Qt.

    \note This API is considered tech preview and may change in future versions of Qt.
*/

/*!
    \class QQuickTextDocument
    \since 5.1
    \brief The QQuickTextDocument class provides access to the QTextDocument of QQuickTextEdit.
    \inmodule QtQuick

    This class provides access to the QTextDocument of QQuickTextEdit elements.
    This is provided to allow usage of the \l{Rich Text Processing} functionalities of Qt,
    including document modifications. It can also be used to output content,
    for example with \l{QTextDocumentWriter}, or provide additional formatting,
    for example with \l{QSyntaxHighlighter}.
*/

/*!
   Constructs a QQuickTextDocument object with
   \a parent as the parent object.
*/
QQuickTextDocument::QQuickTextDocument(QQuickItem *parent)
    : QObject(*(new QQuickTextDocumentPrivate), parent)
{
    Q_D(QQuickTextDocument);
    Q_ASSERT(parent);
    d->editor = qobject_cast<QQuickTextEdit *>(parent);
    Q_ASSERT(d->editor);
    connect(textDocument(), &QTextDocument::modificationChanged,
            this, &QQuickTextDocument::modifiedChanged);
}

/*!
    \property QQuickTextDocument::status
    \brief the status of document loading or saving
    \since 6.7
    \preliminary

    This property holds the status of document loading or saving.  It can be one of:

    \value Null                     No file has been loaded
    \value Loading                  Reading from \l source has begun
    \value Loaded                   Reading has successfully finished
    \value Saving                   File writing has begun after save() or saveAs()
    \value Saved                    Writing has successfully finished
    \value ReadError                An error occurred while reading from \l source
    \value WriteError               An error occurred in save() or saveAs()
    \value NonLocalFileError        saveAs() was called with a URL pointing
                                    to a remote resource rather than a local file

    \sa errorString, source, save(), saveAs()
*/

/*!
    \qmlproperty enumeration QtQuick::TextDocument::status
    \readonly
    \since 6.7
    \preliminary

    This property holds the status of document loading or saving.  It can be one of:

    \value TextDocument.Null        No file has been loaded
    \value TextDocument.Loading     Reading from \l source has begun
    \value TextDocument.Loaded      Reading has successfully finished
    \value TextDocument.Saving      File writing has begun after save() or saveAs()
    \value TextDocument.Saved       Writing has successfully finished
    \value TextDocument.ReadError   An error occurred while reading from \l source
    \value TextDocument.WriteError  An error occurred in save() or saveAs()
    \value TextDocument.NonLocalFileError saveAs() was called with a URL pointing
                                    to a remote resource rather than a local file

    Use this status to provide an update or respond to the status change in some way.
    For example, you could:

    \list
    \li Trigger a state change:
    \qml
    State {
        name: 'loaded'
        when: textEdit.textDocument.status == textEdit.textDocument.Loaded
    }
    \endqml

    \li Implement an \c onStatusChanged signal handler:
    \qml
    TextEdit {
        onStatusChanged: {
            if (textDocument.status === textDocument.Loaded)
                console.log('Loaded')
        }
    }
    \endqml

    \li Bind to the status value:

    \snippet qml/textEditStatusSwitch.qml 0

    \endlist

    \sa errorString, source, save(), saveAs()
*/
QQuickTextDocument::Status QQuickTextDocument::status() const
{
    Q_D(const QQuickTextDocument);
    return d->status;
}

/*!
    \property QQuickTextDocument::errorString
    \brief a human-readable string describing the error that occurred during loading or saving, if any
    \since 6.7
    \preliminary

    By default this string is empty.

    \sa status, source, save(), saveAs()
*/

/*!
    \qmlproperty string QtQuick::TextDocument::errorString
    \readonly
    \since 6.7
    \preliminary

    This property holds a human-readable string describing the error that
    occurred during loading or saving, if any; otherwise, an empty string.

    \sa status, source, save(), saveAs()
*/
QString QQuickTextDocument::errorString() const
{
    Q_D(const QQuickTextDocument);
    return d->errorString;
}

void QQuickTextDocumentPrivate::setStatus(QQuickTextDocument::Status s, const QString &err)
{
    Q_Q(QQuickTextDocument);
    if (status == s)
        return;

    status = s;
    emit q->statusChanged();

    if (errorString == err)
        return;
    errorString = err;
    emit q->errorStringChanged();
    if (!err.isEmpty())
        qmlWarning(q) << err;
}

/*!
    \property QQuickTextDocument::source
    \brief the URL from which to load document contents
    \since 6.7
    \preliminary

    QQuickTextDocument can handle any text format supported by Qt, loaded from
    any URL scheme supported by Qt.

    The \c source property cannot be changed while the document's \l modified
    state is \c true. If the user has modified the document contents, you
    should prompt the user whether to \l save(), or else discard changes by
    setting \l modified to \c false before setting the \c source property to a
    different URL.

    \sa QTextDocumentWriter::supportedDocumentFormats()
*/

/*!
    \qmlproperty url QtQuick::TextDocument::source
    \since 6.7
    \preliminary

    QQuickTextDocument can handle any text format supported by Qt, loaded from
    any URL scheme supported by Qt.

    The URL may be absolute, or relative to the URL of the component.

    The \c source property cannot be changed while the document's \l modified
    state is \c true. If the user has modified the document contents, you
    should prompt the user whether to \l save(), or else discard changes by
    setting \c {modified = false} before setting the \l source property to a
    different URL.

    \sa QTextDocumentWriter::supportedDocumentFormats()
*/
QUrl QQuickTextDocument::source() const
{
    Q_D(const QQuickTextDocument);
    return d->url;
}

void QQuickTextDocument::setSource(const QUrl &url)
{
    Q_D(QQuickTextDocument);

    if (url == d->url)
        return;

    if (isModified()) {
        qmlWarning(this) << "Existing document modified: you should save(),"
                            " or set modified=false before setting a different source";
        return;
    }

    d->url = url;
    emit sourceChanged();
    d->load();
}

/*!
    \property QQuickTextDocument::modified
    \brief whether the document has been modified by the user
    \since 6.7
    \preliminary

    This property holds whether the document has been modified by the user
    since the last time it was loaded or saved. By default, this property is
    \c false.

    As with \l QTextDocument::modified, you can set the modified property:
    for example, set it to \c false to allow setting the \l source property
    to a different URL (thus discarding the user's changes).

    \sa QTextDocument::modified
*/

/*!
    \qmlproperty bool QtQuick::TextDocument::modified
    \since 6.7
    \preliminary

    This property holds whether the document has been modified by the user
    since the last time it was loaded or saved. By default, this property is
    \c false.

    As with \l QTextDocument::modified, you can set the modified property:
    for example, set it to \c false to allow setting the \l source property
    to a different URL (thus discarding the user's changes).

    \sa QTextDocument::modified
*/
bool QQuickTextDocument::isModified() const
{
    const auto *doc = textDocument();
    return doc && doc->isModified();
}

void QQuickTextDocument::setModified(bool modified)
{
    if (auto *doc = textDocument())
        doc->setModified(modified);
}

void QQuickTextDocumentPrivate::load()
{
    auto *doc = editor->document();
    if (!doc) {
        setStatus(QQuickTextDocument::Status::ReadError,
                  QQuickTextDocument::tr("Null document object: cannot load"));
        return;
    }
    const QQmlContext *context = qmlContext(editor);
    const QUrl &resolvedUrl = context ? context->resolvedUrl(url) : url;
    const QString filePath = QQmlFile::urlToLocalFileOrQrc(resolvedUrl);
    QFile file(filePath);
    if (file.exists()) {
#if QT_CONFIG(mimetype)
        QMimeType mimeType = QMimeDatabase().mimeTypeForFile(filePath);
        const bool isHtml = mimeType.inherits("text/html"_L1);
        const bool isMarkdown = mimeType.inherits("text/markdown"_L1)
                             || mimeType.inherits("text/x-web-markdown"_L1);    //Tika database
#else
        const bool isHtml = filePath.endsWith(".html"_L1, Qt::CaseInsensitive) ||
                filePath.endsWith(".htm"_L1, Qt::CaseInsensitive);
        const bool isMarkdown = filePath.endsWith(".md"_L1, Qt::CaseInsensitive) ||
                filePath.endsWith(".markdown"_L1, Qt::CaseInsensitive);
#endif
        if (isHtml)
            detectedFormat = Qt::RichText;
        else if (isMarkdown)
            detectedFormat = Qt::MarkdownText;
        else
            detectedFormat = Qt::PlainText;
        if (file.open(QFile::ReadOnly | QFile::Text)) {
            setStatus(QQuickTextDocument::Status::Loading, {});
            QByteArray data = file.readAll();
            doc->setBaseUrl(resolvedUrl.adjusted(QUrl::RemoveFilename));
#if QT_CONFIG(textmarkdownreader) || QT_CONFIG(texthtmlparser)
            const bool plainText = editor->textFormat() == QQuickTextEdit::PlainText;
#endif
#if QT_CONFIG(textmarkdownreader)
            if (!plainText && isMarkdown) {
                doc->setMarkdown(QString::fromUtf8(data));
            } else
#endif
#if QT_CONFIG(texthtmlparser)
            if (!plainText && isHtml) {
                // If a user loads an HTML file, remember the encoding.
                // If the user then calls save() later, the same encoding will be used.
                encoding = QStringConverter::encodingForHtml(data);
                if (encoding) {
                    QStringDecoder decoder(*encoding);
                    doc->setHtml(decoder(data));
                } else {
                    // fall back to utf8
                    doc->setHtml(QString::fromUtf8(data));
                }
            } else
#endif
            {
                doc->setPlainText(QString::fromUtf8(data));
            }
            setStatus(QQuickTextDocument::Status::Loaded, {});
            qCDebug(lcTextDoc) << editor << "loaded" << filePath
                               << "as" << editor->textFormat() << "detected" << detectedFormat
#if QT_CONFIG(mimetype)
                               << "(file type" << mimeType << ')'
#endif
                    ;
            doc->setModified(false);
            return;
        }
        setStatus(QQuickTextDocument::Status::ReadError,
                  QQuickTextDocument::tr("Failed to read: %1").arg(file.errorString()));
    } else {
        setStatus(QQuickTextDocument::Status::ReadError,
                  QQuickTextDocument::tr("%1 does not exist").arg(filePath));
    }
}

void QQuickTextDocumentPrivate::writeTo(const QUrl &fileUrl)
{
    auto *doc = editor->document();
    if (!doc)
        return;

    const QString filePath = fileUrl.toLocalFile();
    const bool sameUrl = fileUrl == url;
    if (!sameUrl) {
#if QT_CONFIG(mimetype)
        const auto type = QMimeDatabase().mimeTypeForUrl(fileUrl);
        if (type.inherits("text/html"_L1))
            detectedFormat = Qt::RichText;
        else if (type.inherits("text/markdown"_L1))
            detectedFormat = Qt::MarkdownText;
        else
            detectedFormat = Qt::PlainText;
#else
        if (filePath.endsWith(".html"_L1, Qt::CaseInsensitive) ||
            filePath.endsWith(".htm"_L1, Qt::CaseInsensitive))
            detectedFormat = Qt::RichText;
        else if (filePath.endsWith(".md"_L1, Qt::CaseInsensitive) ||
                 filePath.endsWith(".markdown"_L1, Qt::CaseInsensitive))
            detectedFormat = Qt::MarkdownText;
        else
            detectedFormat = Qt::PlainText;
#endif
    }
    QFile file(filePath);
    if (!file.open(QFile::WriteOnly | QFile::Truncate |
                   (detectedFormat == Qt::RichText ? QFile::NotOpen : QFile::Text))) {
        setStatus(QQuickTextDocument::Status::WriteError,
                  QQuickTextDocument::tr("Cannot save: %1").arg(file.errorString()));
        return;
    }
    setStatus(QQuickTextDocument::Status::Saving, {});
    QByteArray raw;

    switch (detectedFormat) {
#if QT_CONFIG(textmarkdownwriter)
    case Qt::MarkdownText:
        raw = doc->toMarkdown().toUtf8();
        break;
#endif
#if QT_CONFIG(texthtmlparser)
    case Qt::RichText:
        if (sameUrl && encoding) {
            QStringEncoder enc(*encoding);
            raw = enc.encode(doc->toHtml());
        } else {
            // default to UTF-8 unless the user is saving the same file as previously loaded
            raw = doc->toHtml().toUtf8();
        }
        break;
#endif
    default:
        raw = doc->toPlainText().toUtf8();
        break;
    }

    file.write(raw);
    file.close();
    setStatus(QQuickTextDocument::Status::Saved, {});
    doc->setModified(false);
}

QTextDocument *QQuickTextDocumentPrivate::document() const
{
    return editor->document();
}

void QQuickTextDocumentPrivate::setDocument(QTextDocument *doc)
{
    Q_Q(QQuickTextDocument);
    QTextDocument *oldDoc = editor->document();
    if (doc == oldDoc)
        return;

    if (oldDoc)
        oldDoc->disconnect(q);
    if (doc) {
        q->connect(doc, &QTextDocument::modificationChanged,
                   q, &QQuickTextDocument::modifiedChanged);
    }
    editor->setDocument(doc);
    emit q->textDocumentChanged();
}

/*!
   Returns a pointer to the QTextDocument object.
*/
QTextDocument *QQuickTextDocument::textDocument() const
{
    Q_D(const QQuickTextDocument);
    return d->document();
}

/*!
    \brief Sets the given \a document.
    \since 6.7

    The caller retains ownership of the document.
*/
void QQuickTextDocument::setTextDocument(QTextDocument *document)
{
    d_func()->setDocument(document);
}

/*!
    \fn void QQuickTextDocument::textDocumentChanged()
    \since 6.7

    This signal is emitted when the underlying QTextDocument is
    replaced with a different instance.

    \sa setTextDocument()
*/

/*!
    \preliminary
    \fn void QQuickTextDocument::sourceChanged()
*/

/*!
    \preliminary
    \fn void QQuickTextDocument::modifiedChanged()
*/

/*!
    \preliminary
    \fn void QQuickTextDocument::statusChanged()
*/

/*!
    \preliminary
    \fn void QQuickTextDocument::errorStringChanged()
*/

/*!
    \fn void QQuickTextDocument::save()
    \since 6.7
    \preliminary

    Saves the contents to the same file and format specified by \l source.

    \note You can save only to a \l {QUrl::isLocalFile()}{file on a mounted filesystem}.

    \sa source, saveAs()
*/

/*!
    \qmlmethod void QtQuick::TextDocument::save()
    \brief Saves the contents to the same file and format specified by \l source.
    \since 6.7
    \preliminary

    \note You can save only to a \l {QUrl::isLocalFile()}{file on a mounted filesystem}.

    \sa source, saveAs()
*/
void QQuickTextDocument::save()
{
    Q_D(QQuickTextDocument);
    d->writeTo(d->url);
}

/*!
    \fn void QQuickTextDocument::saveAs(const QUrl &url)
    \brief Saves the contents to the file and format specified by \a url.
    \since 6.7
    \preliminary

    The file extension in \a url specifies the file format
    (as determined by QMimeDatabase::mimeTypeForUrl()).

    \note You can save only to a \l {QUrl::isLocalFile()}{file on a mounted filesystem}.

    \sa source, save()
*/

/*!
    \qmlmethod void QtQuick::TextDocument::saveAs(url url)
    \brief Saves the contents to the file and format specified by \a url.
    \since 6.7
    \preliminary

    The file extension in \a url specifies the file format
    (as determined by QMimeDatabase::mimeTypeForUrl()).

    \note You can save only to a \l {QUrl::isLocalFile()}{file on a mounted filesystem}.

    \sa source, save()
*/
void QQuickTextDocument::saveAs(const QUrl &url)
{
    Q_D(QQuickTextDocument);
    if (!url.isLocalFile()) {
        d->setStatus(QQuickTextDocument::Status::NonLocalFileError,
                     QQuickTextDocument::tr("Can only save to local files"));
        return;
    }
    d->writeTo(url);

    if (url == d->url)
        return;

    d->url = url;
    emit sourceChanged();
}

QQuickTextImageHandler::QQuickTextImageHandler(QObject *parent)
    : QObject(parent)
{
}

QSizeF QQuickTextImageHandler::intrinsicSize(
        QTextDocument *doc, int, const QTextFormat &format)
{
    if (format.isImageFormat()) {
        QTextImageFormat imageFormat = format.toImageFormat();
        int width = qRound(imageFormat.width());
        const bool hasWidth = imageFormat.hasProperty(QTextFormat::ImageWidth) && width > 0;
        const int height = qRound(imageFormat.height());
        const bool hasHeight = imageFormat.hasProperty(QTextFormat::ImageHeight) && height > 0;
        const auto maxWidth = imageFormat.maximumWidth();
        const bool hasMaxWidth = imageFormat.hasProperty(QTextFormat::ImageMaxWidth) && maxWidth.type() != QTextLength::VariableLength;

        int effectiveMaxWidth = INT_MAX;
        if (hasMaxWidth) {
            if (maxWidth.type() == QTextLength::PercentageLength) {
                effectiveMaxWidth = (doc->pageSize().width() - 2 * doc->documentMargin()) * maxWidth.value(100) / 100;
            } else {
                effectiveMaxWidth = maxWidth.rawValue();
            }

            width = qMin(effectiveMaxWidth, width);
        }

        QSizeF size(width, height);
        if (!hasWidth || !hasHeight) {
            QVariant res = doc->resource(QTextDocument::ImageResource, QUrl(imageFormat.name()));
            QImage image = res.value<QImage>();
            if (image.isNull()) {
                // autotests expect us to reserve a 16x16 space for a "broken image" icon,
                // even though we don't actually display one
                if (!hasWidth)
                    size.setWidth(16);
                if (!hasHeight)
                    size.setHeight(16);
                return size;
            }
            QSize imgSize = image.size();
            if (imgSize.width()  > effectiveMaxWidth) {
                // image is bigger than effectiveMaxWidth, scale it down
                imgSize.setHeight(effectiveMaxWidth * imgSize.height() / (qreal) imgSize.width());
                imgSize.setWidth(effectiveMaxWidth);
            }

            if (!hasWidth) {
                if (!hasHeight)
                    size.setWidth(imgSize.width());
                else
                    size.setWidth(qMin(effectiveMaxWidth, qRound(height * (imgSize.width() / (qreal) imgSize.height()))));
            }
            if (!hasHeight) {
                if (!hasWidth)
                    size.setHeight(imgSize.height());
                else
                    size.setHeight(qRound(width * (imgSize.height() / (qreal) imgSize.width())));
            }
        }
        return size;
    }
    return QSizeF();
}

QT_END_NAMESPACE

#include "moc_qquicktextdocument.cpp"
#include "moc_qquicktextdocument_p.cpp"
