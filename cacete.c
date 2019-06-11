#include <stdlib.h>
#include <stdio.h>
typedef struct {
    int id;
    int a[]
} CU;

int main()
{
    CU *s = malloc (sizeof(*s) + sizeof(int)*3);
    s->a[0] = 1;
    s->a[1] = 2;
    s->a[2] = 3;
    int a=2;
    int c=1;
    char st[]="DIEGO";
    printf("%d %d %d", s->a[0],s->a[1],s->a[2] );
    s= realloc(s,sizeof(*s)+sizeof(int)*4);
    s->a[3]=1230;
    printf("-%d-%d-%d-",s->a[0], s->a[1], s->a[3]);
}
