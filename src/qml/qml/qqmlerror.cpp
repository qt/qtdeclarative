// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlerror.h"
#include "qqmlfile.h"
#include <private/qqmljsdiagnosticmessage_p.h>

#include <QtCore/qdebug.h>
#include <QtCore/qfile.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qvector.h>

#include <QtCore/qobject.h>
#include <QtCore/qpointer.h>

QT_BEGIN_NAMESPACE

/*!
    \class QQmlError
    \since 5.0
    \inmodule QtQml
    \brief The QQmlError class encapsulates a QML error.

    QQmlError includes a textual description of the error, as well
    as location information (the file, line, and column). The toString()
    method creates a single-line, human-readable string containing all of
    this information, for example:
    \code
    file:///home/user/test.qml:7:8: Invalid property assignment: double expected
    \endcode

    You can use qDebug(), qInfo(), or qWarning() to output errors to the console.
    This method will attempt to open the file indicated by the error
    and include additional contextual information.
    \code
    file:///home/user/test.qml:7:8: Invalid property assignment: double expected
            y: "hello"
               ^
    \endcode

    \sa QQuickView::errors(), QQmlComponent::errors()
*/
class QQmlErrorPrivate
{
public:
    QUrl url;
    QPointer<QObject> object;
    QString message;
    QtMsgType type = QtWarningMsg;
    int line = -1;
    int column = -1;

    friend bool operator==(const QQmlErrorPrivate &a, const QQmlErrorPrivate &b)
    {
        return a.url == b.url && a.object == b.object && a.message == b.message
                && a.type == b.type && a.line == b.line && a.column == b.column;
    }
};

/*!
    Creates an empty error object.
*/
QQmlError::QQmlError()
: d(nullptr)
{
}

/*!
    Creates a copy of \a other.
*/
QQmlError::QQmlError(const QQmlError &other)
: d(nullptr)
{
    *this = other;
}

/*!
    Assigns \a other to this error object.
*/
QQmlError &QQmlError::operator=(const QQmlError &other)
{
    if (!other.d) {
        delete d;
        d = nullptr;
    } else {
        if (!d)
            d = new QQmlErrorPrivate;
        d->url = other.d->url;
        d->message = other.d->message;
        d->line = other.d->line;
        d->column = other.d->column;
        d->object = other.d->object;
        d->type = other.d->type;
    }
    return *this;
}

/*!
    \internal
*/
QQmlError::~QQmlError()
{
    delete d; d = nullptr;
}

/*!
    Returns true if this error is valid, otherwise false.
*/
bool QQmlError::isValid() const
{
    return d != nullptr;
}

/*!
    Returns the url for the file that caused this error.
*/
QUrl QQmlError::url() const
{
    if (d)
        return d->url;
    return QUrl();
}

/*!
    Sets the \a url for the file that caused this error.
*/
void QQmlError::setUrl(const QUrl &url)
{
    if (!d)
        d = new QQmlErrorPrivate;
    d->url = url;
}

/*!
    Returns the error description.
*/
QString QQmlError::description() const
{
    if (d)
        return d->message;
    return QString();
}

/*!
    Sets the error \a description.
*/
void QQmlError::setDescription(const QString &description)
{
    if (!d)
        d = new QQmlErrorPrivate;
    d->message = description;
}

/*!
    Returns the error line number.
*/
int QQmlError::line() const
{
    if (d)
        return d->line;
    return -1;
}

/*!
    Sets the error \a line number.
*/
void QQmlError::setLine(int line)
{
    if (!d)
        d = new QQmlErrorPrivate;
    d->line = line;
}

/*!
    Returns the error column number.
*/
int QQmlError::column() const
{
    if (d)
        return d->column;
    return -1;
}

/*!
    Sets the error \a column number.
*/
void QQmlError::setColumn(int column)
{
    if (!d)
        d = new QQmlErrorPrivate;
    d->column = column;
}

/*!
    Returns the nearest object where this error occurred.
    Exceptions in bound property expressions set this to the object
    to which the property belongs. It will be 0 for all
    other exceptions.
 */
QObject *QQmlError::object() const
{
    if (d)
        return d->object;
    return nullptr;
}

/*!
    Sets the nearest \a object where this error occurred.
 */
void QQmlError::setObject(QObject *object)
{
    if (!d)
        d = new QQmlErrorPrivate;
    d->object = object;
}

/*!
    \since 5.9

    Returns the message type.
 */
QtMsgType QQmlError::messageType() const
{
    if (d)
        return d->type;
    return QtMsgType::QtWarningMsg;
}

/*!
    \since 5.9

    Sets the \a messageType for this message. The message type determines which
    QDebug handlers are responsible for receiving the message.
 */
void QQmlError::setMessageType(QtMsgType messageType)
{
    if (!d)
        d = new QQmlErrorPrivate;
    d->type = messageType;
}

/*!
    Returns the error as a human readable string.
*/
QString QQmlError::toString() const
{
    QString rv;

    QUrl u(url());
    int l(line());

    if (u.isEmpty() || (u.isLocalFile() && u.path().isEmpty()))
        rv += QLatin1String("<Unknown File>");
    else
        rv += u.toString();

    if (l != -1) {
        rv += QLatin1Char(':') + QString::number(l);

        int c(column());
        if (c != -1)
            rv += QLatin1Char(':') + QString::number(c);
    }

    rv += QLatin1String(": ") + description();

    return rv;
}

bool operator==(const QQmlError &a, const QQmlError &b)
{
    return a.d == b.d || (a.d && b.d && *a.d == *b.d);
}

/*!
    \relates QQmlError
    \fn QDebug operator<<(QDebug debug, const QQmlError &error)

    Outputs a human readable version of \a error to \a debug.
*/

QDebug operator<<(QDebug debug, const QQmlError &error)
{
    debug << qPrintable(error.toString());

    QUrl url = error.url();

    if (error.line() > 0 && (url.scheme() == QLatin1String("file") || url.scheme() == QLatin1String("qrc"))) {
        QString file = QQmlFile::urlToLocalFileOrQrc(url);
        QFile f(file);
        if (f.open(QIODevice::ReadOnly)) {
            QByteArray data = f.readAll();
            QTextStream stream(data, QIODevice::ReadOnly);
            const QString code = stream.readAll();
            const auto lines = QStringView{code}.split(QLatin1Char('\n'));

            if (lines.size() >= error.line()) {
                const QStringView &line = lines.at(error.line() - 1);
                debug << "\n    " << line.toLocal8Bit().constData();

                if(error.column() > 0) {
                    int column = qMax(0, error.column() - 1);
                    column = qMin(column, line.size());

                    QByteArray ind;
                    ind.reserve(column);
                    for (int i = 0; i < column; ++i) {
                        const QChar ch = line.at(i);
                        if (ch.isSpace())
                            ind.append(ch.unicode());
                        else
                            ind.append(' ');
                    }
                    ind.append('^');
                    debug << "\n    " << ind.constData();
                }
            }
        }
    }
    return debug;
}

QT_END_NAMESPACE
