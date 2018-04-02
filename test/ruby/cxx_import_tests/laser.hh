#ifndef TEST_HEADER_DATA_LASER_H
#define TEST_HEADER_DATA_LASER_H

namespace Laser
{
    typedef int array_typedef[256];
    struct Data
    {
        int sec;
        unsigned int usec;

        double ranges[256];
    };
}

#endif /*TEST_HEADER_DATA_LASER_H*/
