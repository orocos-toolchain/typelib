#include <typelib/registry.hh>
#include <typelib/typemodel.hh>

using namespace Typelib;
using namespace std;

BOOST_STATIC_ASSERT(( sizeof(vector<void*>) == sizeof(vector<Container>) ));
class Vector : public Container
{
public:
    static string fullName(Type const& on)
    { return "/std/vector<" + on.getName() + ">"; }

    Vector(Type const& on)
        : Container("/std/vector", fullName(on), sizeof(std::vector<void*>), on) {}

    size_t getElementCount(void* ptr) const
    {
        size_t byte_count = reinterpret_cast< std::vector<int8_t>* >(ptr)->size();
        return byte_count / getIndirection().getSize();
    }
    void destroy(void* ptr) const
    { reinterpret_cast< std::vector<int8_t>* >(ptr)->~vector<int8_t>(); }
    uint8_t* getElement(void* base, int i) const
    { return &(*reinterpret_cast< std::vector<uint8_t>* >(base))[0] + i * getIndirection().getSize(); }
    static Container const& factory(Registry& registry, std::list<Type const*> const& on)
    {
        if (on.size() != 1)
            throw std::runtime_error("expected only one template argument for std::vector");

        Type const& contained_type = *on.front();
        std::string full_name = Vector::fullName(contained_type);
        if (! registry.has(full_name))
        {
            Vector* new_type = new Vector(contained_type);
            registry.add(new_type);
            return *new_type;
        }
        else
            return dynamic_cast<Container const&>(*registry.get(full_name));
    }
    ContainerFactory getFactory() const { return factory; }
};

struct Registrar
{
    Registrar() {
        Container::registerContainer("/std/vector", Vector::factory);
    }
};

static Registrar registrar;

