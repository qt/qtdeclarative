/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
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
**
****************************************************************************/

#include "qiopipe.h"

#include <QtCore/private/qobject_p.h>
#include <QtCore/qpointer.h>

#include <QDebug>

#include <memory>

QT_BEGIN_NAMESPACE

class QPipeEndPoint : public QIODevice
{
    Q_OBJECT

public:
    bool isSequential() const override;
    qint64 bytesAvailable() const override;

    void setRemoteEndPoint(QPipeEndPoint *other);

protected:
    qint64 readData(char *data, qint64 maxlen) override;
    qint64 writeData(const char *data, qint64 len) override;

private:
    QByteArray m_buffer;
    QPointer<QPipeEndPoint> m_remoteEndPoint;
};

class QIOPipePrivate final : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QIOPipe)
public:
    QIOPipePrivate();
    ~QIOPipePrivate();

    std::unique_ptr<QPipeEndPoint> end1;
    std::unique_ptr<QPipeEndPoint> end2;
};

QIOPipe::QIOPipe(QObject *parent) : QObject(*(new QIOPipePrivate()), parent) { }

bool QIOPipe::open(QIODevice::OpenMode mode)
{
    Q_D(QIOPipe);

    if (!d->end1->open(mode))
        return false;
    switch (mode & QIODevice::ReadWrite) {
    case QIODevice::WriteOnly:
    case QIODevice::ReadOnly:
        return d->end2->open(mode ^ QIODevice::ReadWrite);
    default:
        return d->end2->open(mode);
    }
}

QIODevice *QIOPipe::end1() const
{
    Q_D(const QIOPipe);
    return d->end1.get();
}

QIODevice *QIOPipe::end2() const
{
    Q_D(const QIOPipe);
    return d->end2.get();
}

QIOPipePrivate::QIOPipePrivate()
    : end1(std::make_unique<QPipeEndPoint>()), end2(std::make_unique<QPipeEndPoint>())
{
    end1->setRemoteEndPoint(end2.get());
    end2->setRemoteEndPoint(end1.get());
}

QIOPipePrivate::~QIOPipePrivate() { }

bool QPipeEndPoint::isSequential() const
{
    return true;
}

qint64 QPipeEndPoint::bytesAvailable() const
{
    return m_buffer.length() + QIODevice::bytesAvailable();
}

void QPipeEndPoint::setRemoteEndPoint(QPipeEndPoint *other)
{
    m_remoteEndPoint = other;
}

qint64 QPipeEndPoint::readData(char *data, qint64 maxlen)
{
    maxlen = qMin(maxlen, static_cast<qint64>(m_buffer.length()));
    if (maxlen <= 0)
        return 0;

    Q_ASSERT(maxlen > 0);
    memcpy(data, m_buffer.data(), static_cast<size_t>(maxlen));
    m_buffer = m_buffer.mid(maxlen);
    return maxlen;
}

qint64 QPipeEndPoint::writeData(const char *data, qint64 len)
{
    if (!m_remoteEndPoint)
        return -1;

    if (len <= 0)
        return 0;

    QByteArray &buffer = m_remoteEndPoint->m_buffer;
    const qint64 prevLen = buffer.length();
    Q_ASSERT(prevLen >= 0);
    len = qMin(len, std::numeric_limits<int>::max() - prevLen);

    if (len == 0)
        return 0;

    Q_ASSERT(len > 0);
    Q_ASSERT(prevLen + len > 0);
    Q_ASSERT(prevLen + len <= std::numeric_limits<int>::max());

    buffer.resize(prevLen + len);
    memcpy(buffer.data() + prevLen, data, static_cast<size_t>(len));
    emit bytesWritten(len);
    emit m_remoteEndPoint->readyRead();
    return len;
}

QT_END_NAMESPACE

#include <qiopipe.moc>
