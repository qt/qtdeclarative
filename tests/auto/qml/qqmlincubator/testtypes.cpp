// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#include "testtypes.h"
#include <QtQml/qqml.h>

SelfRegisteringType *SelfRegisteringType::m_me = nullptr;
SelfRegisteringType::SelfRegisteringType()
: m_v(0)
{
    m_me = this;
}

SelfRegisteringType *SelfRegisteringType::me()
{
    return m_me;
}

void SelfRegisteringType::clearMe()
{
    m_me = nullptr;
}

SelfRegisteringOuterType *SelfRegisteringOuterType::m_me = nullptr;
bool SelfRegisteringOuterType::beenDeleted = false;
SelfRegisteringOuterType::SelfRegisteringOuterType()
: m_v(nullptr)
{
    m_me = this;
    beenDeleted = false;
}

SelfRegisteringOuterType::~SelfRegisteringOuterType()
{
    beenDeleted = true;
}

SelfRegisteringOuterType *SelfRegisteringOuterType::me()
{
    return m_me;
}

CompletionRegisteringType *CompletionRegisteringType::m_me = nullptr;
CompletionRegisteringType::CompletionRegisteringType()
{
}

void CompletionRegisteringType::classBegin()
{
}

void CompletionRegisteringType::componentComplete()
{
    m_me = this;
}

CompletionRegisteringType *CompletionRegisteringType::me()
{
    return m_me;
}

void CompletionRegisteringType::clearMe()
{
    m_me = nullptr;
}

CallbackRegisteringType::callback CallbackRegisteringType::m_callback = nullptr;
void *CallbackRegisteringType::m_data = nullptr;
CallbackRegisteringType::CallbackRegisteringType()
: m_v(0)
{
}

void CallbackRegisteringType::clearCallback()
{
    m_callback = nullptr;
    m_data = nullptr;
}

void CallbackRegisteringType::registerCallback(callback c, void *d)
{
    m_callback = c;
    m_data = d;
}

CompletionCallbackType::callback CompletionCallbackType::m_callback = nullptr;
void *CompletionCallbackType::m_data = nullptr;
CompletionCallbackType::CompletionCallbackType()
{
}

void CompletionCallbackType::classBegin()
{
}

void CompletionCallbackType::componentComplete()
{
    if (m_callback) m_callback(this, m_data);
}

void CompletionCallbackType::clearCallback()
{
    m_callback = nullptr;
    m_data = nullptr;
}

void CompletionCallbackType::registerCallback(callback c, void *d)
{
    m_callback = c;
    m_data = d;
}

void registerTypes()
{
    qmlRegisterType<SelfRegisteringType>("Qt.test", 1,0, "SelfRegistering");
    qmlRegisterType<SelfRegisteringOuterType>("Qt.test", 1,0, "SelfRegisteringOuter");
    qmlRegisterType<CompletionRegisteringType>("Qt.test", 1,0, "CompletionRegistering");
    qmlRegisterType<CallbackRegisteringType>("Qt.test", 1,0, "CallbackRegistering");
    qmlRegisterType<CompletionCallbackType>("Qt.test", 1,0, "CompletionCallback");
}
