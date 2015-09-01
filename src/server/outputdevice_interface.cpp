/********************************************************************
Copyright 2014  Martin Gräßlin <mgraesslin@kde.org>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) version 3, or any
later version accepted by the membership of KDE e.V. (or its
successor approved by the membership of KDE e.V.), which shall
act as a proxy defined in Section 6 of version 3 of the license.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/
#include "outputdevice_interface.h"
#include "global_p.h"
#include "display.h"

#include <wayland-server.h>
#include "wayland-org_kde_kwin_outputdevice-server-protocol.h"

namespace KWayland
{
namespace Server
{

static const quint32 s_version = 2;

class OutputDeviceInterface::Private : public Global::Private
{
public:
    struct ResourceData {
        wl_resource *resource;
        uint32_t version;
    };
    Private(OutputDeviceInterface *q, Display *d);
    void sendMode(wl_resource *resource, const Mode &mode);
    void sendDone(const ResourceData &data);
    void updateGeometry();
    void updateScale();

    QSize physicalSize;
    QPoint globalPosition;
    QString manufacturer = QStringLiteral("org.kde.kwin");
    QString model = QStringLiteral("none");
    int scale = 1;
    SubPixel subPixel = SubPixel::Unknown;
    Transform transform = Transform::Normal;
    QList<Mode> modes;
    QList<ResourceData> resources;

private:
    static void unbind(wl_resource *resource);
    void bind(wl_client *client, uint32_t version, uint32_t id) override;
    int32_t toTransform() const;
    int32_t toSubPixel() const;
    void sendGeometry(wl_resource *resource);
    void sendScale(const ResourceData &data);

    OutputDeviceInterface *q;
};

OutputDeviceInterface::Private::Private(OutputDeviceInterface *q, Display *d)
    : Global::Private(d, &org_kde_kwin_outputdevice_interface, s_version)
    , q(q)
{
}

OutputDeviceInterface::OutputDeviceInterface(Display *display, QObject *parent)
    : Global(new Private(this, display), parent)
{
    Q_D();
    connect(this, &OutputDeviceInterface::currentModeChanged, this,
        [this, d] {
            auto currentModeIt = std::find_if(d->modes.constBegin(), d->modes.constEnd(), [](const Mode &mode) { return mode.flags.testFlag(ModeFlag::Current); });
            if (currentModeIt == d->modes.constEnd()) {
                return;
            }
            for (auto it = d->resources.constBegin(); it != d->resources.constEnd(); ++it) {
                d->sendMode((*it).resource, *currentModeIt);
                d->sendDone(*it);
            }
            wl_display_flush_clients(*(d->display));
        }
    );
    connect(this, &OutputDeviceInterface::subPixelChanged,       this, [this, d] { d->updateGeometry(); });
    connect(this, &OutputDeviceInterface::transformChanged,      this, [this, d] { d->updateGeometry(); });
    connect(this, &OutputDeviceInterface::globalPositionChanged, this, [this, d] { d->updateGeometry(); });
    connect(this, &OutputDeviceInterface::modelChanged,          this, [this, d] { d->updateGeometry(); });
    connect(this, &OutputDeviceInterface::manufacturerChanged,   this, [this, d] { d->updateGeometry(); });
    connect(this, &OutputDeviceInterface::scaleChanged,          this, [this, d] { d->updateScale(); });
}

OutputDeviceInterface::~OutputDeviceInterface() = default;

QSize OutputDeviceInterface::pixelSize() const
{
    Q_D();
    auto it = std::find_if(d->modes.begin(), d->modes.end(),
        [](const Mode &mode) {
            return mode.flags.testFlag(ModeFlag::Current);
        }
    );
    if (it == d->modes.end()) {
        return QSize();
    }
    return (*it).size;
}

int OutputDeviceInterface::refreshRate() const
{
    Q_D();
    auto it = std::find_if(d->modes.begin(), d->modes.end(),
        [](const Mode &mode) {
            return mode.flags.testFlag(ModeFlag::Current);
        }
    );
    if (it == d->modes.end()) {
        return 60000;
    }
    return (*it).refreshRate;
}

void OutputDeviceInterface::addMode(const QSize &size, OutputDeviceInterface::ModeFlags flags, int refreshRate)
{
    Q_ASSERT(!isValid());
    Q_D();

    auto currentModeIt = std::find_if(d->modes.begin(), d->modes.end(),
        [](const Mode &mode) {
            return mode.flags.testFlag(ModeFlag::Current);
        }
    );
    if (currentModeIt == d->modes.end() && !flags.testFlag(ModeFlag::Current)) {
        // no mode with current flag - enforce
        flags |= ModeFlag::Current;
    }
    if (currentModeIt != d->modes.end() && flags.testFlag(ModeFlag::Current)) {
        // another mode has the current flag - remove
        (*currentModeIt).flags &= ~uint(ModeFlag::Current);
    }

    if (flags.testFlag(ModeFlag::Preferred)) {
        // remove from existing Preferred mode
        auto preferredIt = std::find_if(d->modes.begin(), d->modes.end(),
            [](const Mode &mode) {
                return mode.flags.testFlag(ModeFlag::Preferred);
            }
        );
        if (preferredIt != d->modes.end()) {
            (*preferredIt).flags &= ~uint(ModeFlag::Preferred);
        }
    }

    auto existingModeIt = std::find_if(d->modes.begin(), d->modes.end(),
        [size,refreshRate](const Mode &mode) {
            return mode.size == size && mode.refreshRate == refreshRate;
        }
    );
    auto emitChanges = [this,flags,size,refreshRate] {
        emit modesChanged();
        if (flags.testFlag(ModeFlag::Current)) {
            emit refreshRateChanged(refreshRate);
            emit pixelSizeChanged(size);
            emit currentModeChanged();
        }
    };
    if (existingModeIt != d->modes.end()) {
        if ((*existingModeIt).flags == flags) {
            // nothing to do
            return;
        }
        (*existingModeIt).flags = flags;
        emitChanges();
        return;
    }
    Mode mode;
    mode.size = size;
    mode.refreshRate = refreshRate;
    mode.flags = flags;
    d->modes << mode;
    emitChanges();
}

void OutputDeviceInterface::setCurrentMode(const QSize &size, int refreshRate)
{
    Q_D();
    auto currentModeIt = std::find_if(d->modes.begin(), d->modes.end(),
        [](const Mode &mode) {
            return mode.flags.testFlag(ModeFlag::Current);
        }
    );
    if (currentModeIt != d->modes.end()) {
        // another mode has the current flag - remove
        (*currentModeIt).flags &= ~uint(ModeFlag::Current);
    }

    auto existingModeIt = std::find_if(d->modes.begin(), d->modes.end(),
        [size,refreshRate](const Mode &mode) {
            return mode.size == size && mode.refreshRate == refreshRate;
        }
    );

    Q_ASSERT(existingModeIt != d->modes.end());
    (*existingModeIt).flags |= ModeFlag::Current;
    emit modesChanged();
    emit refreshRateChanged((*existingModeIt).refreshRate);
    emit pixelSizeChanged((*existingModeIt).size);
    emit currentModeChanged();
}

int32_t OutputDeviceInterface::Private::toTransform() const
{
    switch (transform) {
    case Transform::Normal:
        return WL_OUTPUT_TRANSFORM_NORMAL;
    case Transform::Rotated90:
        return WL_OUTPUT_TRANSFORM_90;
    case Transform::Rotated180:
        return WL_OUTPUT_TRANSFORM_180;
    case Transform::Rotated270:
        return WL_OUTPUT_TRANSFORM_270;
    case Transform::Flipped:
        return WL_OUTPUT_TRANSFORM_FLIPPED;
    case Transform::Flipped90:
        return WL_OUTPUT_TRANSFORM_FLIPPED_90;
    case Transform::Flipped180:
        return WL_OUTPUT_TRANSFORM_FLIPPED_180;
    case Transform::Flipped270:
        return WL_OUTPUT_TRANSFORM_FLIPPED_270;
    }
    abort();
}

int32_t OutputDeviceInterface::Private::toSubPixel() const
{
    switch (subPixel) {
    case SubPixel::Unknown:
        return WL_OUTPUT_SUBPIXEL_UNKNOWN;
    case SubPixel::None:
        return WL_OUTPUT_SUBPIXEL_NONE;
    case SubPixel::HorizontalRGB:
        return WL_OUTPUT_SUBPIXEL_HORIZONTAL_RGB;
    case SubPixel::HorizontalBGR:
        return WL_OUTPUT_SUBPIXEL_HORIZONTAL_BGR;
    case SubPixel::VerticalRGB:
        return WL_OUTPUT_SUBPIXEL_VERTICAL_RGB;
    case SubPixel::VerticalBGR:
        return WL_OUTPUT_SUBPIXEL_VERTICAL_BGR;
    }
    abort();
}

void OutputDeviceInterface::Private::bind(wl_client *client, uint32_t version, uint32_t id)
{
    auto c = display->getConnection(client);
    wl_resource *resource = c->createResource(&org_kde_kwin_outputdevice_interface, qMin(version, s_version), id);
    if (!resource) {
        wl_client_post_no_memory(client);
        return;
    }
    wl_resource_set_user_data(resource, this);
    wl_resource_set_destructor(resource, unbind);
    ResourceData r;
    r.resource = resource;
    r.version = version;
    resources << r;

    sendGeometry(resource);
    sendScale(r);

    auto currentModeIt = modes.constEnd();
    for (auto it = modes.constBegin(); it != modes.constEnd(); ++it) {
        const Mode &mode = *it;
        if (mode.flags.testFlag(ModeFlag::Current)) {
            // needs to be sent as last mode
            currentModeIt = it;
            continue;
        }
        sendMode(resource, mode);
    }

    if (currentModeIt != modes.constEnd()) {
        sendMode(resource, *currentModeIt);
    }

    sendDone(r);
    c->flush();
}

void OutputDeviceInterface::Private::unbind(wl_resource *resource)
{
    auto o = reinterpret_cast<OutputDeviceInterface::Private*>(wl_resource_get_user_data(resource));
    auto it = std::find_if(o->resources.begin(), o->resources.end(), [resource](const ResourceData &r) { return r.resource == resource; });
    if (it != o->resources.end()) {
        o->resources.erase(it);
    }
}

void OutputDeviceInterface::Private::sendMode(wl_resource *resource, const Mode &mode)
{
    int32_t flags = 0;
    if (mode.flags.testFlag(ModeFlag::Current)) {
        flags |= WL_OUTPUT_MODE_CURRENT;
    }
    if (mode.flags.testFlag(ModeFlag::Preferred)) {
        flags |= WL_OUTPUT_MODE_PREFERRED;
    }
    org_kde_kwin_outputdevice_send_mode(resource,
                        flags,
                        mode.size.width(),
                        mode.size.height(),
                        mode.refreshRate);

}

void OutputDeviceInterface::Private::sendGeometry(wl_resource *resource)
{
    org_kde_kwin_outputdevice_send_geometry(resource,
                            globalPosition.x(),
                            globalPosition.y(),
                            physicalSize.width(),
                            physicalSize.height(),
                            toSubPixel(),
                            qPrintable(manufacturer),
                            qPrintable(model),
                            toTransform());
}

void OutputDeviceInterface::Private::sendScale(const ResourceData &data)
{
    if (data.version < 2) {
        return;
    }
    org_kde_kwin_outputdevice_send_scale(data.resource, scale);
}

void OutputDeviceInterface::Private::sendDone(const ResourceData &data)
{
    if (data.version < 2) {
        return;
    }
    org_kde_kwin_outputdevice_send_done(data.resource);
}

void OutputDeviceInterface::Private::updateGeometry()
{
    for (auto it = resources.constBegin(); it != resources.constEnd(); ++it) {
        sendGeometry((*it).resource);
        sendDone(*it);
    }
}

void OutputDeviceInterface::Private::updateScale()
{
    for (auto it = resources.constBegin(); it != resources.constEnd(); ++it) {
        sendScale(*it);
        sendDone(*it);
    }
}

#define SETTER(setterName, type, argumentName) \
    void OutputDeviceInterface::setterName(type arg) \
    { \
        Q_D(); \
        if (d->argumentName == arg) { \
            return; \
        } \
        d->argumentName = arg; \
        emit argumentName##Changed(d->argumentName); \
    }

SETTER(setPhysicalSize, const QSize&, physicalSize)
SETTER(setGlobalPosition, const QPoint&, globalPosition)
SETTER(setManufacturer, const QString&, manufacturer)
SETTER(setModel, const QString&, model)
SETTER(setScale, int, scale)
SETTER(setSubPixel, SubPixel, subPixel)
SETTER(setTransform, Transform, transform)

#undef SETTER

QSize OutputDeviceInterface::physicalSize() const
{
    Q_D();
    return d->physicalSize;
}

QPoint OutputDeviceInterface::globalPosition() const
{
    Q_D();
    return d->globalPosition;
}

QString OutputDeviceInterface::manufacturer() const
{
    Q_D();
    return d->manufacturer;
}

QString OutputDeviceInterface::model() const
{
    Q_D();
    return d->model;
}

int OutputDeviceInterface::scale() const
{
    Q_D();
    return d->scale;
}

OutputDeviceInterface::SubPixel OutputDeviceInterface::subPixel() const
{
    Q_D();
    return d->subPixel;
}

OutputDeviceInterface::Transform OutputDeviceInterface::transform() const
{
    Q_D();
    return d->transform;
}

QList< OutputDeviceInterface::Mode > OutputDeviceInterface::modes() const
{
    Q_D();
    return d->modes;
}

OutputDeviceInterface::Private *OutputDeviceInterface::d_func() const
{
    return reinterpret_cast<Private*>(d.data());
}

}
}