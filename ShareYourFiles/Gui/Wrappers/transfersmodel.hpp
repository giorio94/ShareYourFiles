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

#ifndef SYFFTINSTANCESMODEL_HPP
#define SYFFTINSTANCESMODEL_HPP

#include "FileTransfer/syfftprotocolcommon.hpp"
#include "FileTransfer/transferinfo.hpp"

#include <QAbstractListModel>
#include <QPointer>

class LocalUser;
class PeersList;
class SyfftProtocolSender;
class SyfftProtocolReceiver;
class SyfftProtocolSharingRequest;
class SyfftProtocolDuplicatedFile;

class DuplicatedFileModel;
class TransferRequestModel;
class TransferResponseModel;

class QTimer;

///
/// \brief The TransfersModel class provides the c++ model used by the
/// Transfers QML window.
///
/// In particular, the class extends the QAbstractListModel class and provides
/// access to the main properties of the ongoing transfers. It also acts as a
/// proxy intercepting the sharing requests and duplicated file detected ones
/// and re-emitting them after having marshalled the request instance in order
/// to make it accessible from QML.
///
class TransfersModel : public QAbstractListModel
{
    Q_OBJECT

public:
    /// \brief Provides access to the number of elements in the model.
    Q_PROPERTY(int rowCount READ rowCount NOTIFY rowCountChanged)

    ///
    /// \brief The Roles enum represents the possible properties that can be
    /// queried for each element of the model.
    ///
    enum Roles {
        NamesRole = Qt::UserRole + 1, ///< \brief Peer's names.
        IconSetRole,  ///< \brief Whether the peer's icon is set or not.
        IconPathRole, ///< \brief Peer's icon path.

        SenderRole,     ///< \brief Whether the files are sent or received.
        StatusRole,     ///< \brief The current status of the transfer.
        InTransferRole, ///< \brief Whether the instance is in transfer or not.
        ClosedRole,     ///< \brief Whether the connection is closed or not.
        PausedRole,     ///< \brief Whether the connection is paused or not.
        PercentageRole, ///< \brief The transfer completion percentage.
        FilenameRole,   ///< \brief The name of the file in transfer.
        SpeedRole,      ///< \brief The current transfer speed.
        AvgSpeedRole,   ///< \brief The average transfer speed.

        TotalNumberRole,   ///< \brief The total number of files in transfer.
        TotalSizeRole,     ///< \brief The total size of the files in transfer.
        RemainingTimeRole, ///< \brief The remaining transfer time.
        RemainingNumberRole, ///< \brief The remaining number of files.
        RemainingSizeRole,   ///< \brief The remaining size of the files.
        SkippedNumberRole,   ///< \brief The number of skipped files.
        SkippedSizeRole      ///< \brief The size of the skipped files.
    };

    ///
    /// \brief Initializes a new instance of the model.
    /// \param localUser the object representing the local user.
    /// \param peersList the object representing the list of peers.
    /// \param parent the parent of the current object.
    ///
    TransfersModel(LocalUser *localUser, PeersList *peersList,
                   QObject *parent = Q_NULLPTR);

    ///
    /// \brief Returns the number of rows in the model.
    /// \param parent unused.
    /// \return the number of elements stored.
    ///
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    ///
    /// \brief Returns the data stored under the given role for the item
    /// referred to by the index.
    /// \param index the identifier of the requested item.
    /// \param role the identifier of the requested role.
    /// \return the requested value or an invalid QVariant.
    ///
    QVariant data(const QModelIndex &index,
                  int role = Qt::DisplayRole) const override;

    ///
    /// \brief Adds a new SYFFT protocol instance to the model.
    /// \param instance the instance to be added.
    ///
    void addSyfftInstance(QSharedPointer<SyfftProtocolSender> instance);

    ///
    /// \brief Adds a new SYFFT protocol instance to the model.
    /// \param instance the instance to be added.
    ///
    void addSyfftInstance(QSharedPointer<SyfftProtocolReceiver> instance);

    ///
    /// \brief Returns the number of ongoing transfers.
    ///
    int ongoingTransfers();

    ///
    /// \brief Returns the number of ongoing receptions.
    ///
    int ongoingReceptions();

public slots:
    ///
    /// \brief Sets the pause mode of the connection.
    /// \param index the index of the element to be changed.
    /// \param setPause specifies whether it is requested to enter or exit
    /// the
    /// pause mode.
    ///
    void pauseConnection(const int index, bool setPause);

    ///
    /// \brief Aborts the connection.
    /// \param index the index of the element to be changed.
    ///
    void abortConnection(const int index);

    ///
    /// \brief Deletes the connection.
    /// \param index the index of the element to be changed.
    ///
    void deleteConnection(const int index);

signals:
    ///
    /// \brief Signal emitted when it is necessary to ask the user about the
    /// transfer request (to be managed by the QML side).
    /// \param request the object storing the information about the request.
    ///
    void transferRequestedAsk(TransferRequestModel *request);

    ///
    /// \brief Signal emitted when a response to a transfer request is received.
    /// \param response the object storing the information about the response.
    ///
    void transferResponseReceived(TransferResponseModel *response);

    ///
    /// \brief Signal emitted when a duplicated file is detected.
    /// \param request the object storing the information about the conflict.
    ///
    void duplicatedFileDetected(DuplicatedFileModel *request);

    ///
    /// \brief Signal emitted when the number of elements in the model
    /// changes.
    ///
    void rowCountChanged();

protected:
    ///
    /// \brief Adds a new SYFFT protocol instance to the model.
    /// \param instance the instance to be added.
    ///
    void addSyfftInstance(QSharedPointer<SyfftProtocolCommon> instance);

    ///
    /// \brief Returns the model's role names.
    ///
    QHash<int, QByteArray> roleNames() const override;

private slots:
    ///
    /// \brief Manages a new transfer request.
    /// \param request the object storing the information about the request.
    ///
    void transferRequested(QSharedPointer<SyfftProtocolSharingRequest> request);

    ///
    /// \brief Function executed when the response to a transfer request is
    /// received.
    /// \param uuid the the identifier of the sender peer.
    /// \param accepted whether the request has been accepted or not.
    /// \param message the message attached to the response.
    ///
    void responseReceived(const QString &uuid, bool accepted,
                          const QString &message);

    ///
    /// \brief Manages a duplicated file conflict.
    /// \param request the object storing information about the conflict.
    ///
    void duplicatedFile(QSharedPointer<SyfftProtocolDuplicatedFile> request);

private:
    /// \brief The instance storing data about the local user.
    QPointer<LocalUser> m_localUser;
    /// \brief The instance storing data about the peers.
    QPointer<PeersList> m_peersList;

    /// \brief The list of SYFFT protocol instances managed by the model.
    QList<QSharedPointer<SyfftProtocolCommon>> m_instances;

    /// \brief The cached transfer informations.
    QList<TransferInfo> m_transferInformations;

    /// \brief The timer used to update the transfer information.
    QPointer<QTimer> m_timerUpdate;

    /// \brief The interval between transfer information updates.
    static const int UPDATE_INTERVAL = 500;

    /// \brief Provides an association between ReceptionPreferences::Action and
    /// the corresponding textual string.
    static const QMap<SyfftProtocolCommon::Status, QString> STATUS;
};

#endif // SYFFTINSTANCESMODEL_HPP
