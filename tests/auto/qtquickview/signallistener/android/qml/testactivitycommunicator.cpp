// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "testactivitycommunicator.h"
#include <jni.h>

using namespace QtJniTypes;

static TestActivityCommunicator *s_instance;

TestActivityCommunicator::TestActivityCommunicator(QObject *parent)
    : QObject{ parent },
      m_activity(TestActivity::callStaticMethod<TestActivity>("instance")),
      m_view(m_activity.callMethod<TestView>("testView"))
{
    s_instance = this;
    TestActivity::registerNativeMethods({
        Q_JNI_NATIVE_SCOPED_METHOD(onBasicSignal, TestActivityCommunicator),
        Q_JNI_NATIVE_SCOPED_METHOD(onIntSignal, TestActivityCommunicator),
        Q_JNI_NATIVE_SCOPED_METHOD(onBoolSignal, TestActivityCommunicator),
        Q_JNI_NATIVE_SCOPED_METHOD(onDoubleSignal, TestActivityCommunicator),
        Q_JNI_NATIVE_SCOPED_METHOD(onStringSignal, TestActivityCommunicator),
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

void TestActivityCommunicator::onBasicSignal(JNIEnv *, jclass)
{
    Q_ASSERT(s_instance);
    emit s_instance->basicSignal();
}

void TestActivityCommunicator::onIntSignal(JNIEnv *, jclass, Integer value)
{
    Q_ASSERT(s_instance);
    emit s_instance->intSignal(value.callMethod<jint>("intValue"));
}

void TestActivityCommunicator::onBoolSignal(JNIEnv *, jclass, Boolean value)
{
    Q_ASSERT(s_instance);
    emit s_instance->boolSignal(value.callMethod<jboolean>("booleanValue"));
}

void TestActivityCommunicator::onDoubleSignal(JNIEnv *, jclass, Double value)
{
    Q_ASSERT(s_instance);
    emit s_instance->doubleSignal(value.callMethod<jdouble>("doubleValue"));
}

void TestActivityCommunicator::onStringSignal(JNIEnv *, jclass, String value)
{
    Q_ASSERT(s_instance);
    emit s_instance->stringSignal(value.toString());
}
