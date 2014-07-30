#ifndef TEST_HEADER_DATA_TEMPLATES_H
#define TEST_HEADER_DATA_TEMPLATES_H

#include <Eigen/Core>
#include <Eigen/Geometry>

namespace templates {

    namespace eigen {

        typedef Eigen::Matrix<float,3,4> aMatrix;
        struct S1 {
            aMatrix M;
        };

        struct S2 {
            Eigen::Vector3f V;
        };

        struct S3 {
            Eigen::Quaternionf Q;
        };
    }

    // just a testcase...
    template<class T>
    class C1 {
        T A;
    };

}

#endif /*TEST_HEADER_DATA_TEMPLATES_H*/
