#include <typelib/registry.hh>
#include <typelib/typemodel.hh>
#include <typelib/value_ops.hh>
#include <boost/tuple/tuple.hpp>
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

    size_t getElementCount(void const* ptr) const
    {
        size_t byte_count = reinterpret_cast< std::vector<int8_t> const* >(ptr)->size();
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

    void insert(void* ptr, Value v) const
    {
        if (v.getType() != getIndirection())
            throw std::runtime_error("type mismatch in vector insertion");

        std::vector<uint8_t>* vector_ptr =
            reinterpret_cast< std::vector<uint8_t>* >(ptr);

        size_t offset = vector_ptr->size();
        vector_ptr->resize(offset + getIndirection().getSize());
        Typelib::copy(
                Value(&(*vector_ptr)[offset], getIndirection()),
                v);
    }

    bool erase(void* ptr, Value v) const
    {
        if (v.getType() != getIndirection())
            throw std::runtime_error("type mismatch in vector insertion");

        std::vector<uint8_t>* vector_ptr =
            reinterpret_cast< std::vector<uint8_t>* >(ptr);
        Type const& element_t  = getIndirection();
        size_t   element_size  = element_t.getSize();
        size_t   element_count = getElementCount(vector_ptr);

        uint8_t* base_ptr = &(*vector_ptr)[0];

        for (int i = 0; i < element_count; ++i)
        {
            uint8_t* element_ptr = base_ptr + i * element_size;
            Value    element_v(element_ptr, element_t);
            if (Typelib::compare(element_v, v))
            {
                // First, destroy the object at this place
                Typelib::destroy(element_v);
                // Then shrink the vector
                vector<uint8_t>::iterator element_it =
                    vector_ptr->begin() + i * element_size;
                vector_ptr->erase(element_it, element_it + element_size);
                return true;
            }
        }
        return false;
    }

    bool compare(void* ptr, void* other) const
    {
        std::vector<uint8_t>* a_ptr =
            reinterpret_cast< std::vector<uint8_t>* >(ptr);
        std::vector<uint8_t>* b_ptr =
            reinterpret_cast< std::vector<uint8_t>* >(other);

        size_t   element_count = getElementCount(a_ptr);
        Type const& element_t  = getIndirection();
        size_t   element_size  = element_t.getSize();
        if (element_count != getElementCount(b_ptr))
            return false;

        uint8_t* base_a = &(*a_ptr)[0];
        uint8_t* base_b = &(*b_ptr)[0];
        for (int i = 0; i < element_count; ++i)
        {
            if (!Typelib::compare(
                        Value(base_a + i * element_size, element_t),
                        Value(base_b + i * element_size, element_t)))
                return false;
        }
        return true;
    }

    void copy(void* dst, void* src) const
    {
        std::vector<uint8_t>* dst_ptr =
            reinterpret_cast< std::vector<uint8_t>* >(dst);
        std::vector<uint8_t>* src_ptr =
            reinterpret_cast< std::vector<uint8_t>* >(src);

        Type const& element_t = getIndirection();
        size_t element_count = getElementCount(src_ptr);
        size_t element_size  = element_t.getSize();
        dst_ptr->resize(element_t.getSize() * element_count);

        uint8_t* base_src = &(*src_ptr)[0];
        uint8_t* base_dst = &(*dst_ptr)[0];
        for (int i = 0; i < element_count; ++i)
        {
            Typelib::copy(
                    Value(base_dst + i * element_size, element_t),
                    Value(base_src + i * element_size, element_t));
        }
    }

    bool visit(void* ptr, ValueVisitor& visitor) const
    {
        std::vector<uint8_t>* vector_ptr =
            reinterpret_cast< std::vector<uint8_t>* >(ptr);
        uint8_t* base = &(*vector_ptr)[0];
        size_t   element_size  = getIndirection().getSize();
        size_t   element_count = getElementCount(vector_ptr);

        for (int i = 0; i < element_count; ++i)
            visitor.dispatch(Value(base + i * element_size, getIndirection()));

        return true;
    }

    void delete_if_impl(void* ptr, DeleteIfPredicate& pred) const
    {
        std::vector<uint8_t>* vector_ptr =
            reinterpret_cast< std::vector<uint8_t>* >(ptr);

        size_t   element_count = getElementCount(vector_ptr);
        Type const&  element_t = getIndirection();
        size_t   element_size  = element_t.getSize();

        uint8_t* base = &(*vector_ptr)[0];
        for (int i = 0; i < element_count; )
        {
            uint8_t* element_ptr = base + i * element_size;
            Value    element_v(element_ptr, element_t);
            if (pred.should_delete(element_v))
            {
                Typelib::destroy(element_v);
                vector<uint8_t>::iterator element_it =
                    vector_ptr->begin() + i * element_size;
                vector_ptr->erase(element_it, element_it + element_size);
                element_count--;
            }
            else
                ++i;
        }
    }

    bool isElementMemcpy(MarshalOps::const_iterator begin, MarshalOps::const_iterator end) const
    {
        return (end - begin >= 3 &&
                *begin == MemLayout::FLAG_MEMCPY &&
                *(begin + 2) == MemLayout::FLAG_END);
    }

    MarshalOps::const_iterator dump(
            void const* container_ptr, size_t element_count, std::vector<uint8_t>& buffer,
            MarshalOps::const_iterator const begin, MarshalOps::const_iterator const end) const
    {
        std::vector<uint8_t> const* vector_ptr =
            reinterpret_cast< std::vector<uint8_t> const* >(container_ptr);

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
            MarshalOps::const_iterator it_end = begin;
            size_t in_offset = 0;
            for (int i = 0; i < element_count; ++i)
            {
                boost::tie(in_offset, it_end) = ValueOps::dump(
                        &(*vector_ptr)[i * getIndirection().getSize()], 0,
                        buffer, begin, end);
            }
            return it_end;
        }
    }

    boost::tuple<size_t, MarshalOps::const_iterator> load(
            void* container_ptr, size_t element_count,
            std::vector<uint8_t> const& buffer, size_t in_offset,
            MarshalOps::const_iterator const begin, MarshalOps::const_iterator const end) const
    {
        std::vector<uint8_t>* vector_ptr =
            reinterpret_cast< std::vector<uint8_t>* >(container_ptr);

        Type const& element_t = getIndirection();
        size_t      element_size = element_t.getSize();
        vector_ptr->resize(element_count * element_size);

        for (int i = 0; i < element_count; ++i)
            Typelib::init(Value(&(*vector_ptr)[i * element_size], element_t));

        MarshalOps::const_iterator it = begin;
        if (isElementMemcpy(begin, end))
        {
            size_t size       = *(++it) * element_count;
            memcpy(&(*vector_ptr)[0], &buffer[in_offset], size);
            return boost::make_tuple(in_offset + size, begin + 2);
        }
        else
        {
            MarshalOps::const_iterator it_end;
            size_t out_offset = 0;
            for (int i = 0; i < element_count; ++i)
            {
                boost::tie(out_offset, in_offset, it_end) =
                    ValueOps::load(&(*vector_ptr)[i * element_size], 0,
                        buffer, in_offset, begin, end);
            }
            return boost::make_tuple(in_offset, it_end);
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

