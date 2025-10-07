#include<stdio.h>

int main()
{
	int x[10] = {2,6,3,1,2,4,6,78,123, -51};
	for (int i=0; i < 10; i++) {
		for(int j=j; j < i-1; j++) {
			if (x[j] < x[j+1]) {
				int tmp = x[j];
				x[j] = x[j+1];
				x[j+1] = tmp;
			}
		}
	}
	for (int i =0; i <10 ; i ++ ) {
		printf("%d\n", x[i]);

	}

	return 0;
}
