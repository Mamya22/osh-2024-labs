![picture](src/picture.jpeg "picture")

``` cpp
#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#define LIST_INIT_SIZE 5000
void Initial_list(NumList *numSet)
{
    numSet->elem = (A *)malloc(LIST_INIT_SIZE * sizeof(A));
    if (!numSet->elem)
        exit(1);
    numSet->length = 0;
    numSet->listSize = LIST_INIT_SIZE;
} // 初始化顺序表，分配内存并且初始化长度为0
```

$$
	x = \frac{-b\pm\sqrt{b^2-4ac}}{2a}
$$
