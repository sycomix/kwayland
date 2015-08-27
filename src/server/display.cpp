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
#include "display.h"
#include "compositor_interface.h"
#include "datadevicemanager_interface.h"
#include "idle_interface.h"
#include "fakeinput_interface.h"
#include "logging_p.h"
#include "output_interface.h"
#include "plasmashell_interface.h"
#include "plasmawindowmanagement_interface.h"
#include "qtsurfaceextension_interface.h"
#include "seat_interface.h"
#include "shadow_interface.h"
#include "blur_interface.h"
#include "shell_interface.h"
#include "subcompositor_interface.h"

#include <QCoreApplication>
#include <QDebug>
#include <QAbstractEventDispatcher>
#include <QSocketNotifier>
#include <QThread>

#include <wayland-server.h>

#include <EGL/egl.h>

namespace KWayland
{
namespace Server
{

class Display::Private
{
public:
    Private(Display *q);
    void flush();
    void dispatch();
    void setRunning(bool running);
    void installSocketNotifier();

    wl_display *display = nullptr;
    wl_event_loop *loop = nullptr;
    QString socketName = QStringLiteral("wayland-0");
    bool running = false;
    QList<OutputInterface*> outputs;
    QVector<ClientConnection*> clients;
    EGLDisplay eglDisplay = EGL_NO_DISPLAY;

private:
    Display *q;
};

Display::Private::Private(Display *q)
    : q(q)
{
}

void Display::Private::installSocketNotifier()
{
    if (!QThread::currentThread()) {
        return;
    }
    int fd = wl_event_loop_get_fd(loop);
    if (fd == -1) {
        qCWarning(KWAYLAND_SERVER) << "Did not get the file descriptor for the event loop";
        return;
    }
    QSocketNotifier *m_notifier = new QSocketNotifier(fd, QSocketNotifier::Read, q);
    QObject::connect(m_notifier, &QSocketNotifier::activated, q, [this] { dispatch(); } );
    QObject::connect(QThread::currentThread()->eventDispatcher(), &QAbstractEventDispatcher::aboutToBlock, q, [this] { flush(); });
    setRunning(true);
}

Display::Display(QObject *parent)
    : QObject(parent)
    , d(new Private(this))
{
}

Display::~Display()
{
    terminate();
}

void Display::Private::flush()
{
    if (!display || !loop) {
        return;
    }
    wl_display_flush_clients(display);
}

void Display::Private::dispatch()
{
    if (!display || !loop) {
        return;
    }
    if (wl_event_loop_dispatch(loop, 0) != 0) {
        qCWarning(KWAYLAND_SERVER) << "Error on dispatching Wayland event loop";
    }
}

void Display::setSocketName(const QString &name)
{
    if (d->socketName == name) {
        return;
    }
    d->socketName = name;
    emit socketNameChanged(d->socketName);
}

QString Display::socketName() const
{
    return d->socketName;
}

void Display::start(StartMode mode)
{
    Q_ASSERT(!d->running);
    Q_ASSERT(!d->display);
    d->display = wl_display_create();
    if (mode == StartMode::ConnectToSocket) {
        if (wl_display_add_socket(d->display, qPrintable(d->socketName)) != 0) {
            return;
        }
    }

    d->loop = wl_display_get_event_loop(d->display);
    d->installSocketNotifier();
}

void Display::startLoop()
{
    Q_ASSERT(!d->running);
    Q_ASSERT(d->display);
    d->installSocketNotifier();
}

void Display::dispatchEvents(int msecTimeout)
{
    Q_ASSERT(d->display);
    if (d->running) {
        d->dispatch();
    } else {
        wl_event_loop_dispatch(d->loop, msecTimeout);
        wl_display_flush_clients(d->display);
    }
}

void Display::terminate()
{
    if (!d->running) {
        return;
    }
    emit aboutToTerminate();
    wl_display_terminate(d->display);
    wl_display_destroy(d->display);
    d->display = nullptr;
    d->loop = nullptr;
    d->setRunning(false);
}

void Display::Private::setRunning(bool r)
{
    Q_ASSERT(running != r);
    running = r;
    emit q->runningChanged(running);
}

OutputInterface *Display::createOutput(QObject *parent)
{
    OutputInterface *output = new OutputInterface(this, parent);
    connect(output, &QObject::destroyed, this, [this,output] { d->outputs.removeAll(output); });
    connect(this, &Display::aboutToTerminate, output, [this,output] { removeOutput(output); });
    d->outputs << output;
    return output;
}

CompositorInterface *Display::createCompositor(QObject *parent)
{
    CompositorInterface *compositor = new CompositorInterface(this, parent);
    connect(this, &Display::aboutToTerminate, compositor, [this,compositor] { delete compositor; });
    return compositor;
}

ShellInterface *Display::createShell(QObject *parent)
{
    ShellInterface *shell = new ShellInterface(this, parent);
    connect(this, &Display::aboutToTerminate, shell, [this,shell] { delete shell; });
    return shell;
}

SeatInterface *Display::createSeat(QObject *parent)
{
    SeatInterface *seat = new SeatInterface(this, parent);
    connect(this, &Display::aboutToTerminate, seat, [this,seat] { delete seat; });
    return seat;
}

SubCompositorInterface *Display::createSubCompositor(QObject *parent)
{
    auto c = new SubCompositorInterface(this, parent);
    connect(this, &Display::aboutToTerminate, c, [this,c] { delete c; });
    return c;
}

DataDeviceManagerInterface *Display::createDataDeviceManager(QObject *parent)
{
    auto m = new DataDeviceManagerInterface(this, parent);
    connect(this, &Display::aboutToTerminate, m, [this,m] { delete m; });
    return m;
}

PlasmaShellInterface *Display::createPlasmaShell(QObject* parent)
{
    auto s = new PlasmaShellInterface(this, parent);
    connect(this, &Display::aboutToTerminate, s, [this, s] { delete s; });
    return s;
}

PlasmaWindowManagementInterface *Display::createPlasmaWindowManagement(QObject *parent)
{
    auto wm = new PlasmaWindowManagementInterface(this, parent);
    connect(this, &Display::aboutToTerminate, wm, [this, wm] { delete wm; });
    return wm;
}

QtSurfaceExtensionInterface *Display::createQtSurfaceExtension(QObject *parent)
{
    auto s = new QtSurfaceExtensionInterface(this, parent);
    connect(this, &Display::aboutToTerminate, s, [this, s] { delete s; });
    return s;
}

IdleInterface *Display::createIdle(QObject *parent)
{
    auto i = new IdleInterface(this, parent);
    connect(this, &Display::aboutToTerminate, i, [this, i] { delete i; });
    return i;
}

FakeInputInterface *Display::createFakeInput(QObject *parent)
{
    auto i = new FakeInputInterface(this, parent);
    connect(this, &Display::aboutToTerminate, i, [this, i] { delete i; });
    return i;
}

ShadowManagerInterface *Display::createShadowManager(QObject *parent)
{
    auto s = new ShadowManagerInterface(this, parent);
    connect(this, &Display::aboutToTerminate, s, [this, s] { delete s; });
    return s;
}

BlurManagerInterface *Display::createBlurManager(QObject *parent)
{
    auto b = new BlurManagerInterface(this, parent);
    connect(this, &Display::aboutToTerminate, b, [this, b] { delete b; });
    return b;
}

void Display::createShm()
{
    Q_ASSERT(d->display);
    wl_display_init_shm(d->display);
}

void Display::removeOutput(OutputInterface *output)
{
    d->outputs.removeAll(output);
    delete output;
}

quint32 Display::nextSerial()
{
    return wl_display_next_serial(d->display);
}

quint32 Display::serial()
{
    return wl_display_get_serial(d->display);
}

bool Display::isRunning() const
{
    return d->running;
}

Display::operator wl_display*()
{
    return d->display;
}

Display::operator wl_display*() const
{
    return d->display;
}

QList< OutputInterface* > Display::outputs() const
{
    return d->outputs;
}

ClientConnection *Display::getConnection(wl_client *client)
{
    Q_ASSERT(client);
    auto it = std::find_if(d->clients.constBegin(), d->clients.constEnd(),
        [client](ClientConnection *c) {
            return c->client() == client;
        }
    );
    if (it != d->clients.constEnd()) {
        return *it;
    }
    // no ConnectionData yet, create it
    auto c = new ClientConnection(client, this);
    d->clients << c;
    connect(c, &ClientConnection::disconnected, this,
        [this] (ClientConnection *c) {
            const int index = d->clients.indexOf(c);
            Q_ASSERT(index != -1);
            d->clients.remove(index);
            Q_ASSERT(d->clients.indexOf(c) == -1);
            emit clientDisconnected(c);
        }
    );
    emit clientConnected(c);
    return c;
}

QVector< ClientConnection* > Display::connections() const
{
    return d->clients;
}

ClientConnection *Display::createClient(int fd)
{
    Q_ASSERT(fd != -1);
    Q_ASSERT(d->display);
    wl_client *c = wl_client_create(d->display, fd);
    if (!c) {
        return nullptr;
    }
    return getConnection(c);
}

void Display::setEglDisplay(void *display)
{
    if (d->eglDisplay != EGL_NO_DISPLAY) {
        qCWarning(KWAYLAND_SERVER) << "EGLDisplay cannot be changed";
        return;
    }
    d->eglDisplay = (EGLDisplay)display;
}

void *Display::eglDisplay() const
{
    return d->eglDisplay;
}

}
}
