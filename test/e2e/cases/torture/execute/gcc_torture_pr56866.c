// expect: 0
package main;

/* PR target/56866 */

extern void abort (void);
extern void *memset (void *, int, unsigned long);

int
main (void)
{
  unsigned long long wq[256], rq[256];
  unsigned int wi[256], ri[256];
  unsigned short ws[256], rs[256];
  unsigned char wc[256], rc[256];
  int t;

  memset (wq, 0, sizeof wq);
  memset (wi, 0, sizeof wi);
  memset (ws, 0, sizeof ws);
  memset (wc, 0, sizeof wc);
  wq[0] = 0x0123456789abcdefULL;
  wi[0] = 0x01234567;
  ws[0] = 0x4567;
  wc[0] = 0x73;

  for (t = 0; t < 256; ++t)
    rq[t] = (wq[t] >> 8) | (wq[t] << (sizeof (wq[0]) * 8 - 8));
  for (t = 0; t < 256; ++t)
    ri[t] = (wi[t] >> 8) | (wi[t] << (sizeof (wi[0]) * 8 - 8));
  for (t = 0; t < 256; ++t)
    rs[t] = (ws[t] >> 9) | (ws[t] << (sizeof (ws[0]) * 8 - 9));
  for (t = 0; t < 256; ++t)
    rc[t] = (wc[t] >> 5) | (wc[t] << (sizeof (wc[0]) * 8 - 5));

  if (rq[0] != 0xef0123456789abcdULL || rq[1])
    abort ();
  if (ri[0] != 0x67012345 || ri[1])
    abort ();
  if (rs[0] != 0xb3a2 || rs[1])
    abort ();
  if (rc[0] != 0x9b || rc[1])
    abort ();
  return 0;
}
