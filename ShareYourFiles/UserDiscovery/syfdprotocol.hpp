/*
 *  Copyright (c) 2017 Marco Iorio (giorio94 at gmail dot com)
 *  This file is part of Share Your Files (SYF).
 *
 *  SYF is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  SYF is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.

 *  You should have received a copy of the GNU General Public License
 *  along with SYF.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SYFDPROTOCOL_HPP
#define SYFDPROTOCOL_HPP

#include "Common/common.hpp"
#include "Common/networkentrieslist.hpp"

#include <QHostAddress>
#include <QObject>
#include <QPointer>

class QByteArray;
class QUdpSocket;
class QTimer;

class SyfdDatagram;

///
/// \brief The SyfdProtocol class provides an implementation of the SYFD
/// protocol.
///
/// The SYFD (Share Your Files Discovery) Protocol is the network protocol used
/// to allow different instances of Share Your Files operating on different
/// hosts to know each other, in order to allow providing the needed file
/// sharing services.
///
/// Given the nature of the advertisement and the use limited to LAN, it has
/// been chosen to use the UDP protocol to carry the messages, in order to be
/// able to exploit the multicast functionalities. More in detail, a specific
/// multicast group (IPv4 address and UDP port) is used to send periodically the
/// SyfdDatagram representing the local user and, at the same time, to receive
/// the datagrams representing the other peers. Therefore this class implements
/// both the sending and the receiving side of the protocol.
///
/// \see SyfdDatagram
///
class SyfdProtocol : public QObject
{
    Q_OBJECT

public:
    ///
    /// \brief The Status enum describes the status the SyfdProtocol is
    /// currently in.
    ///
    enum class Status {
        Started, ///< \brief The protocol is operating.
        Stopped  ///< \brief The protocol is stopped.
    };
    Q_ENUM(Status)

    ///
    /// \brief Constructs a new instance of the protocol.
    /// \param entry the pair containing the network interface and the IPv4
    /// address to be used.
    /// \param parent the parent of the current object.
    ///
    explicit SyfdProtocol(const NetworkEntriesList::Entry &entry,
                          QObject *parent = Q_NULLPTR);

    ///
    /// \brief Stops the SyfdProtocol and releases the resources.
    ///
    ~SyfdProtocol();

    ///
    /// \brief Returns the current status of the SyfdProtocol.
    /// \see Status
    ///
    Status status() const { return m_status; }

    ///
    /// \brief Returns whether the SyfdProtocol instance is valid or not.
    ///
    /// In case of invalid instance, that is when the initialization
    /// of the sockets fail, it is not possible to start the protocol.
    ///
    bool valid() const { return m_valid; }

public slots:
    ///
    /// \brief Starts the SyfdProtocol.
    /// \param mode the operational mode chosen to be entered.
    /// \param datagram the datagram to be advertised by the protocol.
    ///
    void start(Enums::OperationalMode mode, const SyfdDatagram &datagram);

    ///
    /// \brief Stops the SyfdProtocol.
    ///
    void stop();

    ///
    /// \brief Sets the mode in which the SyfdProtocol operates.
    /// \param mode the mode the protocol has to switch to.
    /// \param emitSignal specifies whether the modeChanged() signal is emitted
    /// or not.
    ///
    void setMode(Enums::OperationalMode mode, bool emitSignal = true);

    ///
    /// \brief Updates the buffered datagram.
    /// \param datagram the new SYFD datagram instance.
    ///
    void updateDatagram(const SyfdDatagram &datagram);

signals:
    ///
    /// \brief Signal emitted when the protocol is started.
    ///
    void started();

    ///
    /// \brief Signal emitted when the protocol is stopped.
    ///
    void stopped();

    ///
    /// \brief Signal emitted when the operational mode changes.
    /// \param mode the entered mode.
    ///
    void modeChanged(Enums::OperationalMode mode);

    ///
    /// \brief Signal emitted when a datagram is received.
    /// \param datagram the received SYFD datagram.
    ///
    void datagramReceived(const SyfdDatagram &datagram);

    ///
    /// \brief Signal emitted when the protocol fails sending datagrams.
    ///
    void error();

private:
    ///
    /// \brief Sends a datagram containing the data specified.
    /// \param datagram data to be sent.
    ///
    void sendDatagram(const QByteArray &datagram);

    ///
    /// \brief Sends the buffered datagram advertising the local user.
    ///
    void sendBufferedDatagram();

    ///
    /// \brief Sends a datagram with the quit flag set.
    ///
    void sendQuitDatagram();

    ///
    /// \brief Performs datagram reception and dispatching.
    ///
    void receiveDatagram();

    ///
    /// \brief Updates the buffered datagram.
    /// \return true in case of success and false in case of failure.
    ///
    bool updateDatagram();

private:
    /// \brief The IPv4 multicast address used by this protocol.
    static const QHostAddress SYFD_ADDRESS;
    /// \brief The UDP port used by this protocol.
    static const quint16 SYFD_PORT = 10101;
    /// \brief Interval between execution of datagram shipping and aging.
    static const int SYDF_INTERVAL = 5000;
    /// \brief The number of errors before the error() signal is emitted.
    static const int ERROR_THRESHOLD = 3;


    /// \brief Specifies whether the SyfdProtocol instance is valid or not.
    bool m_valid;

    /// \brief Specifies the current status of the protocol.
    Status m_status;
    /// \brief Specifies the current mode the protocol is in.
    Enums::OperationalMode m_mode;

    QPointer<QUdpSocket> m_sender;   ///< \brief Sender socket.
    QPointer<QUdpSocket> m_receiver; ///< \brief Receiver socket.

    /// \brief Timer used for datagram shipping and aging.
    QPointer<QTimer> m_timer;

    /// \brief Local IPv4 address used to send the datagrams.
    quint32 m_localAddress;
    /// \brief UDP port used to send the datagrams.
    quint16 m_localPort;

    /// \brief The datagram instance representing the local user.
    QScopedPointer<SyfdDatagram> m_datagram;
    /// \brief Buffer containing a copy of the datagram representing the local
    /// user.
    QScopedPointer<QByteArray> m_datagramBuffer;

    /// \brief The number of errors occurred while sending datagrams.
    int m_errorCount;
};

#endif // SYFDPROTOCOL_HPP
