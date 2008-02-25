typedef struct Compound
{
    int a;
    short b;
    char c;
    int array[10];
} Compound;

struct CompoundInCompound {
    Compound test[10];
};

typedef enum Enum { A, B, C } Enum;

typedef Enum OtherEnum;

#ifdef IDL_POINTER_ALIAS
typedef int* Pointer;
#endif
#ifdef IDL_POINTER_IN_STRUCT
struct InvalidStruct
{
    int* value;
};
#endif
#ifdef IDL_MULTI_ARRAY
struct MultiArrayStruct
{
    int array[10][20];
};
#endif

