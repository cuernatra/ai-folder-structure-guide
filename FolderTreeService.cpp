// Folder tree backend implementation for generating Ollama-ready project context.

#include "FolderTreeService.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
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

QString FolderTreeService::buildOllamaInputWithContent(const QString &folderUrl,
                                                       bool includeContent,
                                                       int maxFileChars,
                                                       int maxTotalChars) const
{
    QString result = buildTree(folderUrl, 5, true, 1800);

    if (!includeContent)
    {
        return result;
    }

    const QString path = normalizePath(folderUrl);
    const QFileInfo rootInfo(path);
    if (!rootInfo.exists() || !rootInfo.isDir())
    {
        return result;
    }

    QStringList lines;
    lines << "";
    lines << "--- FILE CONTENTS (filtered) ---";
    int totalChars = 0;
    appendFileContents(lines,
                       rootInfo.absoluteFilePath(),
                       5,
                       qMax(200, maxFileChars),
                       qMax(1000, maxTotalChars),
                       totalChars);

    return result + "\n" + lines.join('\n');
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

bool FolderTreeService::shouldIncludeFileContent(const QFileInfo &info)
{
    if (!info.isFile())
        return false;

    const QString suffix = info.suffix().toLower();
    static const QStringList allowed = {
        QStringLiteral("cpp"),
        QStringLiteral("h"),
        QStringLiteral("hpp"),
        QStringLiteral("c"),
        QStringLiteral("qml"),
        QStringLiteral("js"),
        QStringLiteral("ts"),
        QStringLiteral("py"),
        QStringLiteral("md"),
        QStringLiteral("txt"),
        QStringLiteral("json"),
        QStringLiteral("cmake")
    };

    return allowed.contains(suffix);
}

void FolderTreeService::appendFileContents(QStringList &lines,
                                           const QString &rootPath,
                                           int maxDepth,
                                           int maxFileChars,
                                           int maxTotalChars,
                                           int &totalChars)
{
    struct Node
    {
        QString path;
        int depth;
    };

    QVector<Node> stack;
    stack.push_back({rootPath, 0});

    while (!stack.isEmpty())
    {
        const Node current = stack.takeLast();
        const QDir dir(current.path);
        const QFileInfoList entries = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries | QDir::Readable,
                                                        QDir::DirsFirst | QDir::IgnoreCase | QDir::Name);

        for (int i = entries.size() - 1; i >= 0; --i)
        {
            const QFileInfo info = entries.at(i);
            if (shouldSkipName(info.fileName()))
                continue;

            if (info.isDir())
            {
                if (current.depth < maxDepth)
                    stack.push_back({info.absoluteFilePath(), current.depth + 1});
                continue;
            }

            if (!shouldIncludeFileContent(info))
                continue;

            QFile f(info.absoluteFilePath());
            if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
                continue;

            QString content = QString::fromUtf8(f.readAll());
            if (content.size() > maxFileChars)
                content = content.left(maxFileChars) + "\n... [truncated]";

            const QString block = QString("\n### %1\n```\n%2\n```\n")
                                  .arg(info.absoluteFilePath(), content);

            if (totalChars + block.size() > maxTotalChars)
            {
                lines << "... [total content limit reached]";
                return;
            }

            lines << block;
            totalChars += block.size();
        }
    }
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
