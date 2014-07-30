#ifndef TEST_HEADER_DATA_CLASSES_H
#define TEST_HEADER_DATA_CLASSES_H

// testing stuff with classes

namespace classes {

    // we have a base-class with a member
    class BaseClass {
        int a;
    };

    // an inheriting class which adds nothing to the interface
    class FirstLevel : public classes::BaseClass {
    };

    // and finally someone adding another int.
    class NextLevel : public classes::FirstLevel {
        int b;
    };

}

#endif /*TEST_HEADER_DATA_CLASSES_H*/
