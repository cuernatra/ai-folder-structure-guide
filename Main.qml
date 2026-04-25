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

    Settings 
    {
        id: appSettings
        property string savedFolder: "file:///"
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

    // READY: restore saved folder on startup
    Component.onCompleted: 
    {
        if (appSettings.savedFolder && appSettings.savedFolder !== "file:///") {
            window.currentFolder = appSettings.savedFolder
        }
    }

    // native folder picker dialog
    FolderDialog 
    {
        id: pickFolderDialog
        currentFolder: window.currentFolder
        onAccepted: window.currentFolder = selectedFolder.toString()
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

    ColumnLayout 
    {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 10

        RowLayout {
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

            Button 
            {
                text: "Save"
                onClicked: appSettings.savedFolder = window.currentFolder
            }

            // TODO: choose from multiple saved folders/favorites.
            Button 
            {
                text: "Use Saved"
                enabled: appSettings.savedFolder && appSettings.savedFolder !== "file:///"
                onClicked: window.currentFolder = appSettings.savedFolder
            }

            TextField 
            {
                Layout.fillWidth: true
                readOnly: true
                text: window.pathFromUrl(window.currentFolder)
            }
        }

        Label 
        {
            Layout.fillWidth: true
            text: "Saved: " + window.pathFromUrl(appSettings.savedFolder)
            elide: Text.ElideMiddle
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
                delegate: ItemDelegate {
                    width: ListView.view.width
                    text: (fileIsDir ? "📁 " : "📄 ") + fileName

                    onClicked: {
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
