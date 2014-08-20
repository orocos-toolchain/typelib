#ifndef TEST_HEADER_DATA_CLASSES_H
#define TEST_HEADER_DATA_CLASSES_H

// testing stuff with classes

struct timespec;

struct S1 { long l[2]; };


namespace classes {

    // we have a base-class with a member
    class BaseClass {
        int a;

        double thisIsSomeFancyFunction(struct timespec* input) {
            a = -10;
            float local_variable = 10000.0f;
            double another(a + local_variable);
            return another;
        }
    };

    // an inheriting class which adds nothing to the interface
    class FirstLevel : public classes::BaseClass {
    };

    // and finally someone adding another int.
    class NextLevel : public classes::FirstLevel {
        int b;
    };

    class NextWith_anonymous_InName {
        int spamspamspamspamspamspamspamspamspamspamspamspamspamspamspamspamspam;
    };

    struct With_sizeof_InName {
        int holymolymotherfucker;
    };

    class StructWithAnonStruct {
        int b;
        class {
            int c;
        } firstAnonVariant;
        struct secondAnonVariant{
            int e;
        };
    };

    // second: test the "base_classes" meta-data feature
    class C1 { double a; };
    class C2 : public C1 { double b; };
    class C3 : public S1, private C1 { double c; };

}

#endif /*TEST_HEADER_DATA_CLASSES_H*/
