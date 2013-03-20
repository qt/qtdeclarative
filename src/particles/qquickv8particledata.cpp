/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQuick module of the Qt Toolkit.
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

#include "qquickv8particledata_p.h"
#include "qquickparticlesystem_p.h"//for QQuickParticleData
#include <QDebug>

QT_BEGIN_NAMESPACE

/*!
    \qmltype Particle
    \inqmlmodule QtQuick.Particles 2
    \brief Represents particles manipulated by emitters and affectors
    \ingroup qtquick-particles

    Particle elements are always managed internally by the ParticleSystem and cannot be created in QML.
    However, sometimes they are exposed via signals so as to allow arbitrary changes to the particle state
*/

/*!
    \qmlproperty real QtQuick.Particles2::Particle::initialX
    The x coordinate of the particle at the beginning of its lifetime.

    The method of simulation prefers to have the initial values changed, rather
    than determining and changing the value at a given time. Change initial
    values in CustomEmitters instead of the current values.
*/

/*!
    \qmlproperty real QtQuick.Particles2::Particle::initialVX
    The x velocity of the particle at the beginning of its lifetime.

    The method of simulation prefers to have the initial values changed, rather
    than determining and changing the value at a given time. Change initial
    values in CustomEmitters instead of the current values.
*/

/*!
    \qmlproperty real QtQuick.Particles2::Particle::initialAX
    The x acceleration of the particle at the beginning of its lifetime.

    The method of simulation prefers to have the initial values changed, rather
    than determining and changing the value at a given time. Change initial
    values in CustomEmitters instead of the current values.
*/

/*!
    \qmlproperty real QtQuick.Particles2::Particle::initialY
    The y coordinate of the particle at the beginning of its lifetime.

    The method of simulation prefers to have the initial values changed, rather
    than determining and changing the value at a given time. Change initial
    values in CustomEmitters instead of the current values.
*/

/*!
    \qmlproperty real QtQuick.Particles2::Particle::initialVY
    The y velocity of the particle at the beginning of its lifetime.

    The method of simulation prefers to have the initial values changed, rather
    than determining and changing the value at a given time. Change initial
    values in CustomEmitters instead of the current values.
*/

/*!
    \qmlproperty real QtQuick.Particles2::Particle::initialAY
    The y acceleration of the particle at the beginning of its lifetime.

    The method of simulation prefers to have the initial values changed, rather
    than determining and changing the value at a given time. Change initial
    values in CustomEmitters instead of the current values.
*/

/*!
    \qmlproperty real QtQuick.Particles2::Particle::x
    The current x coordinate of the particle.
*/

/*!
    \qmlproperty real QtQuick.Particles2::Particle::vx
    The current x velocity of the particle.
*/

/*!
    \qmlproperty real QtQuick.Particles2::Particle::ax
    The current x acceleration of the particle.
*/

/*!
    \qmlproperty real QtQuick.Particles2::Particle::y
    The current y coordinate of the particle.
*/

/*!
    \qmlproperty real QtQuick.Particles2::Particle::vy
    The current y velocity of the particle.
*/

/*!
    \qmlproperty real QtQuick.Particles2::Particle::ay
    The current y acceleration of the particle.
*/

/*!
    \qmlproperty real QtQuick.Particles2::Particle::t
    The time, in seconds since the beginning of the simulation, that the particle was born.
*/


/*!
    \qmlproperty real QtQuick.Particles2::Particle::startSize
    The size in pixels that the particle image is at the start
    of its life.
*/


/*!
    \qmlproperty real QtQuick.Particles2::Particle::endSize
    The size in pixels that the particle image is at the end
    of its life. If this value is less than 0, then it is
    disregarded and the particle will have its startSize for the
    entire lifetime.
*/

/*!
    \qmlproperty real QtQuick.Particles2::Particle::lifeSpan
    The time in seconds that the particle will live for.
*/

/*!
    \qmlproperty real QtQuick.Particles2::Particle::rotation
    Degrees clockwise that the particle image is rotated at
    the beginning of its life.
*/

/*!
    \qmlproperty real QtQuick.Particles2::Particle::rotationVelocity
    Degrees clockwise per second that the particle image is rotated at while alive.
*/
/*!
    \qmlproperty bool QtQuick.Particles2::Particle::autoRotate
    If autoRotate is true, then the particle's rotation will be
    set so that it faces the direction of travel, plus any
    rotation from the rotation or rotationVelocity properties.
*/

/*!
    \qmlproperty bool QtQuick.Particles2::Particle::update

    Inside an Affector, the changes made to the particle will only be
    applied if update is set to true.
*/
/*!
    \qmlproperty real QtQuick.Particles2::Particle::xDeformationVectorX

    The x component of the deformation vector along the X axis. ImageParticle
    can draw particles across non-square shapes. It will draw the texture rectangle
    across the parallelogram drawn with the x and y deformation vectors.
*/

/*!
    \qmlproperty real QtQuick.Particles2::Particle::yDeformationVectorX

    The y component of the deformation vector along the X axis. ImageParticle
    can draw particles across non-square shapes. It will draw the texture rectangle
    across the parallelogram drawn with the x and y deformation vectors.
*/

/*!
    \qmlproperty real QtQuick.Particles2::Particle::xDeformationVectorY

    The x component of the deformation vector along the X axis. ImageParticle
    can draw particles across non-square shapes. It will draw the texture rectangle
    across the parallelogram drawn with the x and y deformation vectors.
*/

/*!
    \qmlproperty real QtQuick.Particles2::Particle::yDeformationVectorY

    The y component of the deformation vector along the Y axis. ImageParticle
    can draw particles across non-square shapes. It will draw the texture rectangle
    across the parallelogram drawn with the x and y deformation vectors.
*/

/*!
    \qmlproperty real QtQuick.Particles2::Particle::red

    ImageParticle can draw colorized particles. When it does so, red is used
    as the red channel of the color applied to the source image.

    Values are from 0.0 to 1.0.
*/

/*!
    \qmlproperty real QtQuick.Particles2::Particle::green

    ImageParticle can draw colorized particles. When it does so, green is used
    as the green channel of the color applied to the source image.

    Values are from 0.0 to 1.0.
*/

/*!
    \qmlproperty real QtQuick.Particles2::Particle::blue

    ImageParticle can draw colorized particles. When it does so, blue is used
    as the blue channel of the color applied to the source image.

    Values are from 0.0 to 1.0.
*/

/*!
    \qmlproperty real QtQuick.Particles2::Particle::alpha

    ImageParticle can draw colorized particles. When it does so, alpha is used
    as the alpha channel of the color applied to the source image.

    Values are from 0.0 to 1.0.
*/
/*!
    \qmlproperty real QtQuick.Particles2::Particle::lifeLeft
    The time in seconds that the particle has left to live at
    the current point in time.
*/
/*!
    \qmlproperty real QtQuick.Particles2::Particle::currentSize
    The currentSize of the particle, interpolating between startSize and endSize based on the currentTime.
*/



//### Particle data handles are not locked to within certain scopes like QQuickContext2D, but there's no way to reload either...
class QV8ParticleDataResource : public QV8ObjectResource
{
    V8_RESOURCE_TYPE(ParticleDataType)
public:
    QV8ParticleDataResource(QV8Engine *e) : QV8ObjectResource(e) {}
    QQuickParticleData* datum;//TODO: Guard needed?
};

class QV8ParticleDataDeletable : public QV8Engine::Deletable
{
public:
    QV8ParticleDataDeletable(QV8Engine *engine);
    ~QV8ParticleDataDeletable();

    v8::Persistent<v8::Function> constructor;
};

static v8::Handle<v8::Value> particleData_discard(const v8::Arguments &args)
{
    QV8ParticleDataResource *r = v8_resource_cast<QV8ParticleDataResource>(args.This());

    if (!r || !r->datum)
        V8THROW_ERROR("Not a valid ParticleData object");

    r->datum->lifeSpan = 0; //Don't kill(), because it could still be in the middle of being created
    return v8::Undefined();
}

static v8::Handle<v8::Value> particleData_lifeLeft(const v8::Arguments &args)
{
    QV8ParticleDataResource *r = v8_resource_cast<QV8ParticleDataResource>(args.This());
    if (!r || !r->datum)
        V8THROW_ERROR("Not a valid ParticleData object");

    return v8::Number::New(r->datum->lifeLeft());
}

static v8::Handle<v8::Value> particleData_curSize(const v8::Arguments &args)
{
    QV8ParticleDataResource *r = v8_resource_cast<QV8ParticleDataResource>(args.This());
    if (!r || !r->datum)
        V8THROW_ERROR("Not a valid ParticleData object");

    return v8::Number::New(r->datum->curSize());
}
#define COLOR_GETTER_AND_SETTER(VAR, NAME) static v8::Handle<v8::Value> particleData_get_ ## NAME (v8::Local<v8::String>, const v8::AccessorInfo &info) \
{ \
    QV8ParticleDataResource *r = v8_resource_cast<QV8ParticleDataResource>(info.This()); \
    if (!r || !r->datum) \
        V8THROW_ERROR("Not a valid ParticleData object"); \
\
    return v8::Number::New((r->datum->color. VAR )/255.0);\
}\
\
static void particleData_set_ ## NAME (v8::Local<v8::String>, v8::Local<v8::Value> value, const v8::AccessorInfo &info)\
{\
    QV8ParticleDataResource *r = v8_resource_cast<QV8ParticleDataResource>(info.This());\
    if (!r || !r->datum)\
        V8THROW_ERROR_SETTER("Not a valid ParticleData object");\
\
    r->datum->color. VAR = qMin(255, qMax(0, (int)floor(value->NumberValue() * 255.0)));\
}


#define SEMIBOOL_GETTER_AND_SETTER(VARIABLE) static v8::Handle<v8::Value> particleData_get_ ## VARIABLE (v8::Local<v8::String>, const v8::AccessorInfo &info) \
{ \
    QV8ParticleDataResource *r = v8_resource_cast<QV8ParticleDataResource>(info.This()); \
    if (!r || !r->datum) \
        V8THROW_ERROR("Not a valid ParticleData object"); \
\
    return v8::Boolean::New(r->datum-> VARIABLE);\
}\
\
static void particleData_set_ ## VARIABLE (v8::Local<v8::String>, v8::Local<v8::Value> value, const v8::AccessorInfo &info)\
{\
    QV8ParticleDataResource *r = v8_resource_cast<QV8ParticleDataResource>(info.This());\
    if (!r || !r->datum)\
        V8THROW_ERROR_SETTER("Not a valid ParticleData object");\
\
    r->datum-> VARIABLE = value->BooleanValue() ? 1.0 : 0.0;\
}

#define FLOAT_GETTER_AND_SETTER(VARIABLE) static v8::Handle<v8::Value> particleData_get_ ## VARIABLE (v8::Local<v8::String>, const v8::AccessorInfo &info) \
{ \
    QV8ParticleDataResource *r = v8_resource_cast<QV8ParticleDataResource>(info.This()); \
    if (!r || !r->datum) \
        V8THROW_ERROR("Not a valid ParticleData object"); \
\
    return v8::Number::New(r->datum-> VARIABLE);\
}\
\
static void particleData_set_ ## VARIABLE (v8::Local<v8::String>, v8::Local<v8::Value> value, const v8::AccessorInfo &info)\
{\
    QV8ParticleDataResource *r = v8_resource_cast<QV8ParticleDataResource>(info.This());\
    if (!r || !r->datum)\
        V8THROW_ERROR_SETTER("Not a valid ParticleData object");\
\
    r->datum-> VARIABLE = value->NumberValue();\
}

#define FAKE_FLOAT_GETTER_AND_SETTER(VARIABLE, GETTER, SETTER) static v8::Handle<v8::Value> particleData_get_ ## VARIABLE (v8::Local<v8::String>, const v8::AccessorInfo &info) \
{ \
    QV8ParticleDataResource *r = v8_resource_cast<QV8ParticleDataResource>(info.This()); \
    if (!r || !r->datum) \
        V8THROW_ERROR("Not a valid ParticleData object"); \
\
    return v8::Number::New(r->datum-> GETTER ());\
}\
\
static void particleData_set_ ## VARIABLE (v8::Local<v8::String>, v8::Local<v8::Value> value, const v8::AccessorInfo &info)\
{\
    QV8ParticleDataResource *r = v8_resource_cast<QV8ParticleDataResource>(info.This());\
    if (!r || !r->datum)\
        V8THROW_ERROR_SETTER("Not a valid ParticleData object");\
\
    r->datum-> SETTER ( value->NumberValue() );\
}

#define REGISTER_ACCESSOR(FT, ENGINE, VARIABLE, NAME) FT ->PrototypeTemplate()->SetAccessor( v8::String::New( #NAME ), particleData_get_ ## VARIABLE , particleData_set_ ## VARIABLE , v8::External::New(ENGINE))

COLOR_GETTER_AND_SETTER(r, red)
COLOR_GETTER_AND_SETTER(g, green)
COLOR_GETTER_AND_SETTER(b, blue)
COLOR_GETTER_AND_SETTER(a, alpha)
SEMIBOOL_GETTER_AND_SETTER(autoRotate)
SEMIBOOL_GETTER_AND_SETTER(update)
FLOAT_GETTER_AND_SETTER(x)
FLOAT_GETTER_AND_SETTER(y)
FLOAT_GETTER_AND_SETTER(t)
FLOAT_GETTER_AND_SETTER(lifeSpan)
FLOAT_GETTER_AND_SETTER(size)
FLOAT_GETTER_AND_SETTER(endSize)
FLOAT_GETTER_AND_SETTER(vx)
FLOAT_GETTER_AND_SETTER(vy)
FLOAT_GETTER_AND_SETTER(ax)
FLOAT_GETTER_AND_SETTER(ay)
FLOAT_GETTER_AND_SETTER(xx)
FLOAT_GETTER_AND_SETTER(xy)
FLOAT_GETTER_AND_SETTER(yx)
FLOAT_GETTER_AND_SETTER(yy)
FLOAT_GETTER_AND_SETTER(rotation)
FLOAT_GETTER_AND_SETTER(rotationVelocity)
FLOAT_GETTER_AND_SETTER(animIdx)
FLOAT_GETTER_AND_SETTER(frameDuration)
FLOAT_GETTER_AND_SETTER(frameAt)
FLOAT_GETTER_AND_SETTER(frameCount)
FLOAT_GETTER_AND_SETTER(animT)
FLOAT_GETTER_AND_SETTER(r)
FAKE_FLOAT_GETTER_AND_SETTER(curX, curX, setInstantaneousX)
FAKE_FLOAT_GETTER_AND_SETTER(curVX, curVX, setInstantaneousVX)
FAKE_FLOAT_GETTER_AND_SETTER(curAX, curAX, setInstantaneousAX)
FAKE_FLOAT_GETTER_AND_SETTER(curY, curY, setInstantaneousY)
FAKE_FLOAT_GETTER_AND_SETTER(curVY, curVY, setInstantaneousVY)
FAKE_FLOAT_GETTER_AND_SETTER(curAY, curAY, setInstantaneousAY)

QV8ParticleDataDeletable::QV8ParticleDataDeletable(QV8Engine *engine)
{
    v8::HandleScope handle_scope;
    v8::Context::Scope scope(engine->context());

    v8::Local<v8::FunctionTemplate> ft = v8::FunctionTemplate::New();
    ft->InstanceTemplate()->SetHasExternalResource(true);
    ft->PrototypeTemplate()->Set(v8::String::New("discard"), V8FUNCTION(particleData_discard, engine));
    ft->PrototypeTemplate()->Set(v8::String::New("lifeLeft"), V8FUNCTION(particleData_lifeLeft, engine));
    ft->PrototypeTemplate()->Set(v8::String::New("currentSize"), V8FUNCTION(particleData_curSize, engine));
    REGISTER_ACCESSOR(ft, engine, x, initialX);
    REGISTER_ACCESSOR(ft, engine, y, initialY);
    REGISTER_ACCESSOR(ft, engine, t, t);
    REGISTER_ACCESSOR(ft, engine, lifeSpan, lifeSpan);
    REGISTER_ACCESSOR(ft, engine, size, startSize);
    REGISTER_ACCESSOR(ft, engine, endSize, endSize);
    REGISTER_ACCESSOR(ft, engine, vx, initialVX);
    REGISTER_ACCESSOR(ft, engine, vy, initialVY);
    REGISTER_ACCESSOR(ft, engine, ax, initialAX);
    REGISTER_ACCESSOR(ft, engine, ay, initialAY);
    REGISTER_ACCESSOR(ft, engine, xx, xDeformationVectorX);
    REGISTER_ACCESSOR(ft, engine, xy, xDeformationVectorY);
    REGISTER_ACCESSOR(ft, engine, yx, yDeformationVectorX);
    REGISTER_ACCESSOR(ft, engine, yy, yDeformationVectorY);
    REGISTER_ACCESSOR(ft, engine, rotation, rotation);
    REGISTER_ACCESSOR(ft, engine, rotationVelocity, rotationVelocity);
    REGISTER_ACCESSOR(ft, engine, autoRotate, autoRotate);
    REGISTER_ACCESSOR(ft, engine, animIdx, animationIndex);
    REGISTER_ACCESSOR(ft, engine, frameDuration, frameDuration);
    REGISTER_ACCESSOR(ft, engine, frameAt, frameAt);
    REGISTER_ACCESSOR(ft, engine, frameCount, frameCount);
    REGISTER_ACCESSOR(ft, engine, animT, animationT);
    REGISTER_ACCESSOR(ft, engine, r, r);
    REGISTER_ACCESSOR(ft, engine, update, update);
    REGISTER_ACCESSOR(ft, engine, curX, x);
    REGISTER_ACCESSOR(ft, engine, curVX, vx);
    REGISTER_ACCESSOR(ft, engine, curAX, ax);
    REGISTER_ACCESSOR(ft, engine, curY, y);
    REGISTER_ACCESSOR(ft, engine, curVY, vy);
    REGISTER_ACCESSOR(ft, engine, curAY, ay);
    REGISTER_ACCESSOR(ft, engine, red, red);
    REGISTER_ACCESSOR(ft, engine, green, green);
    REGISTER_ACCESSOR(ft, engine, blue, blue);
    REGISTER_ACCESSOR(ft, engine, alpha, alpha);

    constructor = qPersistentNew(ft->GetFunction());
}

QV8ParticleDataDeletable::~QV8ParticleDataDeletable()
{
    qPersistentDispose(constructor);
}

V8_DEFINE_EXTENSION(QV8ParticleDataDeletable, particleV8Data);


QQuickV8ParticleData::QQuickV8ParticleData(QV8Engine* engine, QQuickParticleData* datum)
{
    if (!engine || !datum)
        return;
    v8::HandleScope handle_scope;
    v8::Context::Scope scope(engine->context());

    QV8ParticleDataDeletable *d = particleV8Data(engine);
    m_v8Value = qPersistentNew(d->constructor->NewInstance());
    QV8ParticleDataResource *r = new QV8ParticleDataResource(engine);
    r->datum = datum;
    m_v8Value->SetExternalResource(r);
}

QQuickV8ParticleData::~QQuickV8ParticleData()
{
    qPersistentDispose(m_v8Value);
}

QQmlV8Handle QQuickV8ParticleData::v8Value()
{
    return QQmlV8Handle::fromHandle(m_v8Value);
}

QT_END_NAMESPACE
