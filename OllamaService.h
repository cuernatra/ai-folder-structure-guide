// Ollama backend API: model discovery, auto-start handling, and analysis requests.

#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QProcess>
#include <QStringList>

class OllamaService : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)

public:
    explicit OllamaService(QObject *parent = nullptr);

    Q_INVOKABLE void analyzeTree(const QString &tree, const QString &model);
    Q_INVOKABLE QStringList availableModels();

    bool loading() const;

signals:
    void analysisReady(const QString &result);
    void analysisError(const QString &error);
    void loadingChanged();

private:
    QNetworkAccessManager *m_network;
    QProcess *m_ollamaProcess = nullptr;
    QString m_ollamaProgram;
    bool m_loading = false;
    void setLoading(bool v);
    QString resolveOllamaProgram() const;
    bool ensureOllamaRunning();
};
