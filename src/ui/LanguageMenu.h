#pragma once
#include <QtGui/QAction>
#include <QtGui/QActionGroup>
#include <QtWidgets/QMenu>
#include "I18n.h"

// 可复用的「界面语言」二级子菜单构建器。
// 齿轮设置菜单与托盘右键菜单共用这一份逻辑；新增语种时无需改动本文件，
// 只要在 I18n.cpp 的注册表加一行，两处子菜单会自动多出对应单选项。
namespace LanguageMenu {

// 刷新子菜单标题与互斥勾选状态（启动时、语言切换后各调一次）
inline void retranslate(QMenu *sub) {
    if (!sub) return;
    sub->setTitle(I18n::zh() ? QString::fromUtf8("🌐 界面语言 (Language)")
                             : QString::fromUtf8("🌐 Language (界面语言)"));
    const int current = static_cast<int>(I18n::instance().lang());
    for (QAction *a : sub->actions()) {
        if (a->data().toInt() == current) {
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

    for (const auto &info : I18n::supportedLanguages()) {
        QAction *a = sub->addAction(QString::fromUtf8(info.nativeName));
        a->setCheckable(true);
        a->setData(static_cast<int>(info.id));   // 用枚举值关联，避免依赖菜单文本
        a->setActionGroup(group);
        const I18n::Lang id = info.id;
        QObject::connect(a, &QAction::triggered, receiver,
                         [id] { I18n::instance().setLang(id); });
    }
    retranslate(sub);
    return sub;
}

} // namespace LanguageMenu
