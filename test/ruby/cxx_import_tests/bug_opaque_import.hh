#ifndef OPAQUES_HH
#define OPAQUES_HH

class OpaquePoint
{
    int padding; // to make sure that OpaquePoint and Point do not look the same
    int _x, _y;
public:
    OpaquePoint(int x = 0, int y = 0)
        : _x(x), _y(y) {}

    int x() const { return _x; }
    int y() const { return _y; }
};

struct OpaqueContainingType
{
    OpaquePoint field;
};

struct NonExportedType
{
    OpaquePoint field;
};

#endif

