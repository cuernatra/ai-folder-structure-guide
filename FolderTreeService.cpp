#include "FolderTreeService.h"

#include <QDir>
#include <QFileInfo>
#include <QUrl>

#include <algorithm>

FolderTreeService::FolderTreeService(QObject *parent)
    : QObject(parent)
{
}

QString FolderTreeService::buildTree(const QString &folderUrl,
                                     int maxDepth,
                                     bool includeFiles,
                                     int maxEntries) const
{
    const QString path = normalizePath(folderUrl);
    const QFileInfo rootInfo(path);

    if (!rootInfo.exists() || !rootInfo.isDir())
    {
        return QStringLiteral("Invalid folder: %1").arg(path);
    }

    if (maxDepth < 1)
    {
        maxDepth = 1;
    }

    if (maxEntries < 1)
    {
        maxEntries = 1;
    }

    QStringList lines;
    BuildState state;

    QString rootLabel = rootInfo.fileName();
    if (rootLabel.isEmpty())
    {
        rootLabel = rootInfo.absoluteFilePath();
    }

    lines << (rootLabel + "/");
    appendTree(lines,
               rootInfo.absoluteFilePath(),
               QString(),
               1,
               maxDepth,
               includeFiles,
               maxEntries,
               state);

    return lines.join('\n');
}

QString FolderTreeService::buildOllamaInput(const QString &folderUrl) const
{
    return buildTree(folderUrl, 5, true, 1800);
}

QString FolderTreeService::normalizePath(const QString &folderUrl)
{
    const QUrl url(folderUrl);
    if (url.isValid() && url.isLocalFile())
    {
        return QDir::cleanPath(url.toLocalFile());
    }

    return QDir::cleanPath(folderUrl);
}

bool FolderTreeService::shouldSkipName(const QString &name)
{
    static const QStringList skipNames = {
        QStringLiteral(".git"),
        QStringLiteral("build"),
        QStringLiteral("dist"),
        QStringLiteral("node_modules"),
        QStringLiteral(".venv"),
        QStringLiteral("venv"),
        QStringLiteral("__pycache__"),
        QStringLiteral(".idea"),
        QStringLiteral(".vscode")
    };

    return skipNames.contains(name, Qt::CaseInsensitive);
}

void FolderTreeService::appendTree(QStringList &lines,
                                   const QString &path,
                                   const QString &prefix,
                                   int depth,
                                   int maxDepth,
                                   bool includeFiles,
                                   int maxEntries,
                                   BuildState &state)
{
    const QDir dir(path);
    QFileInfoList entries = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries | QDir::Readable,
                                              QDir::DirsFirst | QDir::IgnoreCase | QDir::Name);

    QFileInfoList filtered;
    filtered.reserve(entries.size());

    for (const QFileInfo &info : entries)
    {
        if (!includeFiles && !info.isDir())
        {
            continue;
        }

        if (shouldSkipName(info.fileName()))
        {
            continue;
        }

        filtered.push_back(info);
    }

    for (int i = 0; i < filtered.size(); ++i)
    {
        const QFileInfo &entry = filtered.at(i);
        const bool isLast = (i == filtered.size() - 1);
        const QString branch = isLast ? QStringLiteral("└── ") : QStringLiteral("├── ");
        const QString childPrefix = prefix + (isLast ? QStringLiteral("    ") : QStringLiteral("│   "));

        if (state.count >= maxEntries)
        {
            state.truncated = true;
            return;
        }

        ++state.count;
        const QString name = entry.fileName() + (entry.isDir() ? QStringLiteral("/") : QString());
        lines << prefix + branch + name;

        if (!entry.isDir())
        {
            continue;
        }

        if (depth >= maxDepth)
        {
            continue;
        }

        appendTree(lines,
                   entry.absoluteFilePath(),
                   childPrefix,
                   depth + 1,
                   maxDepth,
                   includeFiles,
                   maxEntries,
                   state);

        if (state.truncated)
        {
            return;
        }
    }
}
