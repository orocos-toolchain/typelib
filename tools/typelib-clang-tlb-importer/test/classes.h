#ifndef TEST_HEADER_DATA_CLASSES_H
#define TEST_HEADER_DATA_CLASSES_H

// testing stuff with classes

struct timespec;

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

}

#endif /*TEST_HEADER_DATA_CLASSES_H*/
