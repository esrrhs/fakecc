// expect: 0
package main;

extern void abort(void);
extern void exit(int);

int x;

void __attribute__((noinline)) foo(int i) { x = i; }
void __attribute__((noinline)) bar(void) { exit(0); }

int
main(int argc, char *argv[])
{
	int i;
	int numbers[4] = { 0xdead, 0xbeef, 0x1337, 0x4242 };

	for (i = 1; i <= 12; i++) {
		if (i <= 4)
			foo(numbers[i-1]);
		else if (i >= 7 && i <= 9)
			bar();
	}

	abort();
}
