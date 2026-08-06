#include "minikv.h"

int main()
{
    MiniKV kv;
    return kv.Set("startup", "ready").ok() ? 0 : 1;
}
