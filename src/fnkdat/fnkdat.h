/*
   fnkdat - an interface for determining common directory names
   Copyright (C) 2001, 2002  David MacCormack
   $Header$

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

   As a special exception, David MacCormack gives permission
   for additional uses of the text contained in the files named
   ``fnkdat.h'' and ``fnkdat.c'', hereafter known as FNKDAT.

   The exception is that, if you link the FNKDAT with other files
   to produce an executable, this does not by itself cause the
   resulting executable to be covered by the GNU General Public License.
   Your use of that executable is in no way restricted on account of
   linking the FNKDAT code into it.

   This exception does not however invalidate any other reasons why
   the executable file might be covered by the GNU General Public License.

   This exception applies only to the code released by David MacCormack
   under the name FNKDAT.  If you copy code from other software into a
   copy of FNKDAT, as the General Public License permits, the exception does
   not apply to the code that you add in this way.  To avoid misleading
   anyone as to the status of such modified files, you must delete
   this exception notice from them.

   If you write modifications of your own for FNKDAT, it is your choice
   whether to permit this exception to apply to your modifications.
   If you do not wish that, delete this exception notice.

   David MacCormack (djm at maccormack dot net)

*/

#ifndef _FNKDAT_H
#define _FNKDAT_H

#define FNKDAT_CONF     0x01
#define FNKDAT_DATA     0x02
#define FNKDAT_VAR      0x04
#define FNKDAT_USER     0x08
#define FNKDAT_INIT     0x10
#define FNKDAT_UNINIT   0x20
#define FNKDAT_CREAT    0x80

/* version automatically populated */
#define FNKDAT_VERSION "0.0.8"

#ifdef __cplusplus
extern "C" {
#endif


#ifdef WIN32

/*
 * Include UNICODE crap
 */
#include <tchar.h>

/*
 * define in a UNICODE compatible way
 */
int fnkdat(const _TCHAR* target, _TCHAR* buffer, int len, int flags);

#else

/*
 * basic, lovable ANSI C
 */
int fnkdat(const char* target, char* buffer, int len, int flags);


#endif /* WIN32 */



#ifdef __cplusplus
}
#endif


#endif /* _FNKDAT_H */

/* vi: set sw=3 ts=3 tw=78 et sts: */

