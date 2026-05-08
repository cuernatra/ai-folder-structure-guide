// App entry point: initializes Qt/QML and exposes backend services.

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "FolderTreeService.h"
#include "OllamaService.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    FolderTreeService folderTreeService;
    OllamaService ollamaService;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("folderTreeService", &folderTreeService);
    engine.rootContext()->setContextProperty("ollamaService", &ollamaService);
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("ai_folder_structure_guide", "Main");

    return QCoreApplication::exec();
}
