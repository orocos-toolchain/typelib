#ifndef TEST_HEADER_DATA_CLASSES_H
#define TEST_HEADER_DATA_CLASSES_H

// testing stuff with classes

// this is just the declaration, no definition. should not end up in the
// database, although its written in this header.
struct timespec;

struct S1 { long l[2]; };

namespace classes {

    // we have a base-class with a member
    class BaseClass {
      public:
        int a;
        // some garbage:
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
      public:
        int b;
    };

    class NextWith_anonymous_InName : public NextLevel {
      public:
        int spamspamspamspamspamspamspamspamspamspamspamspamspamspamspamspamspam;
    };

    struct With_sizeof_InName {
        int holymolymotherfucker;
    };

    class StructWithAnonStruct {
      public:
        int b;
        // this class is anonymous and not exported. also causes the
        // sorrounding class to be ignored
        class {
          public:
            int c;
        } firstAnonVariant;
        // this is exported
        struct secondAnonVariant {
            int e;
        };
    };

    // The C* classes are opaques ... verify that the cxxname and
    // source_file_line metadata are properly set

    class C1 { public: double a; };
    class C2 : public C1 { public: double b; };
    class C3 : public S1, private C1 { public: double c; };

    class P1 {
      public:
        int a;
      private:
        int b;
    };

}

#endif /*TEST_HEADER_DATA_CLASSES_H*/
