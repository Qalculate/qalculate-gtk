/*
    Qalculate (GTK UI)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef COLOR_SCHEME_H
#define COLOR_SCHEME_H

// gtk_theme value (last item of the theme combo) that follows the light/dark preference of the desktop
#define THEME_SYSTEM 4

// Follow (or stop following) the light/dark preference of the XDG desktop portal
// (org.freedesktop.appearance color-scheme), using gtk-application-prefer-dark-theme
void color_scheme_follow_system(bool follow);

#endif /* COLOR_SCHEME_H */
