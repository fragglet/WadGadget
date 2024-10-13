//
// Copyright(C) 2022-2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#ifndef CONV__ERROR_H_INCLUDED
#define CONV__ERROR_H_INCLUDED

void ClearConversionErrors(void);
void ConversionError(char *fmt, ...);
const char *GetConversionError(void);

#endif /* #ifndef CONV__ERROR_H_INCLUDED */
