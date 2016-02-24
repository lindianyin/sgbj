
#include <utils/lockfree.h>
#include <iostream>

int main()
{
    queue_t q;
    data_type d1 = NULL;
    q.push(d1);
    data_type d2 = q.pop();
    std::cout<<d2<<std::endl;
    return 0;
}

