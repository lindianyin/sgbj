#pragma once

#define CAS  __sync_bool_compare_and_swap 
#define CAS2(p,o,n)  __sync_bool_compare_and_swap_16((long long *)(p), *(long long *)(&o), *(long long *)(&n)) 

