// Folder tree backend: builds project tree text (optionally with file contents) for AI prompts.

#pragma once

#include <QObject>
#include <QFileInfo>
#include <QStringList>

class FolderTreeService : public QObject
{
    Q_OBJECT

public:
    explicit FolderTreeService(QObject *parent = nullptr);

    Q_INVOKABLE QString buildTree(const QString &folderUrl,
                                  int maxDepth = 4,
                                  bool includeFiles = true,
                                  int maxEntries = 2000) const;

    Q_INVOKABLE QString buildOllamaInput(const QString &folderUrl) const;
    Q_INVOKABLE QString buildOllamaInputWithContent(const QString &folderUrl,
                                                   bool includeContent,
                                                   int maxFileChars = 4000,
                                                   int maxTotalChars = 30000) const;

private:
    struct BuildState
    {
        int count = 0;
        bool truncated = false;
    };

    static QString normalizePath(const QString &folderUrl);

    static bool shouldSkipName(const QString &name);

    static bool shouldIncludeFileContent(const QFileInfo &info);

    static void appendFileContents(QStringList &lines,
                                   const QString &rootPath,
                                   int maxDepth,
                                   int maxFileChars,
                                   int maxTotalChars,
                                   int &totalChars);

    static void appendTree(QStringList &lines,
                           const QString &path,
                           const QString &prefix,
                           int depth,
                           int maxDepth,
                           bool includeFiles,
                           int maxEntries,
                           BuildState &state);
};
