#include <stdio.h>

long long x = 0x100000000LL;
long long y = 0x1ffffffffLL;
long long z = 0x010000000LL;
long long w = 0x01fffffffLL;

void
main(void)
{
	printf("%lld\n", x);
  /* fprintf(stdout, "x = 0x%016Lx\n", x); */
  /* fprintf(stdout, "x+1 = 0x%016Lx\n", x+1); */
  /* fprintf(stdout, "x-1 = 0x%016Lx\n", x-1); */
  /* fprintf(stdout, "y+1 = 0x%016Lx\n", (y+1)/4); */
  /* fprintf(stdout, "x+y = 0x%016Lx\n", x+y); */
  /* fprintf(stdout, "z*w = 0x%016Lx\n", z*w); */
  if (x > y ) printf("x > y\n");
  else printf("x < y\n");
  exit(0);
}
 
