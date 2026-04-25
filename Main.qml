import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtCore
import Qt.labs.folderlistmodel

ApplicationWindow
{
    id: window
    width: 900
    height: 600
    visible: true
    title: qsTr("Window Browser for AI Guide")

    property string currentFolder: "file:///"
    property var savedFolders: []

    Settings
    {
        id: appSettings
        property string savedFolder: "file:///"
        property string savedFoldersJson: "[]"
    }

    ListModel
    {
        id: favoritesModel
    }

    // READY: URL -> local path
    function pathFromUrl(urlValue)
    {
        return decodeURIComponent(urlValue.toString().replace("file://", ""))
    }

    // READY: local path -> file URL
    function urlFromPath(pathValue)
    {
        return pathValue.startsWith("file://") ? pathValue : "file://" + pathValue
    }

    // READY: move one folder up
    function goUp()
    {
        const p = pathFromUrl(currentFolder)
        if (!p || p === "/")
        {
            return
        }

        const parts = p.split("/").filter(part => part.length > 0)
        parts.pop()
        const parentPath = "/" + parts.join("/")
        currentFolder = urlFromPath(parentPath || "/")
    }

    // READY: add current folder to favorites (if not already there)
    function addCurrentToFavorites()
    {
        if (!currentFolder || currentFolder === "file:///")
        {
            return
        }

        if (savedFolders.indexOf(currentFolder) === -1)
        {
            savedFolders.push(currentFolder)
            appSettings.savedFoldersJson = JSON.stringify(savedFolders)
            refreshFavoritesModel()
        }
    }

    // READY: rebuild combo model from saved favorites
    function refreshFavoritesModel()
    {
        favoritesModel.clear()
        for (let i = 0; i < savedFolders.length; i++)
        {
            const folderUrl = savedFolders[i]
            favoritesModel.append(
            {
                label: pathFromUrl(folderUrl),
                value: folderUrl
            })
        }
    }

    // READY: remove selected favorite and persist changes
    function removeFavoriteAt(index)
    {
        if (index < 0 || index >= savedFolders.length)
        {
            return
        }

        savedFolders.splice(index, 1)
        appSettings.savedFoldersJson = JSON.stringify(savedFolders)
        refreshFavoritesModel()

        if (favoritesModel.count === 0)
        {
            favoritesCombo.currentIndex = -1
        }
        else if (index >= favoritesModel.count)
        {
            favoritesCombo.currentIndex = favoritesModel.count - 1
        }
    }

    // READY: restore saved folder on startup
    Component.onCompleted:
    {
        if (appSettings.savedFolder && appSettings.savedFolder !== "file:///")
        {
            window.currentFolder = appSettings.savedFolder
        }

        try
        {
            const parsed = JSON.parse(appSettings.savedFoldersJson)
            if (parsed && parsed.length !== undefined)
            {
                savedFolders = parsed
            }
        }
        catch (e)
        {
            savedFolders = []
        }

        if (savedFolders.length === 0 && appSettings.savedFolder && appSettings.savedFolder !== "file:///")
        {
            savedFolders = [appSettings.savedFolder]
            appSettings.savedFoldersJson = JSON.stringify(savedFolders)
        }

        refreshFavoritesModel()
    }

    FolderDialog
    {
        id: pickFolderDialog
        currentFolder: window.currentFolder
        onAccepted:
        {
            window.currentFolder = selectedFolder.toString()
            appSettings.savedFolder = selectedFolder.toString()
        }
    }

    // customizing filters/sorting
    FolderListModel
    {
        id: folderModel
        folder: window.currentFolder
        showDirs: true
        showFiles: true
        showDotAndDotDot: false
        nameFilters: ["*"]
    }

    Dialog
    {
        id: removeFavoriteDialog
        modal: true
        title: "Remove favorite"
        standardButtons: Dialog.Yes | Dialog.No

        property int targetIndex: -1

        onAccepted: window.removeFavoriteAt(targetIndex)

        contentItem: Label
        {
            text: "Remove selected favorite folder?"
            wrapMode: Text.WordWrap
            padding: 12
        }
    }

    ColumnLayout
    {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 10

        RowLayout
        {
            Layout.fillWidth: true

            Button
            {
                text: "Up"
                onClicked: window.goUp()
            }

            Button
            {
                text: "Select Folder..."
                onClicked: pickFolderDialog.open()
            }

            TextField
            {
                Layout.fillWidth: true
                readOnly: true
                text: window.pathFromUrl(window.currentFolder)
            }
        }

        RowLayout
        {
            Layout.fillWidth: true

            Button
            {
                text: "Add Favorite"
                onClicked: window.addCurrentToFavorites()
            }

            ComboBox
            {
                id: favoritesCombo
                Layout.fillWidth: true
                model: favoritesModel
                textRole: "label"
            }

            Button
            {
                text: "Open Favorite"
                enabled: favoritesModel.count > 0 && favoritesCombo.currentIndex >= 0
                onClicked:
                {
                    const row = favoritesModel.get(favoritesCombo.currentIndex)
                    if (row && row.value)
                    {
                        window.currentFolder = row.value
                    }
                }
            }
        }

        RowLayout
        {
            Layout.fillWidth: true

            Item
            {
                Layout.fillWidth: true
            }

            Button
            {
                text: "Remove Favorite"
                enabled: favoritesModel.count > 0 && favoritesCombo.currentIndex >= 0

                background: Rectangle
                {
                    radius: 6
                    color: parent.down ? "#b3261e" : "#d32f2f"
                }

                contentItem: Text
                {
                    text: parent.text
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                onClicked:
                {
                    removeFavoriteDialog.targetIndex = favoritesCombo.currentIndex
                    removeFavoriteDialog.open()
                }
            }
        }

        Frame
        {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ListView
            {
                anchors.fill: parent
                clip: true
                model: folderModel

                // customizing row layout/icons/actions.
                delegate: ItemDelegate
                {
                    width: ListView.view.width
                    text: (fileIsDir ? "📁 " : "📄 ") + fileName

                    onClicked:
                    {
                        if (fileIsDir)
                        {
                            window.currentFolder = window.urlFromPath(filePath)
                        }
                    }
                }
            }
        }
    }
}
