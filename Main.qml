import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Controls.Basic as Basic
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
    property string generatedTree: ""
    property bool compactLayout: width < 980

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
        let pathText = pathValue ? pathValue.toString() : "/"

        if (pathText.startsWith("file://"))
        {
            pathText = pathFromUrl(pathText)
        }

        if (!pathText || pathText.length === 0)
        {
            pathText = "/"
        }

        while (pathText.length > 1 && pathText.endsWith("/"))
        {
            pathText = pathText.slice(0, -1)
        }

        return "file://" + pathText
    }

    // READY: normalize folder URL so trailing slash format stays consistent
    function normalizeFolderUrl(folderValue)
    {
        return urlFromPath(folderValue)
    }

    // READY: normalize + deduplicate saved favorites
    function normalizeSavedFolders()
    {
        const normalized = []
        for (let i = 0; i < savedFolders.length; i++)
        {
            const value = normalizeFolderUrl(savedFolders[i])
            if (normalized.indexOf(value) === -1)
            {
                normalized.push(value)
            }
        }

        savedFolders = normalized
        appSettings.savedFoldersJson = JSON.stringify(savedFolders)
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
        const normalizedCurrent = normalizeFolderUrl(currentFolder)

        if (!normalizedCurrent || normalizedCurrent === "file:///")
        {
            return
        }

        if (savedFolders.indexOf(normalizedCurrent) === -1)
        {
            savedFolders.push(normalizedCurrent)
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
            window.currentFolder = normalizeFolderUrl(appSettings.savedFolder)
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
            savedFolders = [normalizeFolderUrl(appSettings.savedFolder)]
            appSettings.savedFoldersJson = JSON.stringify(savedFolders)
        }

        normalizeSavedFolders()

        refreshFavoritesModel()
    }

    FolderDialog
    {
        id: pickFolderDialog
        currentFolder: window.currentFolder
        onAccepted:
        {
            const normalizedSelected = normalizeFolderUrl(selectedFolder.toString())
            window.currentFolder = normalizedSelected
            appSettings.savedFolder = normalizedSelected
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

    MessageDialog
    {
        id: removeFavoriteDialog
        title: "Remove favorite"
        text: "Remove selected favorite folder?"
        buttons: MessageDialog.Yes | MessageDialog.No

        property int targetIndex: -1

        onAccepted: window.removeFavoriteAt(targetIndex)
    }

    ColumnLayout
    {
        anchors.fill: parent
        anchors.margins: compactLayout ? 10 : 12
        spacing: compactLayout ? 8 : 10

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

        GroupBox
        {
            title: "Favorites"
            Layout.fillWidth: true
            Layout.preferredHeight: compactLayout ? 140 : 130

            ColumnLayout
            {
                anchors.fill: parent
                spacing: 8

                GridLayout
                {
                    Layout.fillWidth: true
                    columns: 3

                    Button
                    {
                        text: "Add Favorite"
                        Layout.row: 0
                        Layout.column: 0
                        Layout.preferredWidth: compactLayout ? 120 : 130
                        onClicked: window.addCurrentToFavorites()
                    }

                    Button
                    {
                        text: "Open Favorite"
                        Layout.row: 1
                        Layout.column: 0
                        Layout.preferredWidth: compactLayout ? 120 : 130
                        enabled: favoritesModel.count > 0 && favoritesCombo.currentIndex >= 0
                        onClicked:
                        {
                            const row = favoritesModel.get(favoritesCombo.currentIndex)
                            if (row && row.value)
                            {
                                window.currentFolder = normalizeFolderUrl(row.value)
                            }
                        }
                    }

                    ComboBox
                    {
                        id: favoritesCombo
                        Layout.row: 0
                        Layout.column: 1
                        Layout.rowSpan: 2
                        Layout.fillWidth: true
                        model: favoritesModel
                        textRole: "label"
                    }

                    Basic.Button
                    {
                        text: "Remove Favorite"
                        Layout.row: 0
                        Layout.column: 2
                        Layout.rowSpan: 2
                        Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                        enabled: favoritesModel.count > 0 && favoritesCombo.currentIndex >= 0

                        background: Rectangle
                        {
                            implicitWidth: 140
                            implicitHeight: 36
                            radius: 6
                            color: parent.down ? "#b3261e" : "#d32f2f"
                            opacity: parent.enabled ? 1.0 : 0.45
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
            }
        }

        SplitView
        {
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: Qt.Vertical

            Frame
            {
                SplitView.minimumHeight: 140
                SplitView.preferredHeight: Math.max(170, window.height * 0.32)

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

            GroupBox
            {
                title: "Ollama Input"
                SplitView.minimumHeight: 220
                SplitView.fillHeight: true
                SplitView.preferredHeight: Math.max(260, window.height * 0.48)

                ColumnLayout
                {
                    anchors.fill: parent
                    spacing: 8

                    RowLayout
                    {
                        Layout.fillWidth: true

                        Button
                        {
                            text: "Generate Ollama Input"
                            onClicked:
                            {
                                generatedTree = folderTreeService.buildOllamaInput(window.currentFolder)
                            }
                        }

                        Item
                        {
                            Layout.fillWidth: true
                        }
                    }

                    ScrollView
                    {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        TextArea
                        {
                            readOnly: true
                            wrapMode: TextEdit.NoWrap
                            font.family: "Menlo"
                            text: generatedTree.length > 0
                                ? generatedTree
                                : "1) Select folder\n2) Click Generate Ollama Input\n3) ENJOY!"
                        }
                    }
                }
            }
        }
    }
}
