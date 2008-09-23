#include <typelib/registry.hh>
#include <typelib/typemodel.hh>
#include <boost/tuple/tuple.hpp>
#include <typelib/marshalling.hh>
#include <string.h>

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
    void init(void* ptr) const
    {
        new(ptr) vector<int8_t>();
    }
    void destroy(void* ptr) const
    {
        reinterpret_cast< std::vector<int8_t>* >(ptr)->~vector<int8_t>();
    }

    bool visit(ValueVisitor& visitor, Value const& v) const
    {
        std::vector<uint8_t>* vector_ptr =
            reinterpret_cast< std::vector<uint8_t>* >(v.getData());
        uint8_t* base = &(*vector_ptr)[0];
        size_t   element_size = getIndirection().getSize();
        size_t   element_count = getElementCount(vector_ptr);

        for (int i = 0; i < element_count; ++i)
            visitor.dispatch(Value(base + i * element_size, getIndirection()));

        return true;
    }

    bool isElementMemcpy(MarshalOps::const_iterator begin, MarshalOps::const_iterator end) const
    {
        return (end - begin >= 3 &&
                *begin == MarshalOpsVisitor::FLAG_MEMCPY &&
                *(begin + 2) == MarshalOpsVisitor::FLAG_END);
    }

    MarshalOps::const_iterator dump(
            void* container_ptr, size_t element_count, std::vector<uint8_t>& buffer,
            MarshalOps::const_iterator const begin, MarshalOps::const_iterator const end) const
    {
        std::vector<uint8_t>* vector_ptr =
            reinterpret_cast< std::vector<uint8_t>* >(container_ptr);

        MarshalOps::const_iterator it = begin;
        if (isElementMemcpy(begin, end))
        {
            // optimize a bit: do a huge memcpy if possible
            size_t size       = *(++it) * element_count;
            size_t out_offset = buffer.size();
            buffer.resize( out_offset + size );
            memcpy(&buffer[out_offset], &(*vector_ptr)[0], size);
            return begin + 2;
        }
        else
        {
            MarshalOps::const_iterator it_end;
            size_t in_offset = 0;
            for (int i = 0; i < element_count; ++i)
            {
                boost::tie(it_end, in_offset) = MarshalOpsVisitor::dump(
                        &(*vector_ptr)[i + in_offset], in_offset,
                        buffer, begin, end);
            }
            return it_end;
        }
    }

    boost::tuple<MarshalOps::const_iterator, size_t> load(
            void* container_ptr, size_t element_count,
            std::vector<uint8_t> const& buffer, size_t in_offset,
            MarshalOps::const_iterator const begin, MarshalOps::const_iterator const end) const
    {
        std::vector<uint8_t>* vector_ptr =
            reinterpret_cast< std::vector<uint8_t>* >(container_ptr);

        vector_ptr->resize(element_count * getIndirection().getSize());

        MarshalOps::const_iterator it = begin;
        if (isElementMemcpy(begin, end))
        {
            size_t size       = *(++it) * element_count;
            memcpy(&(*vector_ptr)[0], &buffer[in_offset], size);
            return boost::make_tuple(begin + 2, in_offset + size);
        }
        else
        {
            MarshalOps::const_iterator it_end;
            size_t out_offset = 0;
            for (int i = 0; i < element_count; ++i)
            {
                boost::tie(it_end, out_offset, in_offset) = MarshalOpsVisitor::load(
                        &(*vector_ptr)[i + out_offset], out_offset,
                        buffer, in_offset, begin, end);
            }
            return boost::make_tuple(it_end, in_offset);
        }
    }


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

