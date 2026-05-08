#include "OllamaService.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QUrl>
#include <QFileInfo>
#include <QCoreApplication>
#include <QDir>

QString OllamaService::resolveOllamaProgram() const
{
    const QString localOllama = QDir::toNativeSeparators(
        QCoreApplication::applicationDirPath() + "/../Ollama/ollama.exe");
    if (QFileInfo::exists(localOllama))
    {
        return localOllama;
    }

    const QString userOllama = QDir::toNativeSeparators(
        qEnvironmentVariable("LOCALAPPDATA") + "/Programs/Ollama/ollama.exe");
    if (QFileInfo::exists(userOllama))
    {
        return userOllama;
    }

    return QStringLiteral("ollama");
}

QStringList OllamaService::availableModels()
{
    QStringList fallbackModels{QStringLiteral("llama3.2"), QStringLiteral("llama3.1"), QStringLiteral("mistral"), QStringLiteral("qwen2.5")};

    QProcess listProcess;
    listProcess.setProgram(m_ollamaProgram.isEmpty() ? resolveOllamaProgram() : m_ollamaProgram);
    listProcess.setArguments({QStringLiteral("list")});
    listProcess.setProcessChannelMode(QProcess::MergedChannels);
    listProcess.start();

    if (!listProcess.waitForFinished(3000))
    {
        return fallbackModels;
    }

    const QString output = QString::fromLocal8Bit(listProcess.readAllStandardOutput());
    QStringList models;
    const QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    for (const QString &rawLine : lines)
    {
        const QString line = rawLine.trimmed();
        if (line.isEmpty() || line.startsWith("NAME"))
            continue;

        const QString modelName = line.section(' ', 0, 0).trimmed();
        if (!modelName.isEmpty() && !models.contains(modelName))
            models << modelName;
    }

    return models.isEmpty() ? fallbackModels : models;
}

bool OllamaService::ensureOllamaRunning()
{
    if (m_ollamaProcess && m_ollamaProcess->state() != QProcess::NotRunning)
    {
        return true;
    }

    if (!m_ollamaProcess)
    {
        m_ollamaProcess = new QProcess(this);
    }

    m_ollamaProcess->setProgram(m_ollamaProgram.isEmpty() ? resolveOllamaProgram() : m_ollamaProgram);
    m_ollamaProcess->setArguments({"serve"});
    m_ollamaProcess->setProcessChannelMode(QProcess::MergedChannels);
    m_ollamaProcess->start();

    if (!m_ollamaProcess->waitForStarted(2500))
    {
        emit analysisError(
            "Could not start Ollama automatically.\n\n"
            "Install Ollama: https://ollama.com/download\n"
            "Then open Ollama and try again.");
        return false;
    }

    return true;
}

OllamaService::OllamaService(QObject *parent)
    : QObject(parent)
    , m_network(new QNetworkAccessManager(this))
    , m_ollamaProgram(resolveOllamaProgram())
{
}

bool OllamaService::loading() const
{
    return m_loading;
}

void OllamaService::setLoading(bool v)
{
    if (m_loading == v)
        return;
    m_loading = v;
    emit loadingChanged();
}

void OllamaService::analyzeTree(const QString &tree, const QString &model)
{
    if (m_loading)
        return;

    if (!ensureOllamaRunning())
        return;

    setLoading(true);

    const QString prompt =
        "You are analyzing a project folder structure. "
        "Describe what this project is, what technologies or frameworks it uses, "
        "the project's purpose, and how it's organized. Be concise but informative.\n\n"
        "Folder structure:\n\n" + tree;

    QJsonObject body;
    body["model"] = model.isEmpty() ? QStringLiteral("llama3.2") : model;
    body["prompt"] = prompt;
    body["stream"] = false;

    QNetworkRequest request(QUrl("http://localhost:11434/api/generate"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = m_network->post(request, QJsonDocument(body).toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        setLoading(false);

        if (reply->error() != QNetworkReply::NoError) {
            emit analysisError(
                "Connection error: " + reply->errorString() +
                "\n\nMake sure Ollama is running at localhost:11434"
                "\nIf Ollama is missing, install it from: https://ollama.com/download");
            return;
        }

        const QByteArray data = reply->readAll();
        const QJsonDocument doc = QJsonDocument::fromJson(data);

        if (doc.isNull() || !doc.isObject()) {
            emit analysisError("Invalid response from Ollama");
            return;
        }

        const QString response = doc.object().value("response").toString();
        if (response.isEmpty()) {
            emit analysisError("Empty response from Ollama");
            return;
        }

        emit analysisReady(response);
    });
}
