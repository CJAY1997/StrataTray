#pragma once
#include <QtGui/QAction>
#include <QtGui/QActionGroup>
#include <QtWidgets/QMenu>
#include "I18n.h"
#include "ThemeManager.h"

// 可复用的「外观主题」二级子菜单构建器：齿轮菜单与托盘菜单共用。
// 主题项来自 ThemeManager 注册表（内置 + 外部 .qss 自动发现），因此新增主题无需改 UI 代码。
namespace ThemeMenu {

// 刷新标题、各项显示名与互斥勾选（启动时、主题切换后、语言切换后各调一次）
inline void retranslate(QMenu *sub) {
    if (!sub) return;
    ThemeManager &tm = ThemeManager::instance();
    sub->setTitle(I18n::t("theme_menu"));
    const auto acts = sub->actions();
    const auto &list = tm.themes();
    for (int i = 0; i < acts.size() && i < list.size(); ++i)
        acts.at(i)->setText(tm.displayName(list.at(i)));
    const QString current = tm.currentId();
    for (QAction *a : acts) {
        if (a->data().toString() == current) {
            a->setChecked(true);
            break;
        }
    }
}

// 在 parent 上挂一个带箭头的二级子菜单，返回该子菜单供后续 retranslate 使用
inline QMenu *create(QMenu *parent, QObject *receiver) {
    QMenu *sub = parent->addMenu(QString());
    auto *group = new QActionGroup(receiver);
    group->setExclusive(true);

    ThemeManager &tm = ThemeManager::instance();
    for (const auto &info : tm.themes()) {
        QAction *a = sub->addAction(tm.displayName(info));
        a->setCheckable(true);
        a->setData(info.id);
        a->setActionGroup(group);
        const QString id = info.id;
        QObject::connect(a, &QAction::triggered, receiver,
                         [id] { ThemeManager::instance().apply(id); });
    }
    // 在任一菜单里切换主题后，两处菜单的勾选状态与文案同步
    QObject::connect(&tm, &ThemeManager::themeChanged, receiver,
                     [sub] { retranslate(sub); });
    retranslate(sub);
    return sub;
}

} // namespace ThemeMenu
