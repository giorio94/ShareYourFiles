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

#ifndef TRANSFERRESPOSEMODEL_HPP
#define TRANSFERRESPOSEMODEL_HPP

#include <QObject>
#include <QPointer>

class PeersList;

///
/// \brief The TransferResponseModel class provides the c++ model used by the
/// TransferResponse QML window.
///
/// In particular, the class exposes the information about the other user,
/// along with the provided answer through ad-hoc properties that can be
/// accessed from QML code.
///
class TransferResponseModel : public QObject
{
    Q_OBJECT
public:
    /// \brief Provides access to the sender's names.
    Q_PROPERTY(QString names READ names NOTIFY senderInformationUpdated)
    /// \brief Provides access to whether the senders's icon is set or not.
    Q_PROPERTY(bool iconSet READ iconSet NOTIFY senderInformationUpdated)
    /// \brief Provides access to the senders's icon path.
    Q_PROPERTY(QString iconPath READ iconPath NOTIFY senderInformationUpdated)

    /// \brief Provides access to whether the request has been accepted or not.
    Q_PROPERTY(bool accepted READ accepted CONSTANT)
    /// \brief Provides access to the message attached by the peer.
    Q_PROPERTY(QString message READ message CONSTANT)

    ///
    /// \brief Builds a new instance of this model.
    /// \param uuid the the identifier of the sender peer.
    /// \param peersList the object representing the list of peers.
    /// \param accepted whether the request has been accepted or not.
    /// \param message the message attached to the response.
    /// \param parent the parent of the current object.
    ///
    explicit TransferResponseModel(const QString &uuid, PeersList *peersList,
                                   bool accepted, const QString &message,
                                   QObject *parent = Q_NULLPTR);

    /// \brief Returns the sender's names.
    QString names() const { return m_names; }
    /// \brief Returns whether the user's icon is set or not.
    bool iconSet() const { return m_iconSet; }
    /// \brief Returns the user's icon path.
    QString iconPath() const { return m_iconPath; }

    /// \brief Returns whether the request has been accepted or not.
    bool accepted() const { return m_accepted; }
    /// \brief Returns the the message attached by the peer.
    QString message() const { return m_message; }

signals:
    /// \brief Signal emitted when some information of the sender changes.
    void senderInformationUpdated();

    /// \brief Signal emitted when the instance is requested to be destroyed.
    void requestedDestruction();

    /// \brief Signal emitted if the connection is aborted during the choice.
    void connectionAborted();

public slots:
    ///
    /// \brief Requests the destruction of the current object.
    ///
    void requestDestruction() { emit requestedDestruction(); }

private:
    /// \brief Updates the user information about the sender.
    void updateSenderInformation();

private:
    QString m_uuid;  ///< \brief Sender's identifier.
    QString m_names; ///< \brief Sender's names.
    /// \brief A value indicating whether the sender's icon is set or not.
    bool m_iconSet;
    QString m_iconPath; ///< \brief Sender's icon path.

    /// \brief Whether the request has been accepted or not.
    bool m_accepted;
    /// \brief The message attached by the peer.
    QString m_message;

    /// \brief The instance storing data about the peers.
    QPointer<PeersList> m_peersList;
};

#endif // TRANSFERRESPOSEMODEL_HPP
