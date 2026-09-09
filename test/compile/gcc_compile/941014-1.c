f (char *to)
{
  unsigned int wch;
  register int length;
  unsigned char tmp;
  unsigned int mult = 10;

  tmp = (wch>>(unsigned int)(length * mult));
  *to++ = (unsigned char)tmp;
}
