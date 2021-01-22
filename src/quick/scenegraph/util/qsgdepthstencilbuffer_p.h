/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QSGDEPTHSTENCILBUFFER_P_H
#define QSGDEPTHSTENCILBUFFER_P_H

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

#include <QtCore/qsize.h>
#include <QtGui/private/qopenglcontext_p.h>
#include <QtGui/private/qopenglextensions_p.h>
#include <QtCore/qsharedpointer.h>
#include <QtCore/qhash.h>

QT_BEGIN_NAMESPACE

class QSGDepthStencilBufferManager;

class QSGDepthStencilBuffer
{
public:
    enum Attachment
    {
        NoAttachment = 0x00,
        DepthAttachment = 0x01,
        StencilAttachment = 0x02
    };
    Q_DECLARE_FLAGS(Attachments, Attachment)

    struct Format
    {
        QSize size;
        int samples;
        QSGDepthStencilBuffer::Attachments attachments;
        bool operator == (const Format &other) const;
    };

    QSGDepthStencilBuffer(QOpenGLContext *context, const Format &format);
    virtual ~QSGDepthStencilBuffer();

    // Attaches this depth stencil buffer to the currently bound FBO.
    void attach();
    // Detaches this depth stencil buffer from the currently bound FBO.
    void detach();

    QSize size() const { return m_format.size; }
    int samples() const { return m_format.samples; }
    Attachments attachments() const { return m_format.attachments; }

protected:
    virtual void free() = 0;

    QOpenGLExtensions m_functions;
    QSGDepthStencilBufferManager *m_manager;
    Format m_format;
    GLuint m_depthBuffer;
    GLuint m_stencilBuffer;

    friend class QSGDepthStencilBufferManager;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QSGDepthStencilBuffer::Attachments)

inline bool QSGDepthStencilBuffer::Format::operator == (const Format &other) const
{
    return size == other.size && samples == other.samples && attachments == other.attachments;
}


class QSGDefaultDepthStencilBuffer : public QSGDepthStencilBuffer
{
public:
    QSGDefaultDepthStencilBuffer(QOpenGLContext *context, const Format &format);
    virtual ~QSGDefaultDepthStencilBuffer();

protected:
    void free() override;
};


class QSGDepthStencilBufferManager
{
public:
    QSGDepthStencilBufferManager(QOpenGLContext *ctx) : m_context(ctx) { }
    ~QSGDepthStencilBufferManager();
    QOpenGLContext *context() const { return m_context; }
    QSharedPointer<QSGDepthStencilBuffer> bufferForFormat(const QSGDepthStencilBuffer::Format &fmt);
    void insertBuffer(const QSharedPointer<QSGDepthStencilBuffer> &buffer);

private:
    typedef QHash<QSGDepthStencilBuffer::Format, QWeakPointer<QSGDepthStencilBuffer> > Hash;
    QOpenGLContext *m_context;
    Hash m_buffers;

    friend class QSGDepthStencilBuffer;
};

extern uint qHash(const QSGDepthStencilBuffer::Format &format);

QT_END_NAMESPACE

#endif
