/*
Copyright (c) 2009-2011, yellow wood goblin
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL CHYYRAN BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <ff.h>

#include <stdlib.h>

#include <wchar.h>
#include <stdlib.h>
#include <wctype.h>

static TCHAR CvtBuf[FF_MAX_LFN + 1] __attribute__((aligned(4)));

TCHAR *mbstoucs2(const char *src, size_t *len)
{
    mbstate_t ps = {0};
    wchar_t tempChar;
    int bytes;
    TCHAR *dst = CvtBuf;
    while (src)
    {
        bytes = mbrtowc(&tempChar, src, MB_CUR_MAX, &ps);
        if (bytes > 0)
        {
            *dst = (TCHAR)tempChar;
            src += bytes;
            dst++;
        }
        else if (bytes == 0)
        {
            break;
        }
        else
        {
            dst = CvtBuf;
            break;
        }
    }
    *dst = '\0';
    if (len)
        *len = dst - CvtBuf;
    return CvtBuf;
}

size_t ucs2tombs(char *dst, const TCHAR *src)
{
    mbstate_t ps = {0};
    size_t count = 0;
    int bytes;
    char buff[MB_CUR_MAX + 1];
    int i;

    while (*src != '\0')
    {
        bytes = wcrtomb(buff, *src, &ps);
        if (bytes < 0)
        {
            return -1;
        }
        if (bytes > 0)
        {
            for (i = 0; i < bytes; i++)
            {
                *dst++ = buff[i];
            }
            src++;
            count += bytes;
        }
        else
        {
            break;
        }
    }
    *dst = L'\0';
    return count;
}
