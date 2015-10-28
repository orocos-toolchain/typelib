#ifndef TEST_HEADER_DATA_TEST_IDL_H
#define TEST_HEADER_DATA_TEST_IDL_H

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

typedef enum TestEnum { A, B, C } EnumAlias;

typedef TestEnum OtherEnum;

namespace NS1 {
    namespace NS1_1 {
	struct Test {
	    int a;
	    short b;
	};
    }

    namespace NS1_2 {
	typedef NS1_1::Test Test;
    }

    struct Test {
	NS1_1::Test test2;
    };
}

#endif /*TEST_HEADER_DATA_TEST_IDL_H*/
