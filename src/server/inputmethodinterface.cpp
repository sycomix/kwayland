/********************************************************************
Copyright 2019 Aleix Pol Gonzalez <aleixpol@kde.org>

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
#include "inputmethodinterface.h"
#include "resource_p.h"
#include "seat_interface.h"
#include "display.h"
#include "surface_interface.h"

#include <QHash>
#include "qwayland-server-input-method-unstable-v1.h"

using namespace KWayland;
using namespace Server;

static int s_version = 1;

using namespace KWayland::Server;

class InputMethodInterface::Private : public QtWaylandServer::zwp_input_method_v1
{
public:

};

InputMethodInterface::InputMethodInterface()
    : d(new InputMethodInterface::Private)
{
}

InputMethodInterface::~InputMethodInterface() = default;

class InputMethodContextInterface::Private : public QtWaylandServer::zwp_input_method_context_v1
{
public:
    Private(InputMethodContextInterface* q) : q(q) {}

    void zwp_input_method_context_v1_commit_string(QtWaylandServer::zwp_input_method_context_v1::Resource * resource, uint32_t serial, const QString & text) override {
        Q_EMIT q->commitString(resource->handle, serial, text);
    }
    void zwp_input_method_context_v1_preedit_string(Resource *resource, uint32_t serial, const QString &text, const QString &commit) override {
        Q_EMIT q->preeditString(resource->handle, serial, text, commit);
    }

    void zwp_input_method_context_v1_preedit_styling(Resource *resource, uint32_t index, uint32_t length, uint32_t style) override {
        Q_EMIT q->preeditStyling(resource->handle, index, length, style);
    }
    void zwp_input_method_context_v1_preedit_cursor(Resource *resource, int32_t index) override {
        Q_EMIT q->preeditCursor(resource->handle, index);
    }
    void zwp_input_method_context_v1_delete_surrounding_text(Resource *resource, int32_t index, uint32_t length) override {
        Q_EMIT q->deleteSurroundingText(resource->handle, index, length);
    }
    void zwp_input_method_context_v1_cursor_position(Resource *resource, int32_t index, int32_t anchor) override {
        Q_EMIT q->cursorPosition(resource->handle, index, anchor);
    }
//     void zwp_input_method_context_v1_modifiers_map(Resource *resource, wl_array *map) override {
//         Q_EMIT q->modifiersMap(resource->handle, map);
//     }
    void zwp_input_method_context_v1_keysym(Resource *resource, uint32_t serial, uint32_t time, uint32_t sym, uint32_t state, uint32_t modifiers) override {
        Q_EMIT q->keysym(resource->handle, serial, time, sym, state, modifiers);
    }
    void zwp_input_method_context_v1_grab_keyboard(Resource *resource, uint32_t keyboard) override {
        Q_EMIT q->grabKeyboard(resource->handle, keyboard);
    }
    void zwp_input_method_context_v1_key(Resource *resource, uint32_t serial, uint32_t time, uint32_t key, uint32_t state) override {
        Q_EMIT q->key(resource->handle, serial, time, key, state);
    }
    void zwp_input_method_context_v1_modifiers(Resource *resource, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group) override {
        Q_EMIT q->modifiers(resource->handle, serial, mods_depressed, mods_latched, mods_locked, group);
    }
    void zwp_input_method_context_v1_language(Resource *resource, uint32_t serial, const QString &language) override {
        Q_EMIT q->language(resource->handle, serial, language);
    }
    void zwp_input_method_context_v1_text_direction(Resource *resource, uint32_t serial, uint32_t direction) override {
        Q_EMIT q->textDirection(resource->handle, serial, direction);
    }
private:
    InputMethodContextInterface* const q;
};

InputMethodContextInterface::InputMethodContextInterface()
    : d(new InputMethodContextInterface::Private(this))
{
}

InputMethodContextInterface::~InputMethodContextInterface() = default;

class InputPanelInterface::Private : public QtWaylandServer::zwp_input_panel_v1
{
public:

};

InputPanelInterface::InputPanelInterface()
    : d(new InputPanelInterface::Private)
{
}

InputPanelInterface::~InputPanelInterface() = default;

class InputPanelSurfaceInterface::Private : public QtWaylandServer::zwp_input_method_v1
{
public:

};

InputPanelSurfaceInterface::InputPanelSurfaceInterface()
    : d(new InputPanelSurfaceInterface::Private)
{
}

InputPanelSurfaceInterface::~InputPanelSurfaceInterface() = default;
