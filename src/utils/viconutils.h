#ifndef VICONUTILS_H
#define VICONUTILS_H

#include <QString>
#include <QIcon>

#include "vpalette.h"

extern VPalette *g_palette;


class VIconUtils
{
public:
    // Get an icon from @p_file file. May change the foreground of the icon.
    static QIcon icon(const QString &p_file, const QString &p_fg = QString());

    static QIcon toolButtonIcon(const QString &p_file)
    {
        return icon(p_file, g_palette->color("toolbutton_icon_fg"));
    }

    static QIcon toolButtonDangerIcon(const QString &p_file)
    {
        return icon(p_file, g_palette->color("toolbutton_icon_danger_fg"));
    }

    static QIcon menuIcon(const QString &p_file)
    {
        return icon(p_file, g_palette->color("menu_icon_fg"));
    }

    static QIcon menuDangerIcon(const QString &p_file)
    {
        return icon(p_file, g_palette->color("menu_icon_danger_fg"));
    }

    static QIcon toolBoxIcon(const QString &p_file)
    {
        return icon(p_file, g_palette->color("toolbox_icon_fg"));
    }

    static QIcon toolBoxActiveIcon(const QString &p_file)
    {
        return icon(p_file, g_palette->color("toolbox_icon_active_fg"));
    }

    static QIcon comboBoxIcon(const QString &p_file)
    {
        return icon(p_file, g_palette->color("combobox_item_icon_fg"));
    }

    static QIcon treeViewIcon(const QString &p_file)
    {
        return icon(p_file, g_palette->color("treeview_item_icon_fg"));
    }

    static QIcon tabBarIcon(const QString &p_file)
    {
        return icon(p_file, g_palette->color("tabbar_icon_fg"));
    }

    static QIcon tabBarSpecialIcon(const QString &p_file)
    {
        return icon(p_file, g_palette->color("tabbar_icon_special_fg"));
    }

    static QIcon editWindowCornerIcon(const QString &p_file)
    {
        return icon(p_file, g_palette->color("editwindow_corner_icon_fg"));
    }

    static QIcon editWindowCornerInactiveIcon(const QString &p_file)
    {
        return icon(p_file, g_palette->color("editwindow_corner_icon_inactive_fg"));
    }

    static QIcon titleIcon(const QString &p_file)
    {
        return icon(p_file, g_palette->color("title_icon_fg"));
    }

    static QIcon buttonIcon(const QString &p_file)
    {
        return icon(p_file, g_palette->color("button_icon_fg"));
    }

    static QIcon buttonDangerIcon(const QString &p_file)
    {
        return icon(p_file, g_palette->color("button_icon_danger_fg"));
    }

private:
    VIconUtils();
};

#endif // VICONUTILS_H
