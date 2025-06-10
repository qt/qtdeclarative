// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickgraphicsconfiguration_p.h"
#include <QCoreApplication>
#include <rhi/qrhi.h>

QT_BEGIN_NAMESPACE

/*!
    \class QQuickGraphicsConfiguration
    \since 6.0
    \inmodule QtQuick

    \brief QQuickGraphicsConfiguration controls lower level graphics settings
    for the QQuickWindow.

    The QQuickGraphicsConfiguration class is a container for low-level graphics
    settings that can affect how the underlying graphics API, such as Vulkan,
    is initialized by the Qt Quick scene graph. It can also control certain
    aspects of the scene graph renderer.

    \note Setting a QQuickGraphicsConfiguration on a QQuickWindow must happen
    early enough, before the scene graph is initialized for the first time for
    that window. With on-screen windows this means the call must be done before
    invoking show() on the QQuickWindow or QQuickView. With QQuickRenderControl
    the configuration must be finalized before calling
    \l{QQuickRenderControl::initialize()}{initialize()}.

    \section1 Configuration for External Rendering Engines or XR APIs

    When constructing and showing a QQuickWindow that uses Vulkan to render, a
    Vulkan instance (\c VkInstance), a physical device (\c VkPhysicalDevice), a
    device (\c VkDevice) and associated objects (queues, pools) are initialized
    through the Vulkan API. The same is mostly true when using
    QQuickRenderControl to redirect the rendering into a custom render target,
    such as a texture. While QVulkanInstance construction is under the
    application's control then, the initialization of other graphics objects
    happen the same way in QQuickRenderControl::initialize() as with an
    on-screen QQuickWindow.

    For the majority of applications no additional configuration is needed
    because Qt Quick provides reasonable defaults for many low-level graphics
    settings, for example which device extensions to enable.

    This will not alway be sufficient, however. In advanced use cases, when
    integrating direct Vulkan or other graphics API content, or when
    integrating with an external 3D or VR engine, such as, OpenXR, the
    application will want to specify its own set of settings when it comes to
    details, such as which device extensions to enable.

    That is what this class enables. It allows specifying, for example, a list
    of device extensions that is then picked up by the scene graph when using
    Vulkan, or graphics APIs where the concept is applicable. Where some
    concepts are not applicable, the related settings are simply ignored.

    Examples of functions in this category are setDeviceExtensions() and
    preferredInstanceExtensions(). The latter is useful when the application
    manages its own \l QVulkanInstance which is then associated with the
    QQuickWindow via \l QWindow::setVulkanInstance().

    \section1 Qt Quick Scene Graph Renderer Configuration

    Another class of settings are related to the scene graph's renderer. In
    some cases applications may want to control certain behavior,such as using
    the depth buffer when rendering 2D content. In Qt 5 such settings were
    either not controllable at all, or were managed through environment
    variables. In Qt 6, QQuickGraphicsConfiguration provides a new home for
    these settings, while keeping support for the legacy environment variables,
    where applicable.

    An example in this category is setDepthBufferFor2D().

    \section1 Graphics Device Configuration

    When the graphics instance and device objects (for example, the VkInstance
    and VkDevice with Vulkan, the ID3D11Device with Direct 3D, etc.) are
    created by Qt when initializing a QQuickWindow, there are settings which
    applications or libraries will want to control under certain circumstances.

    Before Qt 6.5, some of such settings were available to control via
    environment variables. For example, \c QSG_RHI_DEBUG_LAYER or \c
    QSG_RHI_PREFER_SOFTWARE_RENDERER. These are still available and continue to
    function as before. QQuickGraphicsConfiguration provides C++ setters in
    addition.

    For example, the following main() function opens a QQuickView while
    specifying that the Vulkan validation or Direct3D debug layer should be
    enabled:

    \code
        int main(int argc, char *argv[])
        {
            QGuiApplication app(argc, argv);

            QQuickGraphicsConfiguration config;
            config.setDebugLayer(true);

            QQuickView *view = new QQuickView;
            view->setGraphicsConfiguration(config);

            view->setSource(QUrl::fromLocalFile("myqmlfile.qml"));
            view->show();
            return app.exec();
        }
    \endcode

    \section1 Pipeline Cache Save and Load

    Qt Quick supports storing the graphics/compute pipeline cache to disk, and
    reloading it in subsequent runs of an application. What exactly the
    pipeline cache contains, how lookups work, and what exactly gets
    accelerated all depend on the Qt RHI backend and the underlying native
    graphics API that is used at run time. Different 3D APIs have different
    concepts when it comes to shaders, programs, and pipeline state objects,
    and corresponding cache mechanisms. The high level pipeline cache concept
    here abstracts all this to storing and retrieving a single binary blob to
    and from a file.

    \note Storing the cache on disk can lead to improvements, sometimes
    significant, in subsequent runs of the application.

    When the same shader program and/or pipeline state is encountered as in a
    previous run, a number of operations are likely skipped, leading to faster
    shader and material initialization times, which means startup may become
    faster and lags and "janks" during rendering may be reduced or avoided.

    When running with a graphics API where retrieving and reloading the
    pipeline cache (or shader/program binaries) is not applicable or not
    supported, attempting to use a file to save and load the cache has no
    effect.

    \note In many cases the retrieved data is dependent on and tied to the
    graphics driver (and possibly the exact version of it). Qt performs the
    necessary checks automatically, by storing additional metadata in the
    pipeline cache file. If the data in the file does not match the graphics
    device and driver version at run time, the contents will be ignored
    transparently to the application. It is therefore safe to reference a cache
    that was generated on another device or driver.

    There are exceptions to the driver dependency problem, most notably Direct
    3D 11, where the "pipeline cache" is used only to store the results of
    runtime HLSL->DXBC compilation and is therefore device and vendor
    independent.

    In some cases it may be desirable to improve the very first run of the
    application, by "pre-seeding" the cache. This is possible by shipping the
    cache file saved from a previous run, and referencing it on another machine
    or device. This way, the application or device has the shader
    programs/pipelines that have been encountered before in the run that saved
    the cache file available already during its first run. Shipping and
    deploying the cache file only makes sense if the device and graphics
    drivers are the same on the target system, otherwise the cache file is
    ignored if the device or driver version does not match (with the exception
    of D3D11), as described above.

    Once the cache contents is loaded, there is still a chance that the
    application builds graphics and compute pipelines that have not been
    encountered in previous runs. In this cases the cache is grown, with the
    pipelines / shader programs added to it. If the application also chooses to
    save the contents (perhaps to the same file even), then both the old and
    new pipelines will get stored. Loading from and saving to the same file in
    every run allows an ever growing cache that stores all encountered
    pipelines and shader programs.

    In practice the Qt pipeline cache can be expected to map to the following
    native graphics API features:

    \list

    \li Vulkan -
    \l{https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPipelineCache.html}{VkPipelineCache}
    - Saving the pipeline cache effectively stores the blob retrieved from
    \l{https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkGetPipelineCacheData.html}{vkGetPipelineCacheData},
    with additional metadata to safely identify the device and the driver
    since the pipeline cache blob is dependent on the exact driver.

    \li Metal -
    \l{https://developer.apple.com/documentation/metal/mtlbinaryarchive?language=objc}{MTLBinaryArchive}
    - With pipeline cache saving enabled, Qt stores all render and compute
    pipelines encountered into an MTLBinaryArchive. Saving the pipeline cache
    stores the blob retrieved from the archive, with additional metadata to
    identify the device. \b{Note:} currently MTLBinaryArchive usage is disabled
    on macOS and iOS due to various issues on some hardware and OS versions.

    \li OpenGL - There is no native concept of pipelines, the "pipeline cache"
    stores a collection of program binaries retrieved via
    \l{https://registry.khronos.org/OpenGL-Refpages/gl4/html/glGetProgramBinary.xhtml}{glGetProgramBinary}.
    The program binaries are packaged into a single blob, with additional
    metadata to identify the device, driver, and its version that the binaries
    were retrieved from. Persistent caching of program binaries is not new in
    Qt: Qt 5 already had similar functionality in QOpenGLShaderProgram, see
    \l{QOpenGLShaderProgram::}{addCacheableShaderFromSourceCode()}
    for example. In fact that mechanism is always active in Qt 6 as well when
    using Qt Quick with OpenGL. However, when using the new, graphics API
    independent pipeline cache abstraction provided here, the Qt 5 era program
    binary cache gets automatically disabled, since the same content is
    packaged in the "pipeline cache" now.

    \li Direct 3D 11 - There is no native concept of pipelines or retrieving
    binaries for the second phase compilation (where the vendor independent,
    intermediate bytecode is compiled into the device specific instruction
    set). Drivers will typically employ their own caching system on that level.
    Instead, the Qt Quick "pipeline cache" is used to speed up cases where the
    shaders contain HLSL source code that needs to be compiled into the
    intermediate bytecode format first. This can present significant
    performance improvements in application and libraries that compose shader
    code at run time, because in subsequent runs the potentially expensive,
    uncached calls to
    \l{https://docs.microsoft.com/en-us/windows/win32/api/d3dcompiler/nf-d3dcompiler-d3dcompile}{D3DCompile()}
    can be avoided if the bytecode is already available for the encountered
    HLSL shader. A good example is Qt Quick 3D, where the runtime-generated
    shaders for materials imply having to deal with HLSL source code. Saving
    and reloading the Qt Quick pipeline cache can therefore bring considerable
    improvements in scenes with one or more \l{View3D} items in
    them. A counterexample may be Qt Quick itself: as most built-in shaders for
    2D content ship with DirectX bytecode generated at build time, the cache is
    not going to present any significant improvements.

    \endlist

    All this is independent from the shader processing performed by the
    \l [QtShaderTools]{Qt Shader Tools} module and its command-line tools such
    as \c qsb. As an example, take Vulkan. Having the Vulkan-compatible GLSL
    source code compiled to SPIR-V either at offline or build time (directly
    via qsb or CMake) is good, because the expensive compilation from source
    form is avoided at run time. SPIR-V is however a vendor-independent
    intermediate format. At runtime, when constructing graphics or compute
    pipelines, there is likely another round of compilation happening, this
    time from the intermediate format to the vendor-specific instruction set of
    the GPU (and this may be dependent on certain state in the graphics
    pipeline and the render targets as well). The pipeline cache helps with
    this latter phase.

    \note Many graphics API implementation employ their own persistent disk
    cache transparently to the applications. Using the pipeline cache feature
    of Qt Quick will likely provide improvements in this case, but the gains
    might be smaller.

    Call setPipelineCacheSaveFile() and setPipelineCacheLoadFile() to control
    which files a QQuickWindow or QQuickView saves and loads the pipeline cache
    to/from.

    To get an idea of the effects of enabling disk storage of the pipeline
    cache, enable the most important scenegraph and graphics logs either via
    the environment variable \c{QSG_INFO=1}, or both the
    \c{qt.scenegraph.general} and \c{qt.rhi.general} logging categories. When
    closing the QQuickWindow, there is log message like the following:

    \badcode
      Total time spent on pipeline creation during the lifetime of the QRhi was 123 ms
    \endcode

    This gives an approximate idea of how much time was spent in graphics and
    compute pipeline creation (which may include various stages of shader
    compilation) during the lifetime of the window.

    When loading from a pipeline cache file is enabled, this is confirmed with
    a message:

    \badcode
      Attempting to seed pipeline cache from 'filename'
    \endcode

    Similarly, to check if saving of the cache is successfully enabled, look
    for a message such as this:

    \badcode
      Writing pipeline cache contents to 'filename'
    \endcode

    \section1 The Automatic Pipeline Cache

    When no filename is provided for save and load, the automatic pipeline
    caching strategy is used. This involves storing data to the
    application-specific cache location of the system (\l
    QStandardPaths::CacheLocation).

    This can be disabled by one of the following means:

    \list

    \li Set the application attribute Qt::AA_DisableShaderDiskCache.
    (completely disables the automatic storage)

    \li Set the environment variable QT_DISABLE_SHADER_DISK_CACHE to a non-zero
    value. (completely disables the automatic storage)

    \li Set the environment variable QSG_RHI_DISABLE_SHADER_DISK_CACHE to a
    non-zero value. (completely disables the automatic storage)

    \li Call setAutomaticPiplineCache() with the enable argument set to false.
    (completely disables the automatic storage)

    \li Set a filename by calling setPipelineCacheLoadFile(). (only disables
    loading from the automatic storage, prefering the specified file instead)

    \li Set a filename by calling setPipelineCacheSaveFile(). (only disables
    writing to the automatic storage, prefering the specified file instead)

    \endlist

    The first two are existing mechanisms that are used since Qt 5.9 to control
    the OpenGL program binary cache. For compatibility and familiarity the same
    attribute and environment variable are supported for Qt 6's enhanced
    pipeline cache.

    The automatic pipeline cache uses a single file per application, but a
    different one for each RHI backend (graphics API). This means that changing
    to another graphics API in the next run of the application will not lead to
    losing the pipeline cache generated in the previous run. Applications with
    multiple QQuickWindow instances shown simultaneously may however not
    benefit 100% since the automatic cache can only store the data collected
    from one RHI object at a time. (and with the default \c threaded render
    loop each window has its own RHI as rendering operates independently on
    dedicated threads). To fully benefit from the disk cache in application
    with multiple windows, prefer setting the filename explicitly, per-window
    via setPipelineCacheSaveFile().

    \sa QQuickWindow::setGraphicsConfiguration(), QQuickWindow, QQuickRenderControl
*/

/*!
    Constructs a default QQuickGraphicsConfiguration that does not specify any
    additional settings for the scene graph to take into account.
 */
QQuickGraphicsConfiguration::QQuickGraphicsConfiguration()
    : d(new QQuickGraphicsConfigurationPrivate)
{
}

/*!
    \internal
 */
void QQuickGraphicsConfiguration::detach()
{
    qAtomicDetach(d);
}

/*!
    \internal
 */
QQuickGraphicsConfiguration::QQuickGraphicsConfiguration(const QQuickGraphicsConfiguration &other)
    : d(other.d)
{
    d->ref.ref();
}

/*!
    \internal
 */
QQuickGraphicsConfiguration &QQuickGraphicsConfiguration::operator=(const QQuickGraphicsConfiguration &other)
{
    qAtomicAssign(d, other.d);
    return *this;
}

/*!
    Destructor.
 */
QQuickGraphicsConfiguration::~QQuickGraphicsConfiguration()
{
    if (!d->ref.deref())
        delete d;
}

/*!
    \return the list of Vulkan instance extensions Qt Quick prefers to
    have enabled on the VkInstance.

    In most cases Qt Quick is responsible for creating a QVulkanInstance. This
    function is not relevant then. On the other hand, when using
    QQuickRenderControl in combination with Vulkan-based rendering, it is the
    application's responsibility to create a QVulkanInstance and associate it
    with the (offscreen) QQuickWindow. In this case, it is expected that the
    application queries the list of instance extensions to enable, and passes
    them to QVulkanInstance::setExtensions() before calling
    QVulkanInstance::create().

    \since 6.1
 */
QByteArrayList QQuickGraphicsConfiguration::preferredInstanceExtensions()
{
#if QT_CONFIG(vulkan)
    return QRhiVulkanInitParams::preferredInstanceExtensions();
#else
    return {};
#endif
}

/*!
    Sets the list of additional \a extensions to enable on the graphics device
    (such as, the \c VkDevice).

    When rendering with a graphics API where the concept is not applicable, \a
    extensions will be ignored.

    \note The list specifies additional, extra extensions. Qt Quick always
    enables extensions that are required by the scene graph.
 */
void QQuickGraphicsConfiguration::setDeviceExtensions(const QByteArrayList &extensions)
{
    if (d->deviceExtensions != extensions) {
        detach();
        d->deviceExtensions = extensions;
    }
}

/*!
    \return the list of the requested additional device extensions.
 */
QByteArrayList QQuickGraphicsConfiguration::deviceExtensions() const
{
    return d->deviceExtensions;
}

/*!
    Sets the usage of depth buffer for 2D content to \a enable. When disabled,
    the Qt Quick scene graph never writes into the depth buffer.

    By default the value is true, unless the \c{QSG_NO_DEPTH_BUFFER}
    environment variable is set.

    The default value of true is the most optimal setting for the vast majority
    of scenes. Disabling depth buffer usage reduces the efficiency of the scene
    graph's batching.

    There are cases however, when allowing the 2D content write to the depth
    buffer is not ideal. Consider a 3D scene as an "overlay" on top the 2D
    scene, rendered via Qt Quick 3D using a \l View3D with
    \l{View3D::renderMode}{renderMode} set to \c Overlay. In this case, having
    the depth buffer filled by 2D content can cause unexpected results. This is
    because the way the 2D scene graph renderer generates and handles depth
    values is not necessarily compatible with how a 3D scene works. This may end
    up in depth value clashes, collisions, and unexpected depth test
    failures. Therefore, the robust approach here is to call this function with
    \a enable set to false, and disable depth buffer writes for the 2D content
    in the QQuickWindow.

    \note This flag is not fully identical to setting the
    \c{QSG_NO_DEPTH_BUFFER} environment variable. This flag does not control the
    depth-stencil buffers' presence. It is rather relevant for the rendering
    pipeline. To force not having depth/stencil attachments at all, set
    \c{QSG_NO_DEPTH_BUFFER} and \c{QSG_NO_STENCIL_BUFFER}. Be aware however
    that such a QQuickWindow, and any Item layers in it, may then become
    incompatible with items, such as View3D with certain operating modes,
    because 3D content requires a depth buffer. Calling this function is always
    safe, but can mean that resources, such as depth buffers, are created even
    though they are not actively used.
 */
void QQuickGraphicsConfiguration::setDepthBufferFor2D(bool enable)
{
    if (d->flags.testFlag(QQuickGraphicsConfigurationPrivate::UseDepthBufferFor2D) != enable) {
        detach();
        d->flags.setFlag(QQuickGraphicsConfigurationPrivate::UseDepthBufferFor2D, enable);
    }
}

/*!
    \return true if depth buffer usage is enabled for 2D content.

    By default the value is true, unless the \c{QSG_NO_DEPTH_BUFFER}
    environment variable is set.
 */
bool QQuickGraphicsConfiguration::isDepthBufferEnabledFor2D() const
{
    return d->flags.testFlag(QQuickGraphicsConfigurationPrivate::UseDepthBufferFor2D);
}

/*!
   Enables the graphics API implementation's debug or validation layers, if
   available.

   In practice this is supported with Vulkan and Direct 3D 11, assuming the
   necessary support (validation layers, Windows SDK) is installed and
   available at runtime. When \a enable is true, Qt will attempt to enable the
   standard validation layer on the VkInstance, or set
   \c{D3D11_CREATE_DEVICE_DEBUG} on the graphics device.

   For Metal on \macos, set the environment variable
   \c{METAL_DEVICE_WRAPPER_TYPE=1} instead before launching the application.

   Calling this function with \a enable set to true is equivalent to setting
   the environment variable \c{QSG_RHI_DEBUG_LAYER} to a non-zero value.

   The default value is false.

   \note Enabling debug or validation layers may have a non-insignificant
   performance impact. Shipping applications to production with the flag
   enabled is strongly discouraged.

   \note Be aware that due to differences in the design of the underlying
   graphics APIs, this setting cannot always be a per-QQuickWindow setting,
   even though each QQuickWindow has their own QQuickGraphicsConfiguration.
   With Vulkan in particular, the instance object (VkInstance) is only created
   once and then used by all windows in the application. Therefore, enabling
   the validation layer is something that affects all windows. This also means
   that attempting to enable validation via a window that only gets shown after
   some other windows have already started rendering has no effect with Vulkan.
   Other APIs, such as D3D11, expose the debug layer concept as a per-device
   (ID3D11Device) setting, and so it is controlled on a true per-window basis
   (assuming the scenegraph render loop uses a dedicated graphics
   device/context for each QQuickWindow).

   \since 6.5

   \sa isDebugLayerEnabled()
 */
void QQuickGraphicsConfiguration::setDebugLayer(bool enable)
{
    if (d->flags.testFlag(QQuickGraphicsConfigurationPrivate::EnableDebugLayer) != enable) {
        detach();
        d->flags.setFlag(QQuickGraphicsConfigurationPrivate::EnableDebugLayer, enable);
    }
}

/*!
    \return true if the debug/validation layers are to be enabled.

    By default the value is false.

    \sa setDebugLayer()
 */
bool QQuickGraphicsConfiguration::isDebugLayerEnabled() const
{
    return d->flags.testFlag(QQuickGraphicsConfigurationPrivate::EnableDebugLayer);
}

/*!
    Where applicable, \a enable controls inserting debug markers and object
    names into the graphics command stream.

    Some frameworks, such as Qt Quick 3D, have the ability to annotate the
    graphics objects they create (buffers, textures) with names and also
    indicate the beginning and end of render passes in the command buffer. These
    are then visible in frame captures made with tools like
    \l{https://renderdoc.org/}{RenderDoc} or XCode.

    Graphics APIs where this can be expected to be supported are Vulkan (if
    VK_EXT_debug_utils is available), Direct 3D 11, and Metal.

    Calling this function with \a enable set to true is equivalent to setting
    the environment variable \c{QSG_RHI_PROFILE} to a non-zero
    value.

    The default value is false.

    \note Enabling debug markers may have a performance impact. Shipping
    applications to production with the flag enabled is not recommended.

    \since 6.5

    \sa isDebugMarkersEnabled()
 */
void QQuickGraphicsConfiguration::setDebugMarkers(bool enable)
{
    if (d->flags.testFlag(QQuickGraphicsConfigurationPrivate::EnableDebugMarkers) != enable) {
        detach();
        d->flags.setFlag(QQuickGraphicsConfigurationPrivate::EnableDebugMarkers, enable);
    }
}

/*!
    \return true if debug markers are enabled.

    By default the value is false.

    \sa setDebugMarkers()
 */
bool QQuickGraphicsConfiguration::isDebugMarkersEnabled() const
{
    return d->flags.testFlag(QQuickGraphicsConfigurationPrivate::EnableDebugMarkers);
}

/*!
    When enabled, GPU timing data is collected from command buffers on
    platforms and 3D APIs where this is supported. This data is then printed in
    the renderer logs that can be enabled via \c{QSG_RENDER_TIMING} environment
    variable or logging categories such as \c{qt.scenegraph.time.renderloop},
    and may also be made visible to other modules, such as Qt Quick 3D's
    \l DebugView item.

    By default this is disabled, because collecting the data may involve
    additional work, such as inserting timestamp queries in the command stream,
    depending on the underlying graphics API. To enable, either call this
    function with \a enable set to true, or set the \c{QSG_RHI_PROFILE}
    environment variable to a non-zero value.

    Graphics APIs where this can be expected to be supported are Direct 3D 11,
    Direct 3D 12, Vulkan (as long as the underlying Vulkan implementation
    supports timestamp queries), Metal, and OpenGL with a core or compatibility
    profile context for version 3.3 or newer. Timestamps are not supported with
    OpenGL ES.

    \since 6.6

    \sa timestampsEnabled(), setDebugMarkers()
 */
void QQuickGraphicsConfiguration::setTimestamps(bool enable)
{
    if (d->flags.testFlag(QQuickGraphicsConfigurationPrivate::EnableTimestamps) != enable) {
        detach();
        d->flags.setFlag(QQuickGraphicsConfigurationPrivate::EnableTimestamps, enable);
    }
}

/*!
    \return true if GPU timing collection is enabled.

    By default the value is false.

    \since 6.6
    \sa setTimestamps()
 */
bool QQuickGraphicsConfiguration::timestampsEnabled() const
{
    return d->flags.testFlag(QQuickGraphicsConfigurationPrivate::EnableTimestamps);
}

/*!
    Requests choosing an adapter or physical device that uses software-based
    rasterization. Applicable only when the underlying API has support for
    enumerating adapters (for example, Direct 3D or Vulkan), and is ignored
    otherwise.

    If the graphics API implementation has no such graphics adapter or physical
    device available, the request is ignored. With Direct 3D it can be expected
    that a
    \l{https://docs.microsoft.com/en-us/windows/win32/direct3darticles/directx-warp}{WARP}-based
    rasterizer is always available. With Vulkan, the flag only has an effect if
    Mesa's \c lavapipe, or some other physical device reporting
    \c{VK_PHYSICAL_DEVICE_TYPE_CPU} is available.

    Calling this function with \a enable set to true is equivalent to setting
    the environment variable \c{QSG_RHI_PREFER_SOFTWARE_RENDERER} to a non-zero
    value.

    The default value is false.

    \since 6.5

    \sa prefersSoftwareDevice()
 */
void QQuickGraphicsConfiguration::setPreferSoftwareDevice(bool enable)
{
    if (d->flags.testFlag(QQuickGraphicsConfigurationPrivate::PreferSoftwareDevice) != enable) {
        detach();
        d->flags.setFlag(QQuickGraphicsConfigurationPrivate::PreferSoftwareDevice, enable);
    }
}

/*!
    \return true if a software rasterizer-based graphics device is prioritized.

    By default the value is false.

    \sa setPreferSoftwareDevice()
 */
bool QQuickGraphicsConfiguration::prefersSoftwareDevice() const
{
    return d->flags.testFlag(QQuickGraphicsConfigurationPrivate::PreferSoftwareDevice);
}

/*!
    Changes the usage of the automatic pipeline cache based on \a enable.

    The default value is true, unless certain application attributes or
    environment variables are set. See \l{The Automatic Pipeline Cache} for
    more information.

    \since 6.5

    \sa isAutomaticPipelineCacheEnabled()
 */
void QQuickGraphicsConfiguration::setAutomaticPipelineCache(bool enable)
{
    if (d->flags.testFlag(QQuickGraphicsConfigurationPrivate::AutoPipelineCache) != enable) {
        detach();
        d->flags.setFlag(QQuickGraphicsConfigurationPrivate::AutoPipelineCache, enable);
    }
}

/*!
    \return true if the automatic pipeline cache is enabled.

    By default this is true, unless certain application attributes or
    environment variables are set. See \l{The Automatic Pipeline Cache} for
    more information.

    \since 6.5

    \sa setAutomaticPipelineCache()
 */
bool QQuickGraphicsConfiguration::isAutomaticPipelineCacheEnabled() const
{
    return d->flags.testFlag(QQuickGraphicsConfigurationPrivate::AutoPipelineCache);
}

/*!
    Sets the \a filename where the QQuickWindow is expected to store its
    graphics/compute pipeline cache contents. The default value is empty, which
    means pipeline cache loading is disabled.

    See \l{Pipeline Cache Save and Load} for a discussion on pipeline caches.

    Persistently storing the pipeline cache can lead to performance
    improvements in future runs of the application since expensive shader
    compilation and pipeline construction steps may be avoided.

    If and when the writing of the file happens is not defined. It will likely
    happen at some point when tearing down the scenegraph due to closing the
    window. Therefore, applications should not assume availability of the file
    until the QQuickWindow is fully destructed. QQuickGraphicsConfiguration
    only stores the filename, it does not perform any actual I/O and graphics
    operations on its own.

    When running with a graphics API where retrieving the pipeline cache (or
    shader/program binaries) is not applicable or not supported, calling this
    function has no effect.

    Calling this function is mostly equivalent to setting the environment
    variable \c{QSG_RHI_PIPELINE_CACHE_SAVE} to \a filename, with one important
    difference: this function controls the pipeline cache storage for the
    associated QQuickWindow only. Applications with multiple QQuickWindow or
    QQuickView instances can therefore store and later reload the cache contents
    via files dedicated to each window. The environment variable does not allow
    this.

    \since 6.5

    \sa pipelineCacheLoadFile(), pipelineCacheSaveFile()
 */
void QQuickGraphicsConfiguration::setPipelineCacheSaveFile(const QString &filename)
{
    if (d->pipelineCacheSaveFile != filename) {
        detach();
        d->pipelineCacheSaveFile = filename;
    }
}

/*!
    \return the currently set filename for storing the pipeline cache.

    By default the value is an empty string.
 */
QString QQuickGraphicsConfiguration::pipelineCacheSaveFile() const
{
    return d->pipelineCacheSaveFile;
}

/*!
    Sets the \a filename where the QQuickWindow is expected to load the initial
    contents of its graphics/compute pipeline cache from. The default value is
    empty, which means pipeline cache loading is disabled.

    See \l{Pipeline Cache Save and Load} for a discussion on pipeline caches.

    Persistently storing the pipeline cache can lead to performance
    improvements in future runs of the application since expensive shader
    compilation and pipeline construction steps may be avoided.

    If and when the loading of the file's contents happens is not defined, apart
    from that it will happen at some point during the initialization of the
    scenegraph of the QQuickWindow. Therefore, the file must continue to exist
    after calling this function. QQuickGraphicsConfiguration only stores the
    filename, it cannot perform any actual I/O and graphics operations on its
    own. The real work is going to happen later on, possibly on another thread.

    When running with a graphics API where retrieving and reloading the
    pipeline cache (or shader/program binaries) is not applicable or not
    supported, calling this function has no effect.

    Calling this function is mostly equivalent to setting the environment
    variable \c{QSG_RHI_PIPELINE_CACHE_LOAD} to \a filename, with one important
    difference: this function controls the pipeline cache storage for the
    associated QQuickWindow only. Applications with multiple QQuickWindow or
    QQuickView instances can therefore store and later reload the cache contents
    via files dedicated to each window. The environment variable does not allow
    this.

    \note If the data in the file does not match the graphics device and driver
    version at run time, the contents will be ignored, transparently to the
    application. This applies to a number of graphics APIs, and the necessary
    checks are taken care of by Qt. There are exceptions, most notably Direct
    3D 11, where the "pipeline cache" is used only to store the results of
    runtime HLSL->DXBC compilation and is therefore device and vendor
    independent.

    \warning The serialized pipeline cache data is assumed to be trusted
    content. Application developers are advised to never pass in data from
    untrusted sources.

    \since 6.5

    \sa pipelineCacheLoadFile(), setPipelineCacheSaveFile()
 */
void QQuickGraphicsConfiguration::setPipelineCacheLoadFile(const QString &filename)
{
    if (d->pipelineCacheLoadFile != filename) {
        detach();
        d->pipelineCacheLoadFile = filename;
    }
}

/*!
    \return the currently set filename for loading the pipeline cache.

    By default the value is an empty string.
 */
QString QQuickGraphicsConfiguration::pipelineCacheLoadFile() const
{
    return d->pipelineCacheLoadFile;
}

QQuickGraphicsConfigurationPrivate::QQuickGraphicsConfigurationPrivate()
    : ref(1)
{
    // Defaults based on env.vars. NB! many of these variables are documented
    // and should be considered (semi-)public API. Changing the env.var. names
    // is therefore not allowed.

    flags = {};

    static const bool useDepthBufferFor2D = qEnvironmentVariableIsEmpty("QSG_NO_DEPTH_BUFFER");
    if (useDepthBufferFor2D)
        flags |= UseDepthBufferFor2D;

    static const bool enableDebugLayer = qEnvironmentVariableIntValue("QSG_RHI_DEBUG_LAYER");
    if (enableDebugLayer)
        flags |= EnableDebugLayer;

    static const bool enableProfilingRelated = qEnvironmentVariableIntValue("QSG_RHI_PROFILE");
    if (enableProfilingRelated)
        flags |= EnableDebugMarkers | EnableTimestamps;

    static const bool preferSoftwareDevice = qEnvironmentVariableIntValue("QSG_RHI_PREFER_SOFTWARE_RENDERER");
    if (preferSoftwareDevice)
        flags |= PreferSoftwareDevice;

    // here take the existing QOpenGL disk cache attribute and env.var. into account as well
    static const bool autoPipelineCache = !QCoreApplication::instance()->testAttribute(Qt::AA_DisableShaderDiskCache)
            && !qEnvironmentVariableIntValue("QT_DISABLE_SHADER_DISK_CACHE")
            && !qEnvironmentVariableIntValue("QSG_RHI_DISABLE_DISK_CACHE");
    if (autoPipelineCache)
        flags |= AutoPipelineCache;

    static const QString pipelineCacheSaveFileEnv = qEnvironmentVariable("QSG_RHI_PIPELINE_CACHE_SAVE");
    pipelineCacheSaveFile = pipelineCacheSaveFileEnv;

    static const QString pipelineCacheLoadFileEnv = qEnvironmentVariable("QSG_RHI_PIPELINE_CACHE_LOAD");
    pipelineCacheLoadFile = pipelineCacheLoadFileEnv;
}

QQuickGraphicsConfigurationPrivate::QQuickGraphicsConfigurationPrivate(const QQuickGraphicsConfigurationPrivate &other)
    : ref(1),
      deviceExtensions(other.deviceExtensions),
      flags(other.flags),
      pipelineCacheSaveFile(other.pipelineCacheSaveFile),
      pipelineCacheLoadFile(other.pipelineCacheLoadFile)
{
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QQuickGraphicsConfiguration &config)
{
    QDebugStateSaver saver(dbg);
    const QQuickGraphicsConfigurationPrivate *cd = QQuickGraphicsConfigurationPrivate::get(&config);
    dbg.nospace() << "QQuickGraphicsConfiguration("
                  << "flags=0x" << Qt::hex << cd->flags << Qt::dec
                  << " flag-isDepthBufferEnabledFor2D=" << config.isDepthBufferEnabledFor2D()
                  << " flag-isDebugLayerEnabled=" << config.isDebugLayerEnabled()
                  << " flag-isDebugMarkersEnabled=" << config.isDebugMarkersEnabled()
                  << " flag-prefersSoftwareDevice=" << config.prefersSoftwareDevice()
                  << " flag-isAutomaticPipelineCacheEnabled=" << config.isAutomaticPipelineCacheEnabled()
                  << " pipelineCacheSaveFile=" << cd->pipelineCacheSaveFile
                  << " piplineCacheLoadFile=" << cd->pipelineCacheLoadFile
                  << " extra-device-extension-requests=" << cd->deviceExtensions
                  << ')';
    return dbg;
}
#endif // QT_NO_DEBUG_STREAM

QT_END_NAMESPACE
