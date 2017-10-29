# Share Your Files

Share Your Files (SYF) is an application designed to allow peer to peer, on LAN,
file sharing.

## Project description

It allows different users, operating on different hosts belonging to the
same Local Area Network, to send files or entire directories one another
in just few clicks: the desired elements are selected as to perform a local
copy, the ''Send with SYF'' context menu action is selected and the list of
currently available peers is immediately shown to choose the recipient(s).
Just a final confirmation and, when the transfer is accepted by the
addressee, it is immediately started.

No limitation is enforced either about which types of files can be shared,
or about their size: if you have rights to read it, you are able to send it
as well (to be honest, symbolic links are skipped since they would
be completely meaningless on another system). Anyway, when you receive a
transfer request, a pop-up window appears to notify you about it, and to let
you the final choice about whether accepting or rejecting that reception
and, if confirmed, about where to save it. No local files are overwritten
without your agreement, even in case the names match.

Clearly, it is necessary to identify the other users in an immediate way, to
avoid confusing your friend with the boss, and this task cannot be performed
using ugly and confusing alphanumeric strings: a simple and configurable
profile is mandatory, and Share Your Files offers you the possibility to
show your first and last name joined with a profile image, since a picture
says a thousand words and makes everything funnier.

The whole process is completely plug-and-play: you start the application,
open the settings window to configure the information about yourself
(obviously remembered for the next executions), and you are ready to receive
everything your colleagues wants to share with you. Do you prefer remaining
anonymous? You are lucky: the offline mode is available to avoid disclosing
your presence in the network. You are still able to send whatever you want,
if your colleagues trust you hidden behind your mask; reception, on the
other hand, is forbidden: if the others do not know about you, how the hell
could ever select you?

No external servers or privileged users are necessary to enable Share your
Files operations: multicast messages are exploited to advertise user's
presence to the others through the so called SYFD (Share Your Files
Discovery) protocol, an ad-hoc network protocol developed on the top of UDP
to distribute profile information to all the other listeners. Clearly, some
disadvantages are also introduced by this choice which, in particular,
limits the working area of Share Your Files to multicast enabled Local Area
Networks - no global Internet, I'm sorry. Moreover, the absence of a central
authority is a problem in case security is a concern: peers authentication
is simply impossible and, therefore, there is no way to verify the
correspondence between a profile and its owner, or to be sure about who is
actually sending you some data. Whether this is a problem or not depends on
you, but let me give you a peace of advise: do not send top secret materials
with this software.

Share Your Files, born as an assignment from the ''Programmazione di
Sistema'' course taught at the Politecnico di Torino (Italy), is developed
In c++ using the Qt Framework, choice performed mainly due to the
powerful tools provided to draw great GUIs (in particular the QML
declarative language and the ready to use QtQuick controls), and its
characteristic of targeting all the main operating systems.

## Requirements

+ C++11 toolchain
+ [Qt](https://www.qt.io/) version 5.8 or higher
+ Git

## Instructions

Get a copy of the repository, import the project in QtCreator and build it.

    git clone --recursive https://github.com/giorio94/ShareYourFiles.git

In case you do not want to use the IDE, it is also possible to proceed from
the command line (the following commands work under Linux but similar ones
should be available also under Windows):

    mkdir build_dir
    cd build_dir
    qmake path_to_source
    make

Once the program has been correctly compiled, it is possible to add the ''Send
with SYF'' context menu action to your file manager, following the instructions
provided in the `SYFPicker/ContextMenu` directory in the source tree.

Finally, the main application can be executed for the first time: in case some
errors related to missing libraries are thrown at the startup, verify that all
the need libraries are available and located in the standard path. In
particular, under Linux, it may be necessary to specify the location of the
`CuteLogger` library through

    LD_LIBRARY_PATH=../CuteLogger/ ./ShareYourFiles

Under Windows, on the other hand, the same task can be performed, after having
copied `ShareYourFiles.exe`, `SYFPicker.exe` and `CuteLogger.dll` in a new
directory, through the command (issued from that directory):

    QTDIR/bin/windeployqt --qmldir path_to_source/ShareYourFiles/Gui/Qml ShareYourFiles.exe

which copies all the dependencies in order to be able to execute the
program with just a double click.

## License

This project is licensed under the [GNU General Public License version 3](
https://www.gnu.org/licenses/gpl-3.0.en.html).

### External components

+ [CuteLogger](http://github.com/dept2/CuteLogger) licensed under LGPL 2.1
+ [Source Sans Pro](https://fonts.google.com/specimen/Source+Sans+Pro) font
  by Paul D. Hunt licensed under OFL 1.1
+ SYF icon by [Washaweb](http://www.washaweb.com) licensed under CC BY 3.0
+ [Other icons](http://materialdesignicons.com) by Austin Andrews licensed
  under OFL 1.1
