/*
    UNN - text editor with high ambitions and far-fetched goals
    Copyright (C) 2025  Sergei Igolnikov

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef __UNN_FLAGS_H_
#define __UNN_FLAGS_H_

// f can't be 0

#define flag_toggle(flags, f) (flags = flags ^ f)

#define flag_on(flags, f) (flags = flags | f)

#define flag_off(flags, f) (flags = flags ^ (flags & f))

#define flag_is_on(flags, f) (flags & f)

#define flag_is_off(flags, f) (flags ^ f)

#endif