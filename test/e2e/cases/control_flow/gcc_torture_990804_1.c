// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/990804-1.c
package main;

int gfbyte ( void ) 
{
 return 0;
} 

int main( void ) 
{
 int i,j,k ;

 i = gfbyte();

 i = i + 1 ;

 if ( i == 0 ) 
     k = -0 ;
 else
     k = i + 0 ;

 if (i != 1)
   return 1;

 k = 1 ;
 if ( k <= i)
     do 
	 j = gfbyte () ;
     while ( k++ < i ) ;

 return 0;
}