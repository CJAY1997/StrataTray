# 外部主题扩展（External Themes）

把任意 `.qss` 文件放进本目录（或发行包的 `release/themes/`），重启 StrataTray 后
它会自动出现在「🎨 外观主题」二级子菜单中，**无需修改任何 C++ 代码，也无需重新编译**。

文件名的主体就是主题 id，例如 `dracula.qss` → id `dracula`。

## 可选元数据指令

在 `.qss` 顶部用注释声明（全部可选）：

```css
/* @theme-name Dracula */
/* @theme-dark true */
/* @theme-name-key theme_dracula */
```

| 指令 | 含义 | 缺省 |
|---|---|---|
| `@theme-name` | 菜单里显示的兜底名称 | 文件 id |
| `@theme-dark` | `false` 表示亮色系（Windows 标题栏会变亮） | `true` |
| `@theme-name-key` | 多语言字典里的 key，优先于 `@theme-name` | `theme_<id>` |

若想让菜单名称支持多语言，请同时向 `resources/i18n/en.json` 与 `zh_CN.json`
添加同名 key（例如 `theme_dracula`）。缺少 key 时自动回退到 `@theme-name`。

## 最小示例

`cyberpunk.qss`：

```css
/* @theme-name Cyberpunk */
/* @theme-dark true */
QWidget { background-color: #0b0f1a; color: #e0f2fe; font-size: 10pt; }
QWidget#card { background-color: #131a2b; border: 1px solid #1e3a5f; border-radius: 10px; }
QLabel#cardTitle { color: #f0abfc; font-size: 11pt; font-weight: 600; }
QPushButton { background-color: #1e293b; color: #e0f2fe; border: 1px solid #334155; border-radius: 7px; padding: 7px 14px; }
```

> 提示：主题只需覆盖你关心的选择器；未覆盖的控件会退回 Qt 默认样式。
> 内置 `dark` / `light` 的 id 为保留值，外部同名文件会被忽略。
