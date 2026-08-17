// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20030316-1.c
package main;

/* PR target/9164 */
/* The comparison operand was sign extended erraneously.  */

int
main (void)
{
    long j = 0x40000000;
    if ((unsigned int) (0x40000000 + j) < 0L)
 	return 1;

    return 0;
}