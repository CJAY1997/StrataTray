#pragma once
#include <QtGui/QAction>
#include <QtGui/QActionGroup>
#include <QtWidgets/QMenu>
#include "I18n.h"

// 可复用的「界面语言」二级子菜单构建器。
// 菜单项完全来自 I18n 运行时发现的语言包（内嵌资源 + exe 旁 i18n/ 目录），
// 因此贡献者新增一个 JSON 文件后，齿轮菜单与托盘菜单会自动多出该语种。
namespace LanguageMenu {

// 刷新子菜单标题与互斥勾选状态（启动时、语言切换后各调一次）
inline void retranslate(QMenu *sub) {
    if (!sub) return;
    sub->setTitle(I18n::t("lang_menu"));
    const QString current = I18n::instance().langId();
    for (QAction *a : sub->actions()) {
        if (a->data().toString() == current) {
            a->setChecked(true);        // 只发 toggled，不会触发 setLang 形成回环
            break;
        }
    }
}

// 在 parent 上挂一个带箭头的二级子菜单，返回该子菜单供后续 retranslate 使用
inline QMenu *create(QMenu *parent, QObject *receiver) {
    QMenu *sub = parent->addMenu(QString());
    auto *group = new QActionGroup(receiver);
    group->setExclusive(true);          // 互斥单选：当前语种显示选中标记

    for (const auto &info : I18n::instance().languages()) {
        QAction *a = sub->addAction(info.name);
        a->setCheckable(true);
        a->setData(info.id);            // 用语言包 id 关联，不依赖显示文本
        a->setActionGroup(group);
        const QString id = info.id;
        QObject::connect(a, &QAction::triggered, receiver,
                         [id] { I18n::instance().setLang(id); });
    }
    retranslate(sub);
    return sub;
}

} // namespace LanguageMenu
