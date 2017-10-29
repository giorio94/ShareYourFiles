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

#ifndef TRANSFERLIST_HPP
#define TRANSFERLIST_HPP

#include "fileinfo.hpp"

#include <QString>
#include <QVector>

class QFileInfo;

///
/// \brief The TransferList class represents the files ready to be sent through
/// the SYFFT Protocol.
///
/// The main purpose of this class is to provide an adapter between the list of
/// paths received through the SYFP Protocol, which must be absolute and refer
/// to elements in the same directory (i.e. the path, excluded the file or
/// directory name, must be equal for all of them) and the data format needed
/// by the transfer protocol, where the paths must be relative to a common base
/// directory. Moreover, when a path referring to a directory name is detected,
/// that directory is explored recursively to add to the list every file
/// contained inside it.
///
class TransferList
{
    friend class SyfftProtocolSender;

public:
    ///
    /// \brief Builds a new instance from the list of paths specified.
    /// \param pathsList the list of absolute paths of the files to be shared.
    ///
    explicit TransferList(const QStringList &pathsList);

    /// \brief Returns the path the files are relative to.
    QString basePath() const { return m_basePath; }

    /// \brief Returns a constant iterator to the first element of the list of
    /// files to be transferred.
    QVector<FileInfo>::ConstIterator filesBegin() const
    {
        return m_files.constBegin();
    }
    /// \brief Returns a constant iterator to the past the end element of the
    /// list of files to be transferred.
    QVector<FileInfo>::ConstIterator filesEnd() const
    {
        return m_files.constEnd();
    }

    /// \brief Returns the total number of files to be transferred.
    quint32 totalFiles() const { return static_cast<quint32>(m_files.count()); }
    /// \brief Returns the total size of the files to be transferred.
    quint64 totalBytes() const { return m_totalBytes; }

private:
    ///
    /// \brief Builds the transfer files list.
    /// \param items the list of file or directory names to be added.
    ///
    void buildFileList(QStringList items);

    ///
    /// \brief Adds the file or directory specified by the parameter to the
    /// transfer files list (in a recursive manner).
    /// \param item the object representing a file or directory.
    ///
    void addToFileList(const QFileInfo &item);

private:
    QString m_basePath;        ///< \brief The path the files are relative to.
    QVector<FileInfo> m_files; ///< \brief The list of files to be transferred.

    /// \brief The total size of the files to be transferred.
    quint64 m_totalBytes;
};

#endif // TRANSFERLIST_HPP
