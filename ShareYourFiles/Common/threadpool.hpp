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

#ifndef THREADPOOL_HPP
#define THREADPOOL_HPP

#include <QScopedPointer>

class QThread;

///
/// \brief The ThreadPool class provides a singleton interface to the worker
/// threads used by Share Your Files.
///
/// In particular this class, created according to the singleton pattern, allows
/// the access to the worker threads used by Share Your Files, the ones
/// where all the different protocols run.
///
/// Before being able to access the information, it is necessary to create the
/// instances through the createInstance() method.
///
class ThreadPool
{
public:
    ///
    /// \brief The constructor is disabled (it is not possible to create
    /// instances).
    ///
    explicit ThreadPool() = delete;

    ///
    /// \brief Creates the thread instances.
    ///
    static void createInstance();

    ///
    /// \brief Destroys the thread instances.
    ///
    static void destroyInstance();

    /// \brief Returns the SYFD Thread pointer (to be used for moveToThread())
    static QThread *syfdThread() { return m_syfdThread.data(); }

    /// \brief Returns the SYFD Thread pointer (to be used for moveToThread())
    static QThread *syfpThread() { return m_syfpThread.data(); }

    /// \brief Returns the SYFFT Receiver Thread pointer (to be used for
    /// moveToThread())
    static QThread *syfftReceiverThread() { return m_syfftRecvThread.data(); }

    /// \brief Returns the SYFFT Sender Thread pointer (to be used for
    /// moveToThread())
    static QThread *syfftSenderThread() { return m_syfftSenderThread.data(); }

private:
    /// \brief SYFD thread instance
    static QScopedPointer<QThread> m_syfdThread;

    /// \brief SYFP thread instance
    static QScopedPointer<QThread> m_syfpThread;

    /// \brief SYFFT Receiver thread instance
    static QScopedPointer<QThread> m_syfftRecvThread;

    /// \brief SYFFT Sender thread instance
    static QScopedPointer<QThread> m_syfftSenderThread;
};

#endif // THREADPOOL_HPP
