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
#include "seat_interface.h"
#include "seat_interface_p.h"
#include "display.h"
#include "datadevice_interface.h"
#include "datasource_interface.h"
#include "keyboard_interface.h"
#include "keyboard_interface_p.h"
#include "pointer_interface.h"
#include "pointer_interface_p.h"
#include "surface_interface.h"
#include "textinput_interface_p.h"
// Wayland
#ifndef WL_SEAT_NAME_SINCE_VERSION
#define WL_SEAT_NAME_SINCE_VERSION 2
#endif
// linux
#include <config-kwayland.h>
#if HAVE_LINUX_INPUT_H
#include <linux/input.h>
#endif

namespace KWayland
{

namespace Server
{

const quint32 SeatInterface::Private::s_version = 4;
const qint32 SeatInterface::Private::s_pointerVersion = 3;
const qint32 SeatInterface::Private::s_touchVersion = 3;
const qint32 SeatInterface::Private::s_keyboardVersion = 4;

SeatInterface::Private::Private(SeatInterface *q, Display *display)
    : Global::Private(display, &wl_seat_interface, s_version)
    , q(q)
{
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS
const struct wl_seat_interface SeatInterface::Private::s_interface = {
    getPointerCallback,
    getKeyboardCallback,
    getTouchCallback
};
#endif

SeatInterface::SeatInterface(Display *display, QObject *parent)
    : Global(new Private(this, display), parent)
{
    Q_D();
    connect(this, &SeatInterface::nameChanged, this,
        [this, d] {
            for (auto it = d->resources.constBegin(); it != d->resources.constEnd(); ++it) {
                d->sendName(*it);
            }
        }
    );
    auto sendCapabilitiesAll = [this, d] {
        for (auto it = d->resources.constBegin(); it != d->resources.constEnd(); ++it) {
            d->sendCapabilities(*it);
        }
    };
    connect(this, &SeatInterface::hasPointerChanged,  this, sendCapabilitiesAll);
    connect(this, &SeatInterface::hasKeyboardChanged, this, sendCapabilitiesAll);
    connect(this, &SeatInterface::hasTouchChanged,    this, sendCapabilitiesAll);
}

SeatInterface::~SeatInterface()
{
    Q_D();
    while (!d->resources.isEmpty()) {
        wl_resource_destroy(d->resources.takeLast());
    }
}

void SeatInterface::Private::bind(wl_client *client, uint32_t version, uint32_t id)
{
    wl_resource *r = wl_resource_create(client, &wl_seat_interface, qMin(s_version, version), id);
    if (!r) {
        wl_client_post_no_memory(client);
        return;
    }
    resources << r;

    wl_resource_set_implementation(r, &s_interface, this, unbind);

    sendCapabilities(r);
    sendName(r);
}

void SeatInterface::Private::unbind(wl_resource *r)
{
    cast(r)->resources.removeAll(r);
}

void SeatInterface::Private::updatePointerButtonSerial(quint32 button, quint32 serial)
{
    auto it = globalPointer.buttonSerials.find(button);
    if (it == globalPointer.buttonSerials.end()) {
        globalPointer.buttonSerials.insert(button, serial);
        return;
    }
    it.value() = serial;
}

void SeatInterface::Private::updatePointerButtonState(quint32 button, Pointer::State state)
{
    auto it = globalPointer.buttonStates.find(button);
    if (it == globalPointer.buttonStates.end()) {
        globalPointer.buttonStates.insert(button, state);
        return;
    }
    it.value() = state;
}

bool SeatInterface::Private::updateKey(quint32 key, Keyboard::State state)
{
    auto it = keys.states.find(key);
    if (it == keys.states.end()) {
        keys.states.insert(key, state);
        return true;
    }
    if (it.value() == state) {
        return false;
    }
    it.value() = state;
    return true;
}

void SeatInterface::Private::sendName(wl_resource *r)
{
    if (wl_resource_get_version(r) < WL_SEAT_NAME_SINCE_VERSION) {
        return;
    }
    wl_seat_send_name(r, name.toUtf8().constData());
}

void SeatInterface::Private::sendCapabilities(wl_resource *r)
{
    uint32_t capabilities = 0;
    if (pointer) {
        capabilities |= WL_SEAT_CAPABILITY_POINTER;
    }
    if (keyboard) {
        capabilities |= WL_SEAT_CAPABILITY_KEYBOARD;
    }
    if (touch) {
        capabilities |= WL_SEAT_CAPABILITY_TOUCH;
    }
    wl_seat_send_capabilities(r, capabilities);
}

SeatInterface::Private *SeatInterface::Private::cast(wl_resource *r)
{
    return r ? reinterpret_cast<SeatInterface::Private*>(wl_resource_get_user_data(r)) : nullptr;
}

namespace {
template <typename T>
static
T *interfaceForSurface(SurfaceInterface *surface, const QVector<T*> &interfaces)
{
    if (!surface) {
        return nullptr;
    }

    for (auto it = interfaces.begin(); it != interfaces.end(); ++it) {
        if ((*it)->client() == surface->client()) {
            return (*it);
        }
    }
    return nullptr;
}
}

PointerInterface *SeatInterface::Private::pointerForSurface(SurfaceInterface *surface) const
{
    return interfaceForSurface(surface, pointers);
}

KeyboardInterface *SeatInterface::Private::keyboardForSurface(SurfaceInterface *surface) const
{
    return interfaceForSurface(surface, keyboards);
}

TouchInterface *SeatInterface::Private::touchForSurface(SurfaceInterface *surface) const
{
    return interfaceForSurface(surface, touchs);
}

DataDeviceInterface *SeatInterface::Private::dataDeviceForSurface(SurfaceInterface *surface) const
{
    return interfaceForSurface(surface, dataDevices);
}

TextInputInterface *SeatInterface::Private::textInputForSurface(SurfaceInterface *surface) const
{
    return interfaceForSurface(surface, textInputs);
}

void SeatInterface::Private::registerDataDevice(DataDeviceInterface *dataDevice)
{
    Q_ASSERT(dataDevice->seat() == q);
    dataDevices << dataDevice;
    auto dataDeviceCleanup = [this, dataDevice] {
        dataDevices.removeOne(dataDevice);
        if (keys.focus.selection == dataDevice) {
            keys.focus.selection = nullptr;
        }
        if (currentSelection == dataDevice) {
            // current selection is cleared
            currentSelection = nullptr;
            if (keys.focus.selection) {
                keys.focus.selection->sendClearSelection();
            }
        }
    };
    QObject::connect(dataDevice, &QObject::destroyed, q, dataDeviceCleanup);
    QObject::connect(dataDevice, &Resource::unbound, q, dataDeviceCleanup);
    QObject::connect(dataDevice, &DataDeviceInterface::selectionChanged, q,
        [this, dataDevice] {
            updateSelection(dataDevice, true);
        }
    );
    QObject::connect(dataDevice, &DataDeviceInterface::selectionCleared, q,
        [this, dataDevice] {
            updateSelection(dataDevice, false);
        }
    );
    QObject::connect(dataDevice, &DataDeviceInterface::dragStarted, q,
        [this, dataDevice] {
            if (q->hasImplicitPointerGrab(dataDevice->dragImplicitGrabSerial())) {
                drag.mode = Drag::Mode::Pointer;
            } else {
                // TODO: touch
                return;
            }
            drag.source = dataDevice;
            drag.target = dataDevice;
            drag.surface = dataDevice->origin();
            drag.sourcePointer = interfaceForSurface(drag.surface, pointers);
            // TODO: transformation needs to be either pointer or touch
            drag.transformation = globalPointer.focus.transformation;
            drag.destroyConnection = QObject::connect(dataDevice, &QObject::destroyed, q,
                [this] {
                    endDrag(display->nextSerial());
                }
            );
            dataDevice->updateDragTarget(dataDevice->origin(), dataDevice->dragImplicitGrabSerial());
            emit q->dragStarted();
            emit q->dragSurfaceChanged();
        }
    );
    // is the new DataDevice for the current keyoard focus?
    if (keys.focus.surface && !keys.focus.selection) {
        // same client?
        if (keys.focus.surface->client() == dataDevice->client()) {
            keys.focus.selection = dataDevice;
            if (currentSelection) {
                dataDevice->sendSelection(currentSelection);
            }
        }
    }
}


void SeatInterface::Private::registerTextInput(TextInputInterface *ti)
{
    // text input version 0 might call this multiple times
    if (textInputs.contains(ti)) {
        return;
    }
    textInputs << ti;
    if (textInput.focus.surface && textInput.focus.surface->client() == ti->client()) {
        // this is a text input for the currently focused text input surface
        if (!textInput.focus.textInput) {
            textInput.focus.textInput = ti;
            ti->d_func()->sendEnter(textInput.focus.surface, textInput.focus.serial);
            emit q->focusedTextInputChanged();
        }
    }
    QObject::connect(ti, &QObject::destroyed, q,
        [this, ti] {
            textInputs.removeAt(textInputs.indexOf(ti));
            if (textInput.focus.textInput == ti) {
                textInput.focus.textInput = nullptr;
                emit q->focusedTextInputChanged();
            }
        }
    );
}

void SeatInterface::Private::endDrag(quint32 serial)
{
    auto target = drag.target;
    QObject::disconnect(drag.destroyConnection);
    if (target) {
        target->drop();
        target->updateDragTarget(nullptr, serial);
    }
    drag = Drag();
    emit q->dragSurfaceChanged();
    emit q->dragEnded();
}

void SeatInterface::Private::updateSelection(DataDeviceInterface *dataDevice, bool set)
{
    if (keys.focus.surface && (keys.focus.surface->client() == dataDevice->client())) {
        if (currentSelection) {
            // cancel the previous selection
            if (auto s = currentSelection->selection()) {
                if (currentSelection != dataDevice) {
                    // only if current selection is not on the same device
                    // that would cancel the newly set source
                    s->cancel();
                }
            }
        }
        // new selection on a data device belonging to current keyboard focus
        currentSelection = dataDevice;
    }
    if (dataDevice == currentSelection) {
        // need to send out the selection
        if (keys.focus.selection) {
            if (set) {
                keys.focus.selection->sendSelection(dataDevice);
            } else {
                keys.focus.selection->sendClearSelection();
                currentSelection = nullptr;
            }
        }
    }
}

void SeatInterface::setHasKeyboard(bool has)
{
    Q_D();
    if (d->keyboard == has) {
        return;
    }
    d->keyboard = has;
    emit hasKeyboardChanged(d->keyboard);
}

void SeatInterface::setHasPointer(bool has)
{
    Q_D();
    if (d->pointer == has) {
        return;
    }
    d->pointer = has;
    emit hasPointerChanged(d->pointer);
}

void SeatInterface::setHasTouch(bool has)
{
    Q_D();
    if (d->touch == has) {
        return;
    }
    d->touch = has;
    emit hasTouchChanged(d->touch);
}

void SeatInterface::setName(const QString &name)
{
    Q_D();
    if (d->name == name) {
        return;
    }
    d->name = name;
    emit nameChanged(d->name);
}

void SeatInterface::Private::getPointerCallback(wl_client *client, wl_resource *resource, uint32_t id)
{
    cast(resource)->getPointer(client, resource, id);
}

void SeatInterface::Private::getPointer(wl_client *client, wl_resource *resource, uint32_t id)
{
    // TODO: only create if seat has pointer?
    PointerInterface *pointer = new PointerInterface(q, resource);
    auto clientConnection = display->getConnection(client);
    pointer->create(clientConnection, qMin(wl_resource_get_version(resource), s_pointerVersion), id);
    if (!pointer->resource()) {
        wl_resource_post_no_memory(resource);
        delete pointer;
        return;
    }
    pointers << pointer;
    if (globalPointer.focus.surface && globalPointer.focus.surface->client() == clientConnection) {
        // this is a pointer for the currently focused pointer surface
        if (!globalPointer.focus.pointer) {
            globalPointer.focus.pointer = pointer;
            pointer->setFocusedSurface(globalPointer.focus.surface, globalPointer.focus.serial);
            emit q->focusedPointerChanged(pointer);
        }
    }
    QObject::connect(pointer, &QObject::destroyed, q,
        [pointer,this] {
            pointers.removeAt(pointers.indexOf(pointer));
            if (globalPointer.focus.pointer == pointer) {
                globalPointer.focus.pointer = nullptr;
                emit q->focusedPointerChanged(nullptr);
            }
        }
    );
    emit q->pointerCreated(pointer);
}

void SeatInterface::Private::getKeyboardCallback(wl_client *client, wl_resource *resource, uint32_t id)
{
    cast(resource)->getKeyboard(client, resource, id);
}

void SeatInterface::Private::getKeyboard(wl_client *client, wl_resource *resource, uint32_t id)
{
    // TODO: only create if seat has keyboard?
    KeyboardInterface *keyboard = new KeyboardInterface(q, resource);
    auto clientConnection = display->getConnection(client);
    keyboard->create(clientConnection, qMin(wl_resource_get_version(resource), s_keyboardVersion) , id);
    if (!keyboard->resource()) {
        wl_resource_post_no_memory(resource);
        delete keyboard;
        return;
    }
    keyboard->repeatInfo(keys.keyRepeat.charactersPerSecond, keys.keyRepeat.delay);
    if (keys.keymap.xkbcommonCompatible) {
        keyboard->setKeymap(keys.keymap.fd, keys.keymap.size);
    }
    keyboards << keyboard;
    if (keys.focus.surface && keys.focus.surface->client() == clientConnection) {
        // this is a keyboard for the currently focused keyboard surface
        if (!keys.focus.keyboard) {
            keys.focus.keyboard = keyboard;
            keyboard->setFocusedSurface(keys.focus.surface, keys.focus.serial);
        }
    }
    QObject::connect(keyboard, &QObject::destroyed, q,
        [keyboard,this] {
            keyboards.removeAt(keyboards.indexOf(keyboard));
            if (keys.focus.keyboard == keyboard) {
                keys.focus.keyboard = nullptr;
            }
        }
    );
    emit q->keyboardCreated(keyboard);
}

void SeatInterface::Private::getTouchCallback(wl_client *client, wl_resource *resource, uint32_t id)
{
    cast(resource)->getTouch(client, resource, id);
}

void SeatInterface::Private::getTouch(wl_client *client, wl_resource *resource, uint32_t id)
{
    // TODO: only create if seat has touch?
    TouchInterface *touch = new TouchInterface(q, resource);
    auto clientConnection = display->getConnection(client);
    touch->create(clientConnection, qMin(wl_resource_get_version(resource), s_touchVersion), id);
    if (!touch->resource()) {
        wl_resource_post_no_memory(resource);
        delete touch;
        return;
    }
    touchs << touch;
    if (touchInterface.focus.surface && touchInterface.focus.surface->client() == clientConnection) {
        // this is a touch for the currently focused touch surface
        if (!touchInterface.focus.touch) {
            touchInterface.focus.touch = touch;
            if (!touchInterface.ids.isEmpty()) {
                // TODO: send out all the points
            }
        }
    }
    QObject::connect(touch, &QObject::destroyed, q,
        [touch,this] {
            touchs.removeAt(touchs.indexOf(touch));
            if (touchInterface.focus.touch == touch) {
                touchInterface.focus.touch = nullptr;
            }
        }
    );
    emit q->touchCreated(touch);
}

QString SeatInterface::name() const
{
    Q_D();
    return d->name;
}

bool SeatInterface::hasPointer() const
{
    Q_D();
    return d->pointer;
}

bool SeatInterface::hasKeyboard() const
{
    Q_D();
    return d->keyboard;
}

bool SeatInterface::hasTouch() const
{
    Q_D();
    return d->touch;
}

SeatInterface *SeatInterface::get(wl_resource *native)
{
    return Private::get(native);
}

SeatInterface::Private *SeatInterface::d_func() const
{
    return reinterpret_cast<Private*>(d.data());
}

QPointF SeatInterface::pointerPos() const
{
    Q_D();
    return d->globalPointer.pos;
}

void SeatInterface::setPointerPos(const QPointF &pos)
{
    Q_D();
    if (d->globalPointer.pos == pos) {
        return;
    }
    d->globalPointer.pos = pos;
    emit pointerPosChanged(pos);
}

quint32 SeatInterface::timestamp() const
{
    Q_D();
    return d->timestamp;
}

void SeatInterface::setTimestamp(quint32 time)
{
    Q_D();
    if (d->timestamp == time) {
        return;
    }
    d->timestamp = time;
    emit timestampChanged(time);
}

void SeatInterface::setDragTarget(SurfaceInterface *surface, const QPointF &globalPosition, const QMatrix4x4 &inputTransformation)
{
    Q_D();
    if (surface == d->drag.surface) {
        // no change
        return;
    }
    const quint32 serial = d->display->nextSerial();
    if (d->drag.target) {
        d->drag.target->updateDragTarget(nullptr, serial);
    }
    d->drag.target = d->dataDeviceForSurface(surface);
    // TODO: update touch
    if (d->drag.mode == Private::Drag::Mode::Pointer) {
        setPointerPos(globalPosition);
    }
    if (d->drag.target) {
        d->drag.surface = surface;
        d->drag.transformation = inputTransformation;
        d->drag.target->updateDragTarget(surface, serial);
    } else {
        d->drag.surface = nullptr;
    }
    emit dragSurfaceChanged();
    return;
}

void SeatInterface::setDragTarget(SurfaceInterface *surface, const QMatrix4x4 &inputTransformation)
{
    // TODO: handle touch
    setDragTarget(surface, pointerPos(), inputTransformation);
}

SurfaceInterface *SeatInterface::focusedPointerSurface() const
{
    Q_D();
    return d->globalPointer.focus.surface;
}

void SeatInterface::setFocusedPointerSurface(SurfaceInterface *surface, const QPointF &surfacePosition)
{
    QMatrix4x4 m;
    m.translate(-surfacePosition.x(), -surfacePosition.y());
    setFocusedPointerSurface(surface, m);
    Q_D();
    if (d->globalPointer.focus.surface) {
        d->globalPointer.focus.offset = surfacePosition;
    }
}

void SeatInterface::setFocusedPointerSurface(SurfaceInterface *surface, const QMatrix4x4 &transformation)
{
    Q_D();
    if (d->drag.mode == Private::Drag::Mode::Pointer) {
        // ignore
        return;
    }
    const quint32 serial = d->display->nextSerial();
    if (d->globalPointer.focus.pointer) {
        d->globalPointer.focus.pointer->setFocusedSurface(nullptr, serial);
    }
    if (d->globalPointer.focus.surface) {
        disconnect(d->globalPointer.focus.destroyConnection);
    }
    d->globalPointer.focus = Private::Pointer::Focus();
    d->globalPointer.focus.surface = surface;
    PointerInterface *p = d->pointerForSurface(surface);
    if (p && !p->resource()) {
        p = nullptr;
    }
    d->globalPointer.focus.pointer = p;
    if (d->globalPointer.focus.surface) {
        d->globalPointer.focus.destroyConnection = connect(surface, &QObject::destroyed, this,
            [this] {
                Q_D();
                d->globalPointer.focus = Private::Pointer::Focus();
                emit focusedPointerChanged(nullptr);
            }
        );
        d->globalPointer.focus.offset = QPointF();
        d->globalPointer.focus.transformation = transformation;
        d->globalPointer.focus.serial = serial;
    }
    emit focusedPointerChanged(p);
    if (!p) {
        return;
    }
    p->setFocusedSurface(surface, serial);
}

PointerInterface *SeatInterface::focusedPointer() const
{
    Q_D();
    return d->globalPointer.focus.pointer;
}

void SeatInterface::setFocusedPointerSurfacePosition(const QPointF &surfacePosition)
{
    Q_D();
    if (d->globalPointer.focus.surface) {
        d->globalPointer.focus.offset = surfacePosition;
        d->globalPointer.focus.transformation = QMatrix4x4();
        d->globalPointer.focus.transformation.translate(-surfacePosition.x(), -surfacePosition.y());
    }
}

QPointF SeatInterface::focusedPointerSurfacePosition() const
{
    Q_D();
    return d->globalPointer.focus.offset;
}

void SeatInterface::setFocusedPointerSurfaceTransformation(const QMatrix4x4 &transformation)
{
    Q_D();
    if (d->globalPointer.focus.surface) {
        d->globalPointer.focus.transformation = transformation;
    }
}

QMatrix4x4 SeatInterface::focusedPointerSurfaceTransformation() const
{
    Q_D();
    return d->globalPointer.focus.transformation;
}

namespace {
static quint32 qtToWaylandButton(Qt::MouseButton button)
{
#if HAVE_LINUX_INPUT_H
    static const QHash<Qt::MouseButton, quint32> s_buttons({
        {Qt::LeftButton, BTN_LEFT},
        {Qt::RightButton, BTN_RIGHT},
        {Qt::MiddleButton, BTN_MIDDLE},
        {Qt::ExtraButton1, BTN_BACK},    // note: QtWayland maps BTN_SIDE
        {Qt::ExtraButton2, BTN_FORWARD}, // note: QtWayland maps BTN_EXTRA
        {Qt::ExtraButton3, BTN_TASK},    // note: QtWayland maps BTN_FORWARD
        {Qt::ExtraButton4, BTN_EXTRA},   // note: QtWayland maps BTN_BACK
        {Qt::ExtraButton5, BTN_SIDE},    // note: QtWayland maps BTN_TASK
        {Qt::ExtraButton6, BTN_TASK + 1},
        {Qt::ExtraButton7, BTN_TASK + 2},
        {Qt::ExtraButton8, BTN_TASK + 3},
        {Qt::ExtraButton9, BTN_TASK + 4},
        {Qt::ExtraButton10, BTN_TASK + 5},
        {Qt::ExtraButton11, BTN_TASK + 6},
        {Qt::ExtraButton12, BTN_TASK + 7},
        {Qt::ExtraButton13, BTN_TASK + 8}
        // further mapping not possible, 0x120 is BTN_JOYSTICK
    });
    return s_buttons.value(button, 0);
#else
    return 0;
#endif
}
}

bool SeatInterface::isPointerButtonPressed(Qt::MouseButton button) const
{
    return isPointerButtonPressed(qtToWaylandButton(button));
}

bool SeatInterface::isPointerButtonPressed(quint32 button) const
{
    Q_D();
    auto it = d->globalPointer.buttonStates.constFind(button);
    if (it == d->globalPointer.buttonStates.constEnd()) {
        return false;
    }
    return it.value() == Private::Pointer::State::Pressed ? true : false;
}

void SeatInterface::pointerAxis(Qt::Orientation orientation, quint32 delta)
{
    Q_D();
    if (d->drag.mode == Private::Drag::Mode::Pointer) {
        // ignore
        return;
    }
    if (d->globalPointer.focus.pointer && d->globalPointer.focus.surface) {
        d->globalPointer.focus.pointer->axis(orientation, delta);
    }
}

void SeatInterface::pointerButtonPressed(Qt::MouseButton button)
{
    const quint32 nativeButton = qtToWaylandButton(button);
    if (nativeButton == 0) {
        return;
    }
    pointerButtonPressed(nativeButton);
}

void SeatInterface::pointerButtonPressed(quint32 button)
{
    Q_D();
    const quint32 serial = d->display->nextSerial();
    d->updatePointerButtonSerial(button, serial);
    d->updatePointerButtonState(button, Private::Pointer::State::Pressed);
    if (d->drag.mode == Private::Drag::Mode::Pointer) {
        // ignore
        return;
    }
    if (d->globalPointer.focus.pointer && d->globalPointer.focus.surface) {
        d->globalPointer.focus.pointer->buttonPressed(button, serial);
        if (d->globalPointer.focus.surface == d->keys.focus.surface && d->keys.focus.keyboard) {
            // update the focused child surface
            d->keys.focus.keyboard->d_func()->focusChildSurface(d->globalPointer.focus.pointer->d_func()->focusedChildSurface, serial);
        }
    }
}

void SeatInterface::pointerButtonReleased(Qt::MouseButton button)
{
    const quint32 nativeButton = qtToWaylandButton(button);
    if (nativeButton == 0) {
        return;
    }
    pointerButtonReleased(nativeButton);
}

void SeatInterface::pointerButtonReleased(quint32 button)
{
    Q_D();
    const quint32 serial = d->display->nextSerial();
    const quint32 currentButtonSerial = pointerButtonSerial(button);
    d->updatePointerButtonSerial(button, serial);
    d->updatePointerButtonState(button, Private::Pointer::State::Released);
    if (d->drag.mode == Private::Drag::Mode::Pointer) {
        if (d->drag.source->dragImplicitGrabSerial() != currentButtonSerial) {
            // not our drag button - ignore
            return;
        }
        d->endDrag(serial);
        return;
    }
    if (d->globalPointer.focus.pointer && d->globalPointer.focus.surface) {
        d->globalPointer.focus.pointer->buttonReleased(button, serial);
    }
}

quint32 SeatInterface::pointerButtonSerial(Qt::MouseButton button) const
{
    return pointerButtonSerial(qtToWaylandButton(button));
}

quint32 SeatInterface::pointerButtonSerial(quint32 button) const
{
    Q_D();
    auto it = d->globalPointer.buttonSerials.constFind(button);
    if (it == d->globalPointer.buttonSerials.constEnd()) {
        return 0;
    }
    return it.value();
}

void SeatInterface::keyPressed(quint32 key)
{
    Q_D();
    d->keys.lastStateSerial = d->display->nextSerial();
    if (!d->updateKey(key, Private::Keyboard::State::Pressed)) {
        return;
    }
    if (d->keys.focus.keyboard && d->keys.focus.surface) {
        d->keys.focus.keyboard->keyPressed(key, d->keys.lastStateSerial);
    }
}

void SeatInterface::keyReleased(quint32 key)
{
    Q_D();
    d->keys.lastStateSerial = d->display->nextSerial();
    if (!d->updateKey(key, Private::Keyboard::State::Released)) {
        return;
    }
    if (d->keys.focus.keyboard && d->keys.focus.surface) {
        d->keys.focus.keyboard->keyReleased(key, d->keys.lastStateSerial);
    }
}

SurfaceInterface *SeatInterface::focusedKeyboardSurface() const
{
    Q_D();
    return d->keys.focus.surface;
}

void SeatInterface::setFocusedKeyboardSurface(SurfaceInterface *surface)
{
    Q_D();
    const quint32 serial = d->display->nextSerial();
    if (d->keys.focus.keyboard) {
        d->keys.focus.keyboard->setFocusedSurface(nullptr, serial);
    }
    if (d->keys.focus.surface) {
        disconnect(d->keys.focus.destroyConnection);
    }
    d->keys.focus = Private::Keyboard::Focus();
    d->keys.focus.surface = surface;
    KeyboardInterface *k = d->keyboardForSurface(surface);
    if (k && !k->resource()) {
        k = nullptr;
    }
    d->keys.focus.keyboard = k;
    if (d->keys.focus.surface) {
        d->keys.focus.destroyConnection = connect(surface, &QObject::destroyed, this,
            [this] {
                Q_D();
                d->keys.focus = Private::Keyboard::Focus();
            }
        );
        d->keys.focus.serial = serial;
        // selection?
        d->keys.focus.selection = d->dataDeviceForSurface(surface);
        if (d->keys.focus.selection) {
            if (d->currentSelection) {
                d->keys.focus.selection->sendSelection(d->currentSelection);
            } else {
                d->keys.focus.selection->sendClearSelection();
            }
        }
    }
    if (k) {
        k->setFocusedSurface(surface, serial);
    }
    // focused text input surface follows keyboard
    if (hasKeyboard()) {
        setFocusedTextInputSurface(surface);
    }
}

void SeatInterface::setKeymap(int fd, quint32 size)
{
    Q_D();
    d->keys.keymap.xkbcommonCompatible = true;
    d->keys.keymap.fd = fd;
    d->keys.keymap.size = size;
    for (auto it = d->keyboards.constBegin(); it != d->keyboards.constEnd(); ++it) {
        (*it)->setKeymap(fd, size);
    }
}

void SeatInterface::updateKeyboardModifiers(quint32 depressed, quint32 latched, quint32 locked, quint32 group)
{
    Q_D();
    bool changed = false;
#define UPDATE( value ) \
    if (d->keys.modifiers.value != value) { \
        d->keys.modifiers.value = value; \
        changed = true; \
    }
    UPDATE(depressed)
    UPDATE(latched)
    UPDATE(locked)
    UPDATE(group)
    if (!changed) {
        return;
    }
    const quint32 serial = d->display->nextSerial();
    d->keys.modifiers.serial = serial;
    if (d->keys.focus.keyboard && d->keys.focus.surface) {
        d->keys.focus.keyboard->updateModifiers(depressed, latched, locked, group, serial);
    }
}

void SeatInterface::setKeyRepeatInfo(qint32 charactersPerSecond, qint32 delay)
{
    Q_D();
    d->keys.keyRepeat.charactersPerSecond = qMax(charactersPerSecond, 0);
    d->keys.keyRepeat.delay = qMax(delay, 0);
    for (auto it = d->keyboards.constBegin(); it != d->keyboards.constEnd(); ++it) {
        (*it)->repeatInfo(d->keys.keyRepeat.charactersPerSecond, d->keys.keyRepeat.delay);
    }
}

qint32 SeatInterface::keyRepeatDelay() const
{
    Q_D();
    return d->keys.keyRepeat.delay;
}

qint32 SeatInterface::keyRepeatRate() const
{
    Q_D();
    return d->keys.keyRepeat.charactersPerSecond;
}

bool SeatInterface::isKeymapXkbCompatible() const
{
    Q_D();
    return d->keys.keymap.xkbcommonCompatible;
}

int SeatInterface::keymapFileDescriptor() const
{
    Q_D();
    return d->keys.keymap.fd;
}

quint32 SeatInterface::keymapSize() const
{
    Q_D();
    return d->keys.keymap.size;
}

quint32 SeatInterface::depressedModifiers() const
{
    Q_D();
    return d->keys.modifiers.depressed;
}

quint32 SeatInterface::groupModifiers() const
{
    Q_D();
    return d->keys.modifiers.group;
}

quint32 SeatInterface::latchedModifiers() const
{
    Q_D();
    return d->keys.modifiers.latched;
}

quint32 SeatInterface::lockedModifiers() const
{
    Q_D();
    return d->keys.modifiers.locked;
}

quint32 SeatInterface::lastModifiersSerial() const
{
    Q_D();
    return d->keys.modifiers.serial;
}

QVector< quint32 > SeatInterface::pressedKeys() const
{
    Q_D();
    QVector<quint32> keys;
    for (auto it = d->keys.states.begin(); it != d->keys.states.end(); ++it) {
        if (it.value() == Private::Keyboard::State::Pressed) {
            keys << it.key();
        }
    }
    return keys;
}

KeyboardInterface *SeatInterface::focusedKeyboard() const
{
    Q_D();
    return d->keys.focus.keyboard;
}

void SeatInterface::cancelTouchSequence()
{
    Q_D();
    if (d->touchInterface.focus.touch) {
        d->touchInterface.focus.touch->cancel();
    }
    d->touchInterface.ids.clear();
}

TouchInterface *SeatInterface::focusedTouch() const
{
    Q_D();
    return d->touchInterface.focus.touch;
}

SurfaceInterface *SeatInterface::focusedTouchSurface() const
{
    Q_D();
    return d->touchInterface.focus.surface;
}

QPointF SeatInterface::focusedTouchSurfacePosition() const
{
    Q_D();
    return d->touchInterface.focus.offset;
}

bool SeatInterface::isTouchSequence() const
{
    Q_D();
    return !d->touchInterface.ids.isEmpty();
}

void SeatInterface::setFocusedTouchSurface(SurfaceInterface *surface, const QPointF &surfacePosition)
{
    if (isTouchSequence()) {
        // changing surface not allowed during a touch sequence
        return;
    }
    Q_D();
    if (d->touchInterface.focus.surface) {
        disconnect(d->touchInterface.focus.destroyConnection);
    }
    d->touchInterface.focus = Private::Touch::Focus();
    d->touchInterface.focus.surface = surface;
    d->touchInterface.focus.offset = surfacePosition;
    TouchInterface *t = d->touchForSurface(surface);
    if (t && !t->resource()) {
        t = nullptr;
    }
    d->touchInterface.focus.touch = t;
    if (d->touchInterface.focus.surface) {
        d->touchInterface.focus.destroyConnection = connect(surface, &QObject::destroyed, this,
            [this] {
                Q_D();
                if (isTouchSequence() && d->touchInterface.focus.touch) {
                    // Surface destroyed during touch sequence - send a cancel
                    d->touchInterface.focus.touch->cancel();
                }
                d->touchInterface.focus = Private::Touch::Focus();
            }
        );
    }
}

void SeatInterface::setFocusedTouchSurfacePosition(const QPointF &surfacePosition)
{
    Q_D();
    d->touchInterface.focus.offset = surfacePosition;
}

qint32 SeatInterface::touchDown(const QPointF &globalPosition)
{
    Q_D();
    const qint32 id = d->touchInterface.ids.isEmpty() ? 0 : d->touchInterface.ids.last() + 1;
    const quint32 serial = display()->nextSerial();
    if (d->touchInterface.focus.touch && d->touchInterface.focus.surface) {
        d->touchInterface.focus.touch->down(id, serial, globalPosition - d->touchInterface.focus.offset);
    } else if (id == 0 && focusedTouchSurface()) {
#if HAVE_LINUX_INPUT_H
        auto p = d->pointerForSurface(focusedTouchSurface());
        if (!p) {
            return id;
        }
        const QPointF pos = globalPosition - d->touchInterface.focus.offset;
        wl_pointer_send_enter(p->resource(), serial,
                          focusedTouchSurface()->resource(),
                          wl_fixed_from_double(pos.x()), wl_fixed_from_double(pos.y()));
        wl_pointer_send_motion(p->resource(), timestamp(),
                                wl_fixed_from_double(pos.x()), wl_fixed_from_double(pos.y()));

        wl_pointer_send_button(p->resource(), serial, timestamp(), BTN_LEFT, WL_POINTER_BUTTON_STATE_PRESSED);
#endif
    }

    d->touchInterface.ids << id;
    return id;
}

void SeatInterface::touchMove(qint32 id, const QPointF &globalPosition)
{
    Q_D();
    if (d->touchInterface.focus.touch && d->touchInterface.focus.surface) {
        d->touchInterface.focus.touch->move(id, globalPosition - d->touchInterface.focus.offset);
    } else if (id == 0 && focusedTouchSurface()) {
        auto p = d->pointerForSurface(focusedTouchSurface());
        if (!p) {
            return;
        }

        const QPointF pos = globalPosition - d->touchInterface.focus.offset;
        wl_pointer_send_motion(p->resource(), timestamp(),
                                wl_fixed_from_double(pos.x()), wl_fixed_from_double(pos.y()));
    } 
}

void SeatInterface::touchUp(qint32 id)
{
    Q_D();
    Q_ASSERT(d->touchInterface.ids.contains(id));
    if (d->touchInterface.focus.touch && d->touchInterface.focus.surface) {
        d->touchInterface.focus.touch->up(id, display()->nextSerial());
    } else if (id == 0 && focusedTouchSurface()) {
#if HAVE_LINUX_INPUT_H
        const quint32 serial = display()->nextSerial();
        auto p = d->pointerForSurface(focusedTouchSurface());
        if (!p) {
            return;
        }

        wl_pointer_send_button(p->resource(), serial, timestamp(), BTN_LEFT, WL_POINTER_BUTTON_STATE_RELEASED);
#endif
    }
    d->touchInterface.ids.removeAll(id);
}

void SeatInterface::touchFrame()
{
    Q_D();
    if (d->touchInterface.focus.touch && d->touchInterface.focus.surface) {
        d->touchInterface.focus.touch->frame();
    }
}

bool SeatInterface::isDrag() const
{
    Q_D();
    return d->drag.mode != Private::Drag::Mode::None;
}

bool SeatInterface::isDragPointer() const
{
    Q_D();
    return d->drag.mode == Private::Drag::Mode::Pointer;
}

bool SeatInterface::isDragTouch() const
{
    Q_D();
    return d->drag.mode == Private::Drag::Mode::Touch;
}

bool SeatInterface::hasImplicitPointerGrab(quint32 serial) const
{
    Q_D();
    const auto &serials = d->globalPointer.buttonSerials;
    for (auto it = serials.begin(), end = serials.end(); it != end; it++) {
        if (it.value() == serial) {
            return isPointerButtonPressed(it.key());
        }
    }
    return false;
}

QMatrix4x4 SeatInterface::dragSurfaceTransformation() const
{
    Q_D();
    return d->drag.transformation;
}

SurfaceInterface *SeatInterface::dragSurface() const
{
    Q_D();
    return d->drag.surface;
}

PointerInterface *SeatInterface::dragPointer() const
{
    Q_D();
    if (d->drag.mode != Private::Drag::Mode::Pointer) {
        return nullptr;
    }

    return d->drag.sourcePointer;
}

DataDeviceInterface *SeatInterface::dragSource() const
{
    Q_D();
    return d->drag.source;
}

void SeatInterface::setFocusedTextInputSurface(SurfaceInterface *surface)
{
    Q_D();
    const quint32 serial = d->display->nextSerial();
    const auto old = d->textInput.focus.textInput;
    if (d->textInput.focus.textInput) {
        // TODO: setFocusedSurface like in other interfaces
        d->textInput.focus.textInput->d_func()->sendLeave(serial, d->textInput.focus.surface);
    }
    if (d->textInput.focus.surface) {
        disconnect(d->textInput.focus.destroyConnection);
    }
    d->textInput.focus = Private::TextInput::Focus();
    d->textInput.focus.surface = surface;
    TextInputInterface *t = d->textInputForSurface(surface);
    if (t && !t->resource()) {
        t = nullptr;
    }
    d->textInput.focus.textInput = t;
    if (d->textInput.focus.surface) {
        d->textInput.focus.destroyConnection = connect(surface, &QObject::destroyed, this,
            [this] {
                setFocusedTextInputSurface(nullptr);
            }
        );
        d->textInput.focus.serial = serial;
    }
    if (t) {
        // TODO: setFocusedSurface like in other interfaces
        t->d_func()->sendEnter(surface, serial);
    }
    if (old != t) {
        emit focusedTextInputChanged();
    }
}

SurfaceInterface *SeatInterface::focusedTextInputSurface() const
{
    Q_D();
    return d->textInput.focus.surface;
}

TextInputInterface *SeatInterface::focusedTextInput() const
{
    Q_D();
    return d->textInput.focus.textInput;
}

DataDeviceInterface *SeatInterface::selection() const
{
    Q_D();
    return d->currentSelection;
}

void SeatInterface::setSelection(DataDeviceInterface *dataDevice)
{
    Q_D();
    if (d->currentSelection == dataDevice) {
        return;
    }
    d->currentSelection = dataDevice;
    if (d->keys.focus.selection) {
        if (dataDevice) {
            d->keys.focus.selection->sendSelection(dataDevice);
        } else {
            d->keys.focus.selection->sendClearSelection();
        }
    }
}

}
}
