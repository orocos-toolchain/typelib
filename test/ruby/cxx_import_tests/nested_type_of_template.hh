/* This one is testing for a problem found in the gccxml/castxml importer
 * when using types defined within a template*/

#include <vector>

namespace ns
{
    template<typename T>
    class Context
    {
        public:
            struct Parameter
            {
                T field;
            };
            Parameter params;
    };
}

class Instanciator
{
public:
    std::vector<ns::Context<int>::Parameter> field;
};
