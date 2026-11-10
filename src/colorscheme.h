/*
    Qalculate (GTK UI)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef COLOR_SCHEME_H
#define COLOR_SCHEME_H

// Follow (or stop following) the light/dark preference of the XDG desktop portal
// (org.freedesktop.appearance color-scheme), using gtk-application-prefer-dark-theme.
// Nothing is done if gtk-application-prefer-dark-theme is already enabled by the user's GTK configuration.
void color_scheme_follow_system(bool follow);

#endif /* COLOR_SCHEME_H */
