// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "testactivitycommunicator.h"

using namespace QtJniTypes;

static TestActivityCommunicator *s_instance;

TestActivityCommunicator::TestActivityCommunicator(QObject *parent)
    : QObject{ parent },
      m_activity(TestActivity::callStaticMethod<TestActivity>("instance"))
{
    s_instance = this;
    TestActivity::registerNativeMethods({
        Q_JNI_NATIVE_SCOPED_METHOD(jni_onQtQuickViewStatusChanged, TestActivityCommunicator)
    });
}

TestActivityCommunicator::~TestActivityCommunicator()
{
    s_instance = nullptr;
}

TestActivityCommunicator *TestActivityCommunicator::instance()
{
    return s_instance;
}

void TestActivityCommunicator::loadQtQuickView(const QString &qmlUri)
{
    m_activity.callMethod<void>("loadQtQuickView", qmlUri);
}

void TestActivityCommunicator::jni_onQtQuickViewStatusChanged(JNIEnv *, jclass, jint status)
{
    Q_ASSERT(s_instance);
    emit s_instance->qtQuickViewStatusChanged(status);
}
