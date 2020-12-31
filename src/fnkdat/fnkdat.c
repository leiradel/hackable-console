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

#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "fnkdat.h"

/* version is automatically generated
   #define FNKDAT_VERSION "0.0.8"
 */


#ifdef HAVE_CONFIG_H
#   include "config.h"
#endif

#ifndef PACKAGE
#   error PACKAGE is not defined
#endif

#ifndef FNKDAT_PKGDATADIR
#  define FNKDAT_PKGDATADIR "/usr/share/" PACKAGE
#endif

#ifndef FNKDAT_GAMESUBDIR
#  define FNKDAT_GAMESUBDIR "games/"
#endif

/*
 * BSD seems to put their game stuff in /var/games
 * as opposed to /var/lib/games
 */
#ifndef FNKDAT_PKGLIBDIR
#  ifdef __FreeBSD__
#     ifndef FNKDAT_GAMESUBDIR
#        define FNKDAT_GAMESUBDIR "lib/"
#     endif /* GAMESUBDIR */
#     define FNKDAT_PKGLIBDIR "/var/" FNKDAT_GAMESUBDIR PACKAGE
#  else
#     define FNKDAT_PKGLIBDIR "/var/lib/" FNKDAT_GAMESUBDIR PACKAGE
#  endif /* __FreeBSD__ */
#endif /* FNKDAT_PKGLIBDIR */

#ifndef FNKDAT_SYSCONFDIR
#  define FNKDAT_SYSCONFDIR "/etc"
#endif

/*
 * Used to make sure we don't append past the end of a buffer
 */
#define FNKDAT_S(op)                            	\
   if ((len = total - (int) _tcslen(buffer)) <= 0) {  	\
      errno = ENOMEM;                           	\
      return -1;                                	\
   }                                            	\
   op;


/*
 * do the mkdir(s), if asked to
 */
#define FNKDAT_MKDIRS

   


/************************
 * WIN32 IMPLEMENTATION *
 ************************/
#ifdef WIN32

#define WIN32_LEAN_AND_MEAN
#define STRICT
#include <windows.h>
#include <direct.h>

/*
 * Constants passed to the silly-ass MS function
 */
#define CSIDL_APPDATA         0x001a
#define CSIDL_FLAG_CREATE     0x8000
#define CSIDL_COMMON_APPDATA  0x0023
#define SHGFP_TYPE_CURRENT    0

/* these are used by the common functions defined below */
#define FNKDAT_FILE_SEPARATOR _T('\\')
#define stat _stat

#if defined(_UNICODE) || defined(UNICODE)
#  define FNKDAT_U   "W"
#else
#  define FNKDAT_U   "A"
#endif


/* defined below */
static int fnkdat_mkdirs(_TCHAR* buffer, int rlevel);


/*
 * Get the requested info
 */
int fnkdat(const _TCHAR* target, _TCHAR* buffer, int len, int flags) {

   static FARPROC SHGetFolderPath = NULL;
   static HMODULE hSHFolderDLL = NULL;
   static FARPROC GetUserName = NULL;
   static HMODULE hAdvapi32DLL = NULL;
   _TCHAR szPath[MAX_PATH];
   LPTSTR szCommandLine, szTmp;
   DWORD dwSize;
   int total,i;
   HRESULT hresult;
   DWORD dwFlags;
   int rawflags;

   if (buffer && len)
      buffer[0] = _T('\0');

   /* save room for the null term char
    */
   total = len - 1;

   /* Initialize, if requested to.  Note that initialize must be called
    * from a single thread before anything else.  Other then that,
    * there are no concurrency issues (as far as I know).
    */
   if (flags == FNKDAT_INIT) {

      if (hSHFolderDLL = LoadLibrary(_T("SHFOLDER.DLL"))) {

         /* one would expect that the strings below should be wrapped
            in a _T(), but the docs (and the compiler) say "no!"
          */
         SHGetFolderPath = GetProcAddress(
            hSHFolderDLL, "SHGetFolderPath" FNKDAT_U);

      } else {
         SHGetFolderPath = NULL;
      }

      if (hAdvapi32DLL = LoadLibrary(_T("ADVAPI32.DLL"))) {

         GetUserName = GetProcAddress(
            hAdvapi32DLL, "GetUserName" FNKDAT_U);

      } else {
         GetUserName = NULL;
      }

      return 0;
   }

   /* Uninitialize, if requested to -- probably not necessary but what
    * the hell, why not?
    */
   if (flags == FNKDAT_UNINIT) {
      if (hSHFolderDLL)
         FreeLibrary(hSHFolderDLL);
      if (hAdvapi32DLL)
         FreeLibrary(hAdvapi32DLL);

      return 0;
   }

   /* if target is absolute then simply return it
    */
   if (target) {
      if ((target[0] && target[0] == _T('\\'))
          || (target[0] && target[0] == _T('/'))
          || (target[0] && target[1] && target[1] == _T(':'))) {

         _tcsncpy(buffer, target, len);
         return 0;
      }
   }

   hresult = S_OK;
   dwFlags = 0;

   rawflags = flags & (0xFFFFFFFF ^ FNKDAT_CREAT);

   if (rawflags == FNKDAT_USER)
      dwFlags = CSIDL_APPDATA;
   else if (rawflags == (FNKDAT_VAR | FNKDAT_DATA))
      dwFlags = CSIDL_COMMON_APPDATA;


   /* Get the user conf directory using the silly-ass function if it
      is available.
    */
   if (dwFlags
       && SHGetFolderPath
       && SUCCEEDED(hresult = SHGetFolderPath(
         NULL,
         dwFlags | ((flags & FNKDAT_CREAT) ? CSIDL_FLAG_CREATE : 0),
         NULL,
         SHGFP_TYPE_CURRENT, 
         szPath))) {

         FNKDAT_S(_tcsncpy(buffer, szPath, len));
         FNKDAT_S(_tcsncat(buffer, _T("\\"), len));
         FNKDAT_S(_tcsncat(buffer, _T(PACKAGE), len));
         FNKDAT_S(_tcsncat(buffer, _T("\\"), len));




   /* We always compute the system conf and data directories
      relative to argv[0]

      Why not use SHGetFolderPath(...)
      for system conf??  Here's why.  I'm using this as the win32
      supplement for /etc/. If I used CSIDL_COMMON_APPDATA then
      it would only be available when that function is available
      which means I'd have to fallback to something different when
      it's not.  This would make app installation a royal pain
      because this system conf directory would vary depending on whether
      or not SHFOLDER.DLL happens to be installed.  I intend for this path
      to contain read-only "system" configuration data that is
      installed when the software is.  So, I'm saying it's relative
      to the executable, as that's what most existing software seems
      to do.
    */
   } else if ((flags == FNKDAT_CONF)
       || (flags == FNKDAT_USER)
       || (flags == FNKDAT_DATA)
       || (flags == (FNKDAT_VAR | FNKDAT_DATA))) {
      szCommandLine = GetCommandLine();

      /* argv[0] may be quoted -- if so, skip the quote
         and whack everything after the end quote
       */
      if (szCommandLine[0] == _T('"')) {

         szCommandLine++;
         _tcsncpy(buffer, szCommandLine, len);
         szTmp = buffer;

         while(szTmp[0] && szTmp[0] != _T('"'))
            szTmp++;

         szTmp[0] = _T('\0');

      /* otherwise, whack everything after the first
         space character
       */
      } else {
         _tcsncpy(buffer, szCommandLine, len);
         szTmp = buffer;

         while(szTmp[0] && !_istspace(szTmp[0]))
            szTmp++;

         szTmp[0] = _T('\0');
      }

            
      szTmp = _tcsrchr(buffer, _T('\\'));

      if (!szTmp)
         szTmp = _tcsrchr(buffer, _T('/'));

      if (szTmp) {
         szTmp++;
         szTmp[0] = _T('\0');

      } else {
         _tcsncpy(buffer, _T(".\\"), len);
      }

      /* this only happens when we don't have the silly-ass function */
      if (flags & FNKDAT_USER) {
         dwSize = MAX_PATH;

         FNKDAT_S(_tcsncat(buffer, _T("users\\"), len));

         /* Grab what windows thinks is the current user name */
         if (GetUserName(szPath, &dwSize) == TRUE) {
            FNKDAT_S(_tcsncat(buffer, szPath, len));

         /* if that fails, make something up */
         } else {
            FNKDAT_S(_tcsncat(buffer, _T("default"), len));
         }

         FNKDAT_S(_tcsncat(buffer, _T("\\"), len));
      }


   /* If we get here the user gave a bad flag
      or !SUCCEEDED(hresult)
    */
   } else {
      errno = EINVAL;
      return -1;
   }

   /* append any given filename */
   if (target) {
      FNKDAT_S(_tcsncat(buffer, target, len));
   }

   /* replace unix path characters w/ windows path chars
      so that the fnk_mkdirs funtion works
    */
   for(i = 0; buffer[i]; i++) {
      if (buffer[i] == _T('/'))
         buffer[i] = _T('\\');
   }

   /* do the mkdir(s), if asked to */
   if ((flags & FNKDAT_CREAT)
       && fnkdat_mkdirs(buffer, -1) < 0) {

      return -1;
   }

   return 0;
}


/************************
 * UNIX IMPLEMENTATION  *
 ************************/
#else

#include <pwd.h>
#include <unistd.h>

#ifndef FNKDAT_DIRMODE
#   define FNKDAT_DIRMODE 0775
#endif


/* these are used by the common functions defined below */
#define _TCHAR char
#define _T(s)  s
#define FNKDAT_FILE_SEPARATOR '/'
#define _tmkdir(d) mkdir(d, (mode_t)FNKDAT_DIRMODE)
#define _tcsrchr strrchr
#define _tcslen strlen
#define _tstat stat

/* defined below */
static int fnkdat_mkdirs(_TCHAR* buffer, int rlevel);


int fnkdat(const char* target, char* buffer, int len, int flags) {
   struct passwd* pwent;
   int total, rawflags;

   if (buffer && len)
      buffer[0] = '\0';

   /* save room for the null term char
    */
   total = len - 1;

   rawflags = flags & (0xFFFFFFFF ^ FNKDAT_CREAT);


   /* when we've got an absolute path we simply return it */
   if (target && target[0] == '/') {
      strncpy(buffer, target, len);
      return 0;
   }

   /* nothing to do */
   if (flags == FNKDAT_INIT
       || flags == FNKDAT_UNINIT) {
      return 0;
   }

   if (rawflags == FNKDAT_USER) {

      pwent = getpwuid(getuid());

      if (!pwent)
         return -1;

      strncpy(buffer, pwent->pw_dir, len);

      FNKDAT_S(strncat(buffer, "/." PACKAGE, len));

   } else if (rawflags == FNKDAT_CONF) {
      strncpy(buffer, FNKDAT_SYSCONFDIR, len);
      FNKDAT_S(strncat(buffer, "/" PACKAGE, len));

   } else if (rawflags == (FNKDAT_VAR | FNKDAT_DATA)) {
      strncpy(buffer, FNKDAT_PKGLIBDIR, len);

   } else if (rawflags == FNKDAT_DATA) {
      strncpy(buffer, FNKDAT_PKGDATADIR, len);

   } else {
      errno = EINVAL;
      return -1;
   }

   FNKDAT_S(strncat(buffer, "/", len));

   if (target) {
      FNKDAT_S(strncat(buffer, target, len));
   }


   /* do the mkdir(s), if asked to */
   if ((flags & FNKDAT_CREAT)
       && fnkdat_mkdirs(buffer, -1) < 0) {

      return -1;
   }


   return 0;
}

#endif /* WIN32 */


/********************
 * COMMON FUNCTIONS *
 ********************/

/*
 * This will make the resquested directory, along with
 * any necessary parent directory.
 */
static int fnkdat_mkdirs(_TCHAR* buffer, int rlevel) {

   _TCHAR* pos;
   struct stat statbuf;

   rlevel++;

   /* if this is the first time that we call this function,
      we want to skip past any filename that happens to
      be sitting there, and start working on directories
    */
   if (rlevel == 0) {

      /* if target has a file on the end, we don't
         want to make a directory w/ its name.  So
         we skip everything after the last FNKDAT_FILE_SEPARATOR.
       */
      pos = _tcsrchr(buffer, FNKDAT_FILE_SEPARATOR);
      if (pos)
         pos[0] = _T('\0');

      /* make the necessary directories.  If this fails,
         errno will already be set, so we simply return
         with an error
      */
      if (fnkdat_mkdirs(buffer, rlevel) < 0)
         return -1;

      pos[0] = FNKDAT_FILE_SEPARATOR;

      return 0;
   }


   /* if the directory exists, then we have nothing to do */
   if (_tstat(buffer, &statbuf) == 0)
      return 0;

   switch (errno) {

      case ENOENT:
         pos = _tcsrchr(buffer, FNKDAT_FILE_SEPARATOR);
         if (pos)
            pos[0] = _T('\0');

         if (fnkdat_mkdirs(buffer, rlevel) < 0)
            return -1;

         pos[0] = FNKDAT_FILE_SEPARATOR;

         if (_tmkdir(buffer) < 0)
            return -1;

         break;

      default:
         return -1;
   }


   return 0;
}

/* vi: set sw=3 ts=3 tw=78 et sts: */

