#include "containers.hh"
#include <typelib/registry.hh>
#include <typelib/typemodel.hh>
#include <typelib/value_ops.hh>
#include <boost/tuple/tuple.hpp>
#include <string.h>

using namespace Typelib;
using namespace std;

BOOST_STATIC_ASSERT(( sizeof(vector<void*>) == sizeof(vector<Container>) ));

string Vector::fullName(Type const& on)
{ return "/std/vector<" + on.getName() + ">"; }

Vector::Vector(Type const& on)
    : Container("/std/vector", fullName(on), getNaturalSize(), on)
{
    MemoryLayout ops = Typelib::layout_of(on);
    is_memcpy = (ops.size() == 2 && ops[0] == MemLayout::FLAG_MEMCPY);
}

size_t Vector::getElementCount(void const* ptr) const
{
    size_t byte_count = reinterpret_cast< std::vector<int8_t> const* >(ptr)->size();
    return byte_count / getIndirection().getSize();
}
void Vector::init(void* ptr) const
{
    new(ptr) vector<int8_t>();
}
void Vector::destroy(void* ptr) const
{
    std::vector<uint8_t>* vector_ptr =
        reinterpret_cast< std::vector<uint8_t>* >(ptr);
    resize(vector_ptr, 0);
    vector_ptr->~vector<uint8_t>();
}
void Vector::clear(void* ptr) const
{
    std::vector<uint8_t>* vector_ptr =
        reinterpret_cast< std::vector<uint8_t>* >(ptr);
    resize(vector_ptr, 0);
}

long Vector::getNaturalSize() const
{
    return sizeof(std::vector<void*>);
}

void Vector::resize(std::vector<uint8_t>* ptr, size_t new_size) const
{
    Type const& element_t = getIndirection();
    size_t element_size = getIndirection().getSize();

    size_t old_raw_size   = ptr->size();
    size_t old_size       = getElementCount(ptr);
    size_t new_raw_size   = new_size * element_size;

    if (!is_memcpy && old_size > new_size)
    {
        // Need to destroy the old elements
        for (size_t i = new_raw_size; i < old_raw_size; i += element_size)
            Typelib::destroy(Value(&(*ptr)[i], element_t));
    }

    ptr->resize(new_raw_size);

    if (!is_memcpy && old_size < new_size)
    {
        // Need to initialize the new elements
        for (size_t i = old_raw_size; i < new_raw_size; i += element_size)
            Typelib::init(Value(&(*ptr)[i], element_t));
    }
}

void Vector::insert(void* ptr, Value v) const
{
    if (v.getType() != getIndirection())
        throw std::runtime_error("type mismatch in vector insertion");

    std::vector<uint8_t>* vector_ptr =
        reinterpret_cast< std::vector<uint8_t>* >(ptr);

    size_t size = getElementCount(ptr);
    resize(vector_ptr, size + 1);
    Typelib::copy(
            Value(&(*vector_ptr)[size * getIndirection().getSize()], getIndirection()),
            v);
}

bool Vector::erase(void* ptr, Value v) const
{
    if (v.getType() != getIndirection())
        throw std::runtime_error("type mismatch in vector insertion");

    std::vector<uint8_t>* vector_ptr =
        reinterpret_cast< std::vector<uint8_t>* >(ptr);
    Type const& element_t  = getIndirection();
    size_t   element_size  = element_t.getSize();
    size_t   element_count = getElementCount(vector_ptr);

    uint8_t* base_ptr = &(*vector_ptr)[0];

    for (size_t i = 0; i < element_count; ++i)
    {
        uint8_t* element_ptr = base_ptr + i * element_size;
        Value    element_v(element_ptr, element_t);
        if (Typelib::compare(element_v, v))
        {
            erase(vector_ptr, i);
            return true;
        }
    }
    return false;
}

void Vector::erase(std::vector<uint8_t>* ptr, size_t idx) const
{
    // Copy the remaining elements to the right place
    size_t element_count = getElementCount(ptr);
    if (element_count > idx + 1)
        copy(ptr, idx, ptr, idx + 1, element_count - idx - 1);

    // Then shrink the vector
    resize(ptr, element_count - 1);
}

bool Vector::compare(void* ptr, void* other) const
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
    for (size_t i = 0; i < element_count; ++i)
    {
        if (!Typelib::compare(
                    Value(base_a + i * element_size, element_t),
                    Value(base_b + i * element_size, element_t)))
            return false;
    }
    return true;
}

void Vector::copy(void* dst, void* src) const
{
    std::vector<uint8_t>* dst_ptr =
        reinterpret_cast< std::vector<uint8_t>* >(dst);
    std::vector<uint8_t>* src_ptr =
        reinterpret_cast< std::vector<uint8_t>* >(src);

    size_t element_count = getElementCount(src_ptr);
    resize(dst_ptr, element_count);
    copy(dst_ptr, 0, src_ptr, 0, element_count);
}

void Vector::copy(std::vector<uint8_t>* dst_ptr, size_t dst_idx, std::vector<uint8_t>* src_ptr, size_t src_idx, size_t count) const
{
    Type const& element_t = getIndirection();
    size_t element_size = element_t.getSize();
    uint8_t* base_src = &(*src_ptr)[src_idx * element_size];
    uint8_t* base_dst = &(*dst_ptr)[dst_idx * element_size];
    if (is_memcpy)
    {
        if (dst_ptr == src_ptr)
            memmove(base_dst, base_src, element_size * count);
        else
            memcpy(base_dst, base_src, element_size * count);
    }
    else
    {
        for (size_t i = 0; i < count; ++i)
        {
            Typelib::copy(
                    Value(base_dst + i * element_size, element_t),
                    Value(base_src + i * element_size, element_t));
        }
    }
}


bool Vector::visit(void* ptr, ValueVisitor& visitor) const
{
    std::vector<uint8_t>* vector_ptr =
        reinterpret_cast< std::vector<uint8_t>* >(ptr);
    uint8_t* base = &(*vector_ptr)[0];
    size_t   element_size  = getIndirection().getSize();
    size_t   element_count = getElementCount(vector_ptr);

    for (size_t i = 0; i < element_count; ++i)
        visitor.dispatch(Value(base + i * element_size, getIndirection()));

    return true;
}

void Vector::delete_if_impl(void* ptr, DeleteIfPredicate& pred) const
{
    std::vector<uint8_t>* vector_ptr =
        reinterpret_cast< std::vector<uint8_t>* >(ptr);

    size_t   element_count = getElementCount(vector_ptr);
    Type const&  element_t = getIndirection();
    size_t   element_size  = element_t.getSize();

    uint8_t* base = &(*vector_ptr)[0];
    for (size_t i = 0; i < element_count; )
    {
        uint8_t* element_ptr = base + i * element_size;
        Value    element_v(element_ptr, element_t);
        if (pred.should_delete(element_v))
        {
            erase(vector_ptr, i);
            element_count--;
        }
        else
            ++i;
    }
}

Container::MarshalOps::const_iterator Vector::dump(
        void const* container_ptr, size_t element_count, ValueOps::OutputStream& stream,
        MarshalOps::const_iterator const begin, MarshalOps::const_iterator const end) const
{
    std::vector<uint8_t> const* vector_ptr =
        reinterpret_cast< std::vector<uint8_t> const* >(container_ptr);

    MarshalOps::const_iterator it = begin;
    if (is_memcpy)
    {
        // optimize a bit: do a huge memcpy if possible
        size_t size       = *(++it) * element_count;
        stream.write(&(*vector_ptr)[0], size);
        return begin + 2;
    }
    else
    {
        MarshalOps::const_iterator it_end = begin;
        size_t in_offset = 0;
        for (size_t i = 0; i < element_count; ++i)
        {
            boost::tie(in_offset, it_end) = ValueOps::dump(
                    &(*vector_ptr)[i * getIndirection().getSize()], 0,
                    stream, begin, end);
        }
        return it_end;
    }
}

Container::MarshalOps::const_iterator Vector::load(
        void* container_ptr, size_t element_count,
        ValueOps::InputStream& stream,
        MarshalOps::const_iterator const begin, MarshalOps::const_iterator const end) const
{
    std::vector<uint8_t>* vector_ptr =
        reinterpret_cast< std::vector<uint8_t>* >(container_ptr);

    Type const& element_t = getIndirection();
    size_t      element_size = element_t.getSize();
    resize(vector_ptr, element_count);

    MarshalOps::const_iterator it = begin;
    if (is_memcpy)
    {
        size_t size       = *(++it) * element_count;
        stream.read(&(*vector_ptr)[0], size);
        return begin + 2;
    }
    else
    {
        MarshalOps::const_iterator it_end;
        size_t out_offset = 0;
        for (size_t i = 0; i < element_count; ++i)
        {
            boost::tie(out_offset, it_end) =
                ValueOps::load(&(*vector_ptr)[i * element_size], 0,
                    stream, begin, end);
        }
        return it_end;
    }
}


Container const& Vector::factory(Registry& registry, std::list<Type const*> const& on)
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
Container::ContainerFactory Vector::getFactory() const { return factory; }



String::String(Typelib::Registry const& registry)
    : Container("/std/string", "/std/string", getNaturalSize(), *registry.get("char")) {}

size_t String::getElementCount(void const* ptr) const
{
    size_t byte_count = reinterpret_cast< std::string const* >(ptr)->length();
    return byte_count / getIndirection().getSize();
}
void String::init(void* ptr) const
{
    new(ptr) std::string();
}
void String::destroy(void* ptr) const
{
    reinterpret_cast< std::string* >(ptr)->~string();
}
void String::clear(void* ptr) const
{
    reinterpret_cast< std::string* >(ptr)->clear();
}


long String::getNaturalSize() const
{
    return sizeof(std::string);
}

void String::insert(void* ptr, Value v) const
{
    if (v.getType() != getIndirection())
        throw std::runtime_error("type mismatch in string insertion");

    std::string* string_ptr =
        reinterpret_cast< std::string* >(ptr);

    string_ptr->append(reinterpret_cast<std::string::value_type*>(v.getData()), 1);
}

bool String::erase(void* ptr, Value v) const
{
    return false;
}

bool String::compare(void* ptr, void* other) const
{
    std::string* a_ptr = reinterpret_cast< std::string* >(ptr);
    std::string* b_ptr = reinterpret_cast< std::string* >(other);
    return *a_ptr == *b_ptr;
}

void String::copy(void* dst, void* src) const
{
    std::string* dst_ptr = reinterpret_cast< std::string* >(dst);
    std::string* src_ptr = reinterpret_cast< std::string* >(src);
    *dst_ptr = *src_ptr;
}

bool String::visit(void* ptr, ValueVisitor& visitor) const
{
    std::string* string_ptr =
        reinterpret_cast< std::string* >(ptr);
    char* base = const_cast<char*>(string_ptr->c_str());
    size_t   element_count = string_ptr->length();

    for (size_t i = 0; i < element_count; ++i)
        visitor.dispatch(Value(base + i, getIndirection()));

    return true;
}

Container::MarshalOps::const_iterator String::dump(
        void const* container_ptr, size_t element_count, ValueOps::OutputStream& stream,
        MarshalOps::const_iterator const begin, MarshalOps::const_iterator const end) const
{
    const std::string* string_ptr =
        reinterpret_cast< const std::string* >(container_ptr);

    stream.write(reinterpret_cast<uint8_t const*>(string_ptr->c_str()), element_count);
    return begin + 2;
}

Container::MarshalOps::const_iterator String::load(
        void* container_ptr, size_t element_count,
        ValueOps::InputStream& stream,
        MarshalOps::const_iterator const begin, MarshalOps::const_iterator const end) const
{
    std::string* string_ptr =
        reinterpret_cast< std::string* >(container_ptr);

    std::vector<uint8_t> buffer;
    buffer.resize(element_count);
    stream.read(&buffer[0], element_count);
    (*string_ptr).append(reinterpret_cast<const char*>(&buffer[0]), element_count);
    return begin + 2;
}
void String::delete_if_impl(void* ptr, DeleteIfPredicate& pred) const
{}

Container const& String::factory(Registry& registry, std::list<Type const*> const& on)
{
    throw std::logic_error("factory() called for String");
}
Container::ContainerFactory String::getFactory() const { return factory; }

