// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
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

class SelfRegisteringOuterType : public QObject
{
Q_OBJECT
Q_PROPERTY(QObject* value READ value WRITE setValue);
public:
    SelfRegisteringOuterType();
    ~SelfRegisteringOuterType();

    static bool beenDeleted;

    QObject *value() const { return m_v; }
    void setValue(QObject *v) { m_v = v; }

    static SelfRegisteringOuterType *me();

private:
    static SelfRegisteringOuterType *m_me;

    QObject *m_v;
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

    void classBegin() override;
    void componentComplete() override;

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

    void classBegin() override;
    void componentComplete() override;

    typedef void (*callback)(CompletionCallbackType *, void *);
    static void clearCallback();
    static void registerCallback(callback, void *);

private:
    static callback m_callback;
    static void *m_data;
};

void registerTypes();

#endif // TESTTYPES_H
