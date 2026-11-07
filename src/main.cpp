// StrataTray 入口：单实例检查、初始化、事件循环（约 35 行，无业务逻辑）
#include <QtGui/QGuiApplication>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QSystemTrayIcon>
#include "core/Config.h"
#include "ui/I18n.h"
#include "ui/Theme.h"
#include "ui/TrayApp.h"

int main(int argc, char **argv) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#endif
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif
    QApplication app(argc, argv);
    Config::instance().initialize();
    app.setStyleSheet(QString::fromUtf8(Theme::qss()));
    QApplication::setQuitOnLastWindowClosed(false);
    app.setApplicationName(QStringLiteral("StrataTray"));

    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        QMessageBox::critical(nullptr, QStringLiteral("Strata"),
                              I18n::s("系统托盘不可用。", "System tray is not available."));
        return 1;
    }
    if (TrayApp::notifyExistingInstance()) return 0;   // 已有实例：置顶后静默退出

    TrayApp tray;
    tray.showWindow();
    return app.exec();
}
