/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef TESTTYPES_H
#define TESTTYPES_H

#include <QtCore/qobject.h>
#include <QQmlParserStatus>

class SelfRegisteringType : public QObject
{
Q_OBJECT
Q_PROPERTY(int value READ value WRITE setValue);
public:
    SelfRegisteringType();

    int value() const { return m_v; }
    void setValue(int v) { m_v = v; }

    static SelfRegisteringType *me();
    static void clearMe();

private:
    static SelfRegisteringType *m_me;

    int m_v;
};

class CallbackRegisteringType : public QObject
{
Q_OBJECT
Q_PROPERTY(int value READ value WRITE setValue)
public:
    CallbackRegisteringType();

    int value() const { return m_v; }
    void setValue(int v) { if (m_callback) m_callback(this, m_data); m_v = v; }

    typedef void (*callback)(CallbackRegisteringType *, void *);
    static void clearCallback();
    static void registerCallback(callback, void *);

private:
    static callback m_callback;
    static void *m_data;

    int m_v;
};

class CompletionRegisteringType : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
public:
    CompletionRegisteringType();

    virtual void classBegin();
    virtual void componentComplete();

    static CompletionRegisteringType *me();
    static void clearMe();

private:
    static CompletionRegisteringType *m_me;
};

class CompletionCallbackType : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
public:
    CompletionCallbackType();

    virtual void classBegin();
    virtual void componentComplete();

    typedef void (*callback)(CompletionCallbackType *, void *);
    static void clearCallback();
    static void registerCallback(callback, void *);

private:
    static callback m_callback;
    static void *m_data;
};

void registerTypes();

#endif // TESTTYPES_H
