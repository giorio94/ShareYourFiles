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

#ifndef PEERSSELECTORMODEL_HPP
#define PEERSSELECTORMODEL_HPP

#include "UserDiscovery/userinfo.hpp"

#include <QAbstractListModel>
#include <QPointer>

class PeersList;

///
/// \brief The PeersSelectorModel class provides the c++ model used by the
/// PeersSelector QML window.
///
/// In particular, the class extends the QAbstractListModel class and provides
/// access to the main properties of the peers currently online (e.g. names and
/// icon), with the possibility to select a subset of them. Some properties are
/// also available to access the information about the file transfer ready to be
/// started. A method is provided to complete the selection, by emitting the
/// selectionCompleted(), and getters can be used to access the list of
/// selected peers and the message specified.
///
class PeersSelectorModel : public QAbstractListModel
{
    Q_OBJECT

public:
    /// \brief Provides access to the number of elements in the model.
    Q_PROPERTY(int rowCount READ rowCount NOTIFY rowCountChanged)
    /// \brief Provides access to the number of selected elements in the model.
    Q_PROPERTY(int selectedCount READ selectedCount NOTIFY selectedCountChanged)

    /// \brief Provides access to the total number of files to be shared.
    Q_PROPERTY(quint32 filesNumber READ filesNumber CONSTANT)
    /// \brief Provides access to the total size of the files to be shared.
    Q_PROPERTY(QString filesSize READ filesSize CONSTANT)

    ///
    /// \brief The Roles enum represents the possible properties that can be
    /// queried for each element of the model.
    ///
    enum Roles {
        FirstNameRole = Qt::UserRole + 1, ///< \brief First name.
        LastNameRole,                     ///< \brief Last name.
        IconSetRole,  ///< \brief Whether the icon is set or not.
        IconPathRole, ///< \brief Icon's path.
        SelectedRole  ///< \brief User selected or not.
    };

    ///
    /// \brief Initializes a new instance of the model.
    /// \param filesNumber the total number of files to be shared.
    /// \param filesSize the total size of the files to be shared.
    /// \param source the data source associated to the model.
    /// \param parent the parent of the current object.
    ///
    explicit PeersSelectorModel(quint32 filesNumber, const QString &filesSize,
                                PeersList *source, QObject *parent = Q_NULLPTR);

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
    /// \brief Returns the number of selected elements in the model.
    /// \return the number of selected items.
    ///
    int selectedCount() const { return m_selectedCount; }

    ///
    /// \brief Returns the list of selected peers.
    /// \return a list containing the identifiers of the users.
    ///
    QStringList selectedItems() const;

    /// \brief Returns the total number of files to be shared.
    quint32 filesNumber() const { return m_filesNumber; }
    /// \brief Returns the total size of the files to be shared.
    QString filesSize() const { return m_filesSize; }

    /// \brief Returns the message to be attached to the request.
    QString message() const { return m_message; }

public slots:
    ///
    /// \brief Toggles the selected flag associated to the peer.
    /// \param index the index of the element to be changed.
    ///
    void toggleSelected(const int index);

    ///
    /// \brief Sets the message to be attached to the request.
    /// \param message the message written by the user.
    ///
    void setMessage(const QString &message) { m_message = message; }

    ///
    /// \brief Completes the selection by emitting the selectionCompleted()
    /// signal.
    /// \param confirm whether the selection has been confirmed or canceled.
    ///
    void completeSelection(bool confirm);

signals:
    ///
    /// \brief Signal emitted when the number of elements in the model changes.
    ///
    void rowCountChanged();

    ///
    /// \brief Signal emitted when the number of selected elements in the model
    /// changes.
    ///
    void selectedCountChanged();

    ///
    /// \brief Signal emitted when the selection is completed.
    /// \param confirmed whether the selection has been confirmed or canceled.
    ///
    void selectionCompleted(bool confirmed);

protected:
    ///
    /// \brief Returns the model's role names.
    ///
    QHash<int, QByteArray> roleNames() const override;

private:
    ///
    /// \brief Adds a peer to the list.
    /// \param uuid the identifier of the peer to be added.
    ///
    void addPeer(const QString &uuid);

    ///
    /// \brief Removes a peer from the list.
    /// \param uuid the identifier of the peer to be removed.
    ///
    void removePeer(const QString &uuid);

    ///
    /// \brief Removes a peer stored in the list.
    /// \param uuid the identifier of the peer to be updated.
    ///
    void updatePeer(const QString &uuid);

    ///
    /// \brief Finds the index in the array of the specifies peer.
    /// \param uuid the identifier of the requested peer.
    /// \return the index or -1 if not found.
    ///
    int indexOf(const QString &uuid) const;

private:
    /// \brief The instance storing data associated to this model.
    QPointer<PeersList> m_source;

    /// \brief The cached data of the model.
    QList<UserInfo> m_data;
    /// \brief The parallel list containing whether the item is selected or not.
    QList<bool> m_selected;
    /// \brief The number of selected items.
    int m_selectedCount;

    /// \brief The total number of files to be shared.
    const quint32 m_filesNumber;
    /// \brief The total size of the files to be shared.
    const QString m_filesSize;

    /// \brief The message written by the user to be attached to the request.
    QString m_message;
};

#endif // PEERSSELECTORMODEL_HPP
