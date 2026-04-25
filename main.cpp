#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "FolderTreeService.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    FolderTreeService folderTreeService;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("folderTreeService", &folderTreeService);
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("ai_folder_structure_guide", "Main");

    return QCoreApplication::exec();
}
