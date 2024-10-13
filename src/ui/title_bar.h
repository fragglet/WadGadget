//
// Copyright(C) 2022-2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#ifndef UI__TITLE_BAR_H_INCLUDED
#define UI__TITLE_BAR_H_INCLUDED

#include "ui/pane.h"
#include "ui/text_input.h"

struct pane *UI_TitleBarInit(void);
void UI_ShowNotice(const char *msg, ...);

#endif /* #ifndef UI__TITLE_BAR_H_INCLUDED */
