#include<stdio.h>
int add(int x, int y)
{
	return x + y;
}
int main()
{
	for(int i=0;i<100;i++) 	{
		i = add(i, i+1);
	}
	return 0;
}
