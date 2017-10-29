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

#ifndef TRANSFERREQUESTMODEL_HPP
#define TRANSFERREQUESTMODEL_HPP

#include <QObject>
#include <QPointer>

class LocalUser;
class PeersList;
class SyfftProtocolSharingRequest;

///
/// \brief The TransferRequestModel class provides the c++ model used by the
/// TransferRequest QML window.
///
/// In particular, the class exposes all the main fields of the transfer request
/// object and the information about the sender user through ad-hoc properties
/// that can be accessed from QML code. A pair of slots are also provided to
/// either accept or reject the request, optionally attaching a textual
/// response.
///
/// In case the user preferences already specify the action to be taken, the
/// requestUser flag is cleared and the action is automatically performed by
/// the class constructor. In this case, the values returned by the properties
/// are undefined.
///
class TransferRequestModel : public QObject
{
    Q_OBJECT
public:
    /// \brief Provides access to whether the sender is anonymous or not.
    Q_PROPERTY(bool anonymous READ anonymous CONSTANT)
    /// \brief Provides access to the sender's names.
    Q_PROPERTY(QString names READ names NOTIFY senderInformationUpdated)
    /// \brief Provides access to whether the senders's icon is set or not.
    Q_PROPERTY(bool iconSet READ iconSet NOTIFY senderInformationUpdated)
    /// \brief Provides access to the senders's icon path.
    Q_PROPERTY(QString iconPath READ iconPath NOTIFY senderInformationUpdated)

    /// \brief Provides access to the total number of files to be received.
    Q_PROPERTY(quint32 filesNumber READ filesNumber CONSTANT)
    /// \brief Provides access to the total size of the files to be received.
    Q_PROPERTY(QString filesSize READ filesSize CONSTANT)
    /// \brief Provides access to the message attached by the peer.
    Q_PROPERTY(QString message READ message CONSTANT)

    /// \brief Provides access to the base path where the received files are
    /// saved.
    Q_PROPERTY(QString dataPath READ dataPath CONSTANT)
    /// \brief Provides access to a value specifying if a folder with the user
    /// name is added to the path when a file is received.
    Q_PROPERTY(bool folderUser READ folderUser CONSTANT)
    /// \brief Provides access to a value specifying if a folder with the
    /// current date is added to the path when a file is received.
    Q_PROPERTY(bool folderDate READ folderDate CONSTANT)

    ///
    /// \brief Builds a new instance of this model.
    /// \param request the object representing the received request.
    /// \param localUser the object representing the local user.
    /// \param peersList the object representing the list of peers.
    /// \param parent the parent of the current object.
    ///
    explicit TransferRequestModel(
        QSharedPointer<SyfftProtocolSharingRequest> request,
        LocalUser *localUser, PeersList *peersList,
        QObject *parent = Q_NULLPTR);

    /// \brief Returns whether it is necessary to request the user the action to
    /// be performed or not.
    bool requestUser() const { return m_requestUser; }

    /// \brief Returns whether the sender is anonymous or not.
    bool anonymous() const;

    /// \brief Returns the sender's names.
    QString names() const { return m_names; }
    /// \brief Returns whether the user's icon is set or not.
    bool iconSet() const { return m_iconSet; }
    /// \brief Returns the user's icon path.
    QString iconPath() const { return m_iconPath; }

    /// \brief Returns the total number of files to be shared.
    quint32 filesNumber() const;
    /// \brief Returns the total size of the files to be shared.
    QString filesSize() const;
    /// \brief Returns the the message attached by the peer.
    QString message() const;

    /// \brief Returns the base path where the received files are saved.
    QString dataPath() const { return m_dataPath; }
    /// \brief Returns a value specifying if a folder with the user name is
    /// added to the path when a file is received.
    bool folderUser() const { return m_folderUser; }
    /// \brief Returns a value specifying if a folder with the current date is
    /// added to the path when a file is received.
    bool folderDate() const { return m_folderDate; }

signals:
    /// \brief Signal emitted when some information of the sender changes.
    void senderInformationUpdated();

    /// \brief Signal emitted when the instance is requested to be destroyed.
    void requestedDestruction();

    /// \brief Signal emitted if the connection is aborted during the choice.
    void connectionAborted();

public slots:
    ///
    /// \brief Accepts the file transfer.
    /// \param dataPath the base path where the received files are saved.
    /// \param folderUser true if a folder with the user name is added to the
    /// path.
    /// \param folderDate true if a folder with the current date is added to the
    /// path.
    /// \param message the message to be attached to the answer.
    /// \param always true if this setting is to be always applied.
    ///
    void accept(const QString &dataPath, bool folderUser, bool folderDate,
                const QString &message, bool always);

    ///
    /// \brief Rejects the file transfer.
    /// \param message the message to be attached to the answer.
    /// \param always true if this setting is to be always applied.
    ///
    void reject(const QString &message, bool always);

    ///
    /// \brief Requests the destruction of the current object.
    ///
    void requestDestruction() { emit requestedDestruction(); }

    ///
    /// \brief Converts an url obtained from a file dialog to an absolute path.
    /// \param url the string representing the url.
    /// \return the absolute path associated to the url.
    ///
    static QString urlToPath(const QString &url);

private:
    /// \brief Updates the user information about the sender.
    void updateSenderInformation();

private:
    /// \brief The instance storing data about the sharing request.
    QSharedPointer<SyfftProtocolSharingRequest> m_request;

    /// \brief The instance storing data about the peers.
    QPointer<PeersList> m_peersList;

    /// \brief Stores whether it is necessary to request the user the action to
    /// be performed or not.
    bool m_requestUser;

    QString m_names; ///< \brief Sender's names.
    /// \brief A value indicating whether the sender's icon is set or not.
    bool m_iconSet;
    QString m_iconPath; ///< \brief Sender's icon path.

    /// \brief The base path where the received files are saved.
    QString m_dataPath;
    /// \brief Specifies if a folder with the user name is added to the path.
    bool m_folderUser;
    /// \brief Specifies if a folder with the current date is added to the path.
    bool m_folderDate;
};

#endif // TRANSFERREQUESTMODEL_HPP
