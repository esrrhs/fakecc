// expect: 0
package main;

extern void abort(void);
extern void exit(int);

enum { CHAR_BIT = 8 };

char c = ((char)0xf234);
short s = ((short)0xf234);
int i = ((int)0xf234);
long l = ((long)0xf2345678L);
long long ll = ((long long)0xf2345678abcdef0LL);

int shift1 = 4;
int shift2 = ((int)(sizeof (long long) * CHAR_BIT) - 4);

int
main (void)
{
  if ((((c) >> (shift1)) | ((c) << ((int)(sizeof (c) * CHAR_BIT) - (shift1)))) != ((((char)0xf234) >> (4)) | (((char)0xf234) << ((int)(sizeof (((char)0xf234)) * CHAR_BIT) - (4)))))
    abort ();

  if ((((s) >> (shift1)) | ((s) << ((int)(sizeof (s) * CHAR_BIT) - (shift1)))) != ((((short)0xf234) >> (4)) | (((short)0xf234) << ((int)(sizeof (((short)0xf234)) * CHAR_BIT) - (4)))))
    abort ();

  if ((((i) >> (shift1)) | ((i) << ((int)(sizeof (i) * CHAR_BIT) - (shift1)))) != ((((int)0xf234) >> (4)) | (((int)0xf234) << ((int)(sizeof (((int)0xf234)) * CHAR_BIT) - (4)))))
    abort ();

  if ((((l) >> (shift1)) | ((l) << ((int)(sizeof (l) * CHAR_BIT) - (shift1)))) != ((((long)0xf2345678L) >> (4)) | (((long)0xf2345678L) << ((int)(sizeof (((long)0xf2345678L)) * CHAR_BIT) - (4)))))
    abort ();

  if ((((ll) >> (shift1)) | ((ll) << ((int)(sizeof (ll) * CHAR_BIT) - (shift1)))) != ((((long long)0xf2345678abcdef0LL) >> (4)) | (((long long)0xf2345678abcdef0LL) << ((int)(sizeof (((long long)0xf2345678abcdef0LL)) * CHAR_BIT) - (4)))))
    abort ();

  if ((((ll) >> (shift2)) | ((ll) << ((int)(sizeof (ll) * CHAR_BIT) - (shift2)))) != ((((long long)0xf2345678abcdef0LL) >> (shift2)) | (((long long)0xf2345678abcdef0LL) << ((int)(sizeof (((long long)0xf2345678abcdef0LL)) * CHAR_BIT) - (shift2)))))
    abort ();

  exit (0);
}
