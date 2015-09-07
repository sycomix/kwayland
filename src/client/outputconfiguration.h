/********************************************************************
Copyright 2013  Martin Gräßlin <mgraesslin@kde.org>
Copyright 2015  Sebastian Kügler <sebas@kde.org>

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
#ifndef WAYLAND_KWIN_OUTPUTCONFIGURATION_H
#define WAYLAND_KWIN_OUTPUTCONFIGURATION_H

#include <QObject>
#include <QPointer>
#include <QSize>

#include <KWayland/Client/kwaylandclient_export.h>

struct org_kde_kwin_outputconfiguration;
class QPoint;
class QRect;

namespace KWayland
{
namespace Client
{
class OutputConfiguration;
class OutputDevice;
class EventQueue;

/**
 * @short Wrapper for the org_kde_kwin_outputconfiguration interface.
 *
 * This class provides a convenient wrapper for the org_kde_kwin_outputconfiguration interface.
 * Its main purpose is to provide information about connected, but disabled screens, i.e.
 * outputs that are not visible in the wl_output interface, but could be enabled by the
 * compositor.
 *
 * To use this class one needs to interact with the Registry. There are two
 * possible ways to create an KWinOutputConfiguration interface:
 * @code
 * KWayland::Client::OutputConfiguration *c = registry->createOutputConfiguration(name, version);
 * @endcode
 *
 * This creates the KWinOutputConfiguration and sets it up directly. As an alternative this
 *
 * The OutputConfiguration can be used as a drop-in replacement for any
 * org_kde_kwin_outputconfiguration * pointer as it provides matching cast operators.
 *
 * Please note that all properties of OutputConfiguration are not valid until the
 * done signal has been emitted. The wayland server is pushing the
 * information in an async way to the OutputConfiguration instance. By emitting done,
 * the OutputConfiguration indicates that all relevant information is available.
 *
 * @see Registry
 **/
class KWAYLANDCLIENT_EXPORT OutputConfiguration : public QObject
{
    Q_OBJECT
public:
    explicit OutputConfiguration(QObject *parent = nullptr);
    virtual ~OutputConfiguration();

    /**
     * Setup this OutputConfiguration.
     * When using Registry::createOutputConfiguration there is no need to call this
     * method.
     **/
    void setup(org_kde_kwin_outputconfiguration *output_management);

    void release();
    void destroy();

    /**
     * Sets the @p queue to use for creating a Surface.
     **/
    void setEventQueue(EventQueue *queue);

    /**
     * @returns The event queue to use for creating a Surface.
     **/
    EventQueue *eventQueue();

    void enable(OutputDevice *output, bool on);

    /**
     * @returns @c true if managing a org_kde_kwin_outputconfiguration.
     **/
    bool isValid() const;
    operator org_kde_kwin_outputconfiguration*();
    operator org_kde_kwin_outputconfiguration*() const;

Q_SIGNALS:
    /**
     * This signal is emitted after the configuration has been applied.
     **/
    void applied();

    /**
     * This signal is emitted right before the interface is released.
     **/
    void interfaceAboutToBeReleased();
    /**
     * This signal is emitted right before the data is destroyed.
     **/
    void interfaceAboutToBeDestroyed();
    
    /**
     * The corresponding global for this interface on the Registry got removed.
     *
     * This signal gets only emitted if the OutputConfiguration got created by
     * Registry::createOutputConfiguration
     *
     * @since 5.5
     **/
    void removed();

private:
    class Private;
    QScopedPointer<Private> d;
};

}
}

#endif