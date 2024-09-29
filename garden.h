#if !defined(MAIN_H)

#define internal static 
#define global_variable static
#define local_persist static

#define MAX_BUFFER 100

struct Asset
{
    int counter[MAX_BUFFER];
};

struct pcg32_random_t 
{
    uint64_t state;
    uint64_t inc;
};

struct Combination
{
    int background;
    int nose;
    int eyes;
    int hat;
    int id;
};

#define MAIN_H
#endif
