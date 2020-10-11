#include <ff.h>

#include <stdlib.h>

#include <wchar.h>
#include <stdlib.h>
#include <wctype.h>

static TCHAR CvtBuf[FF_MAX_LFN + 1];

TCHAR *_ELM_mbstoucs2(const char *src, size_t *len)
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

size_t _ELM_ucs2tombs(char *dst, const TCHAR *src)
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
