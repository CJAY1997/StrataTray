#pragma once
#include <QtGui/QIcon>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>
#include "core/ProcessProbe.h"

// 纯样式与图标：无业务逻辑，header-only
namespace Theme {

// 暗色主题：精致科技暗黑风（默认）
inline const char *darkQss() {
    return R"QSS(
QWidget { background-color: #18181b; color: #e4e4e7; font-family: "Segoe UI", "Microsoft YaHei UI", sans-serif; font-size: 10pt; }
QLabel { background: transparent; }
QLabel#cardTitle { color: #fafafa; font-size: 11pt; font-weight: 600; }
QLabel#caption   { color: #71717a; font-size: 9pt; }
QLabel#muted     { color: #a1a1aa; font-size: 9pt; }
QLabel#mono      { color: #a1a1aa; font-size: 9pt; font-family: Consolas, "Cascadia Code", monospace; }
QWidget#card { background-color: #202023; border: 1px solid #2f2f35; border-radius: 10px; }
QWidget#badgePill { border-radius: 11px; }
QLabel#stateDot   { border-radius: 4px; }
QFrame#hr { color: #33333a; background-color: #33333a; border: none; }
QPushButton { background-color: #2b2b31; color: #e4e4e7; border: 1px solid #3a3a42; border-radius: 7px; padding: 7px 14px; font-size: 10pt; }
QPushButton:hover    { background-color: #36363e; border-color: #4a4a55; }
QPushButton:pressed  { background-color: #232329; }
QPushButton:disabled { background-color: #232327; color: #5b5b63; border-color: #2c2c32; }
QPushButton#btnPrimary { background-color: #16a34a; border-color: #22c55e; color: #ffffff; font-weight: 600; }
QPushButton#btnPrimary:hover   { background-color: #15803d; }
QPushButton#btnPrimary:pressed { background-color: #166534; }
QPushButton#btnPrimary:disabled{ background-color: #1c2a22; color: #5b6b60; border-color: #26362c; }
QPushButton#btnDanger { background-color: #3a1d1d; border-color: #7f2d2d; color: #fca5a5; font-weight: 600; }
QPushButton#btnDanger:hover    { background-color: #4d2424; }
QPushButton#btnDanger:pressed  { background-color: #2e1717; }
QPushButton#btnDanger:disabled { background-color: #232327; color: #5b5b63; border-color: #2c2c32; }
QPushButton#btnAccent { background-color: #1d4ed8; border-color: #3b82f6; color: #ffffff; font-weight: 600; }
QPushButton#btnAccent:hover    { background-color: #2563eb; }
QPushButton#btnAccent:pressed  { background-color: #1e40af; }
QPushButton#btnAccent:disabled { background-color: #1d2333; color: #5b6478; border-color: #29314a; }
QPushButton#btnGhost { background-color: transparent; border-color: #3a3a42; color: #a1a1aa; }
QPushButton#btnGhost:hover { background-color: #2b2b31; color: #e4e4e7; }
QPushButton#btnMini { background-color: #2b2b31; border: 1px solid #3a3a42; color: #a1a1aa; border-radius: 6px; padding: 4px 11px; font-size: 9pt; }
QPushButton#btnMini:hover   { background-color: #36363e; color: #e4e4e7; }
QPushButton#btnMini:pressed { background-color: #232329; }
QTextEdit { background-color: #141417; color: #d4d4d8; border: 1px solid #2f2f35; border-radius: 8px; padding: 8px; font-size: 9pt; selection-background-color: #3b82f6; }
QTextEdit#logView     { font-family: Consolas, "Cascadia Code", monospace; }
QTextEdit#versionView { font-family: "Segoe UI", "Microsoft YaHei UI", sans-serif; }
QProgressBar { background-color: #2a2a30; border: none; border-radius: 4px; text-align: right; }
QProgressBar::chunk { background-color: #3b82f6; border-radius: 4px; }
QProgressBar#vramBar::chunk   { background-color: #22c55e; }
QProgressBar#updateBar::chunk { background-color: #f59e0b; }
QTabWidget::pane { border: none; background: transparent; top: 0; }
QTabWidget::tab-bar { alignment: left; }
QTabBar { background: transparent; qproperty-drawBase: 0; }
QTabBar::tab { background: transparent; color: #8b8b95; padding: 6px 16px; margin-right: 4px; border: 1px solid transparent; border-radius: 7px; font-size: 10pt; }
QTabBar::tab:hover    { color: #e4e4e7; background: #232329; }
QTabBar::tab:selected { color: #fafafa; background: #2b2b33; border-color: #3a3a45; font-weight: 600; }
QCheckBox { color: #a1a1aa; font-size: 9pt; spacing: 6px; background: transparent; }
QCheckBox::indicator { width: 14px; height: 14px; border-radius: 4px; border: 1px solid #4a4a55; background: #202023; }
QCheckBox::indicator:checked { background-color: #3b82f6; border-color: #3b82f6; image: url(:/icons/check.svg); }
QCheckBox::indicator:hover   { border-color: #6b6b78; }
QScrollBar:vertical { background: transparent; width: 10px; margin: 2px 2px 2px 0; }
QScrollBar::handle:vertical { background: #3a3a44; border-radius: 5px; min-height: 24px; }
QScrollBar::handle:vertical:hover { background: #52525b; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }
QScrollBar:horizontal { background: transparent; height: 10px; margin: 0 2px 2px 2px; }
QScrollBar::handle:horizontal { background: #3a3a44; border-radius: 5px; min-width: 24px; }
QScrollBar::handle:horizontal:hover { background: #52525b; }
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }
QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal { background: transparent; }
QMenu { background: #202023; border: 1px solid #3a3a45; border-radius: 9px; padding: 6px; color: #e4e4e7; }
QMenu::item { padding: 6px 22px; border-radius: 5px; font-size: 10pt; }
QMenu::item:selected  { background: #34343d; }
QMenu::item:disabled  { color: #5b5b63; }
QMenu::separator { height: 1px; background: #3a3a45; margin: 5px 8px; }
QToolTip { background: #2b2b33; color: #e4e4e7; border: 1px solid #3a3a45; border-radius: 6px; padding: 4px 8px; }
)QSS";
}

// 亮色主题：优雅清爽现代浅色风（背景 #f4f4f5、纯白卡片、深色文字）
inline const char *lightQss() {
    return R"QSS(
QWidget { background-color: #f4f4f5; color: #3f3f46; font-family: "Segoe UI", "Microsoft YaHei UI", sans-serif; font-size: 10pt; }
QLabel { background: transparent; }
QLabel#cardTitle { color: #18181b; font-size: 11pt; font-weight: 600; }
QLabel#caption   { color: #71717a; font-size: 9pt; }
QLabel#muted     { color: #52525b; font-size: 9pt; }
QLabel#mono      { color: #52525b; font-size: 9pt; font-family: Consolas, "Cascadia Code", monospace; }
QWidget#card { background-color: #ffffff; border: 1px solid #e4e4e7; border-radius: 10px; }
QWidget#badgePill { border-radius: 11px; }
QLabel#stateDot   { border-radius: 4px; }
QFrame#hr { color: #e4e4e7; background-color: #e4e4e7; border: none; }
QPushButton { background-color: #ffffff; color: #3f3f46; border: 1px solid #d4d4d8; border-radius: 7px; padding: 7px 14px; font-size: 10pt; }
QPushButton:hover    { background-color: #f4f4f5; border-color: #a1a1aa; }
QPushButton:pressed  { background-color: #e4e4e7; }
QPushButton:disabled { background-color: #fafafa; color: #a1a1aa; border-color: #e4e4e7; }
QPushButton#btnPrimary { background-color: #16a34a; border-color: #15803d; color: #ffffff; font-weight: 600; }
QPushButton#btnPrimary:hover   { background-color: #15803d; }
QPushButton#btnPrimary:pressed { background-color: #166534; }
QPushButton#btnPrimary:disabled{ background-color: #dcfce7; color: #86efac; border-color: #bbf7d0; }
QPushButton#btnDanger { background-color: #fee2e2; border-color: #fca5a5; color: #b91c1c; font-weight: 600; }
QPushButton#btnDanger:hover    { background-color: #fecaca; }
QPushButton#btnDanger:pressed  { background-color: #fca5a5; }
QPushButton#btnDanger:disabled { background-color: #fafafa; color: #a1a1aa; border-color: #e4e4e7; }
QPushButton#btnAccent { background-color: #1d4ed8; border-color: #3b82f6; color: #ffffff; font-weight: 600; }
QPushButton#btnAccent:hover    { background-color: #2563eb; }
QPushButton#btnAccent:pressed  { background-color: #1e40af; }
QPushButton#btnAccent:disabled { background-color: #dbeafe; color: #93c5fd; border-color: #bfdbfe; }
QPushButton#btnGhost { background-color: transparent; border-color: #d4d4d8; color: #52525b; }
QPushButton#btnGhost:hover { background-color: #f4f4f5; color: #18181b; }
QPushButton#btnMini { background-color: #ffffff; border: 1px solid #d4d4d8; color: #52525b; border-radius: 6px; padding: 4px 11px; font-size: 9pt; }
QPushButton#btnMini:hover   { background-color: #f4f4f5; color: #18181b; }
QPushButton#btnMini:pressed { background-color: #e4e4e7; }
QTextEdit { background-color: #f8fafc; color: #3f3f46; border: 1px solid #e4e4e7; border-radius: 8px; padding: 8px; font-size: 9pt; selection-background-color: #3b82f6; }
QTextEdit#logView     { font-family: Consolas, "Cascadia Code", monospace; }
QTextEdit#versionView { font-family: "Segoe UI", "Microsoft YaHei UI", sans-serif; }
QProgressBar { background-color: #e4e4e7; border: none; border-radius: 4px; text-align: right; }
QProgressBar::chunk { background-color: #3b82f6; border-radius: 4px; }
QProgressBar#vramBar::chunk   { background-color: #16a34a; }
QProgressBar#updateBar::chunk { background-color: #d97706; }
QTabWidget::pane { border: none; background: transparent; top: 0; }
QTabWidget::tab-bar { alignment: left; }
QTabBar { background: transparent; qproperty-drawBase: 0; }
QTabBar::tab { background: transparent; color: #71717a; padding: 6px 16px; margin-right: 4px; border: 1px solid transparent; border-radius: 7px; font-size: 10pt; }
QTabBar::tab:hover    { color: #18181b; background: #e4e4e7; }
QTabBar::tab:selected { color: #18181b; background: #ffffff; border-color: #d4d4d8; font-weight: 600; }
QCheckBox { color: #52525b; font-size: 9pt; spacing: 6px; background: transparent; }
QCheckBox::indicator { width: 14px; height: 14px; border-radius: 4px; border: 1px solid #a1a1aa; background: #ffffff; }
QCheckBox::indicator:checked { background-color: #3b82f6; border-color: #3b82f6; image: url(:/icons/check.svg); }
QCheckBox::indicator:hover   { border-color: #71717a; }
QScrollBar:vertical { background: transparent; width: 10px; margin: 2px 2px 2px 0; }
QScrollBar::handle:vertical { background: #d4d4d8; border-radius: 5px; min-height: 24px; }
QScrollBar::handle:vertical:hover { background: #a1a1aa; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }
QScrollBar:horizontal { background: transparent; height: 10px; margin: 0 2px 2px 2px; }
QScrollBar::handle:horizontal { background: #d4d4d8; border-radius: 5px; min-width: 24px; }
QScrollBar::handle:horizontal:hover { background: #a1a1aa; }
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }
QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal { background: transparent; }
QMenu { background: #ffffff; border: 1px solid #d4d4d8; border-radius: 9px; padding: 6px; color: #3f3f46; }
QMenu::item { padding: 6px 22px; border-radius: 5px; font-size: 10pt; }
QMenu::item:selected  { background: #e4e4e7; }
QMenu::item:disabled  { color: #a1a1aa; }
QMenu::separator { height: 1px; background: #e4e4e7; margin: 5px 8px; }
QToolTip { background: #ffffff; color: #3f3f46; border: 1px solid #d4d4d8; border-radius: 6px; padding: 4px 8px; }
)QSS";
}

// 兼容旧调用
inline const char *qss() { return darkQss(); }

// 高 DPI：256px 母版 + 各尺寸 2x 位图
inline QIcon appIcon(State st) {
    constexpr int S = 256;
    QPixmap pm(S, S);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(QColor(40, 40, 46));
    p.setPen(QPen(QColor(90, 90, 100), 8));
    p.drawEllipse(16, 16, S - 32, S - 32);
    QColor dot(0x8a, 0x8a, 0x92);
    switch (st) {
    case State::Ready:    dot = QColor(0x22, 0xc5, 0x5e); break;
    case State::Starting: dot = QColor(0xf5, 0x9e, 0x0b); break;
    case State::Orphan:   dot = QColor(0xef, 0x44, 0x44); break;
    case State::Stopped:  break;
    }
    p.setPen(Qt::NoPen);
    p.setBrush(dot);
    p.drawEllipse(64, 64, S - 128, S - 128);
    p.end();

    QIcon icon(pm);
    for (int s : {16, 20, 24, 32, 48, 64}) {
        QPixmap hiPix = pm.scaled(s * 2, s * 2, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        hiPix.setDevicePixelRatio(2.0);   // 不用 small 作变量名：Windows SDK 有 #define small char
        icon.addPixmap(hiPix);
    }
    return icon;
}

}  // namespace Theme
