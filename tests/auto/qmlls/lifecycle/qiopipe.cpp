// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

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
    return m_buffer.size() + QIODevice::bytesAvailable();
}

void QPipeEndPoint::setRemoteEndPoint(QPipeEndPoint *other)
{
    m_remoteEndPoint = other;
}

qint64 QPipeEndPoint::readData(char *data, qint64 maxlen)
{
    maxlen = qMin(maxlen, static_cast<qint64>(m_buffer.size()));
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
    const qint64 prevLen = buffer.size();
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
