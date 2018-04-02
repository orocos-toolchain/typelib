#include "containers.hh"
#include <typelib/registry.hh>
#include <typelib/typemodel.hh>
#include <typelib/value_ops.hh>
#include <typelib/value_ops_details.hh>
#include <boost/tuple/tuple.hpp>
#include <string.h>
#include <limits>

#include <iostream>

using namespace Typelib;
using namespace std;

BOOST_STATIC_ASSERT(( sizeof(vector<void*>) == sizeof(vector<char>) ));

string Vector::fullName(std::string const& element_name)
{ return "/std/vector<" + element_name + ">"; }

Vector::Vector(Type const& on)
    : Container("/std/vector", fullName(on.getName()), getNaturalSize(), on)
    , element_layout(getElementLayout(on))
{
}

bool Vector::isElementMemcpy() const
{
    return element_layout.isMemcpy();
}

bool Vector::hasElementLayout() const
{
    return !element_layout.isEmpty();
}

MemoryLayout Vector::getElementLayout(Type const& element_t)
{
    try {
        return Typelib::layout_of(element_t);
    }
    catch(std::runtime_error)
    {
        return MemoryLayout();
    }
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

bool Vector::isRandomAccess() const
{ return true; }
void Vector::setElement(void* ptr, int idx, Typelib::Value value) const
{
    std::vector<uint8_t>* vector_ptr =
        reinterpret_cast< std::vector<uint8_t>* >(ptr);
    Typelib::copy(
        Value(&(*vector_ptr)[idx * getIndirection().getSize()], getIndirection()),
        value);
}

Typelib::Value Vector::getElement(void* ptr, int idx) const
{
    std::vector<uint8_t>* vector_ptr =
        reinterpret_cast< std::vector<uint8_t>* >(ptr);
    return Value(&(*vector_ptr)[idx * getIndirection().getSize()], getIndirection());
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

    if (!hasElementLayout())
        throw std::runtime_error("could not compute layout for element type " + element_t.getName() + ", cannot copy");

    if (isElementMemcpy())
    {
        ptr->resize(new_raw_size);
        return;
    }


    if (old_size > new_size)
    {
        // Need to destroy the elements that are at the end of the container
        for (size_t i = new_raw_size; i < old_raw_size; i += element_size)
            Typelib::destroy(&(*ptr)[i], element_layout);
    }


    if (ptr->capacity() >= new_raw_size)
        ptr->resize(new_raw_size);
    else
    {
        std::vector<uint8_t> new_buffer;
        new_buffer.resize(new_raw_size);
        size_t copy_raw_size = std::min(old_raw_size, new_raw_size);
        for (size_t i = 0; i < copy_raw_size; i += element_size)
        {
            Typelib::init(&new_buffer[i], element_layout);
            Typelib::copy(&new_buffer[i], &(*ptr)[i], element_layout);
        }
        ptr->swap(new_buffer);
    }

    if (old_size < new_size)
    {
        // Need to initialize the new elements at the end of the container
        for (size_t i = old_raw_size; i < new_raw_size; i += element_size)
            Typelib::init(&(*ptr)[i], element_layout);
    }
}

void Vector::push(void* ptr, Value v) const
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
        /// !!! We don't use element_layout here as comparing requires a
        // different layout. If performance becomes an issue, we'll have to
        // precompute both
        if (!Typelib::compare(
                    base_a + i * element_size,
                    base_b + i * element_size, element_t))
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
    if (isElementMemcpy())
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
    const Type &indirect(getIndirection());

    for (size_t i = 0; i < element_count; ++i)
        visitor.dispatch(Value(base + i * element_size, indirect));

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
        void const* container_ptr, size_t element_count, OutputStream& stream,
        MarshalOps::const_iterator const begin, MarshalOps::const_iterator const end) const
{
    std::vector<uint8_t> const* vector_ptr =
        reinterpret_cast< std::vector<uint8_t> const* >(container_ptr);

    MarshalOps::const_iterator it = begin;
    if (isElementMemcpy())
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
        InputStream& stream,
        MarshalOps::const_iterator const begin, MarshalOps::const_iterator const end) const
{
    std::vector<uint8_t>* vector_ptr =
        reinterpret_cast< std::vector<uint8_t>* >(container_ptr);

    Type const& element_t = getIndirection();
    size_t      element_size = element_t.getSize();
    resize(vector_ptr, element_count);

    MarshalOps::const_iterator it = begin;
    if (isElementMemcpy())
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

std::string Vector::getIndirectTypeName(std::string const& element_name) const
{
    return Vector::fullName(element_name);
}

Container const& Vector::factory(Registry& registry, std::list<Type const*> const& on)
{
    if (on.size() != 1)
        throw std::runtime_error("expected only one template argument for std::vector");

    Type const& contained_type = *on.front();
    std::string full_name = Vector::fullName(contained_type.getName());
    if (! registry.has(full_name))
    {
        Vector* new_type = new Vector(contained_type);
        registry.add(new_type);
        return *new_type;
    }

    const Type *type = registry.get(full_name);
    if (type->getCategory() != Type::Container) {
        throw BadCategory(type->getCategory(), Type::Container);
    } else {
        return dynamic_cast<Container const &>(*type);
    }
}
Container::ContainerFactory Vector::getFactory() const { return factory; }


Type const& String::getElementType(Typelib::Registry const& registry)
{
    std::string element_type_name;
    if (std::numeric_limits<char>::is_signed)
        element_type_name = "/int8_t";
    else
        element_type_name = "/uint8_t";

    Type const* element_type = registry.get(element_type_name);
    if (!element_type)
        throw std::runtime_error("cannot find string element " + element_type_name + " in registry");
    return *element_type;
}
String::String(Typelib::Registry const& registry)
    : Container("/std/string", "/std/string", getNaturalSize(), String::getElementType(registry)) {}

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

void String::push(void* ptr, Value v) const
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
        void const* container_ptr, size_t element_count, OutputStream& stream,
        MarshalOps::const_iterator const begin, MarshalOps::const_iterator const end) const
{
    const std::string* string_ptr =
        reinterpret_cast< const std::string* >(container_ptr);

    stream.write(reinterpret_cast<uint8_t const*>(string_ptr->c_str()), element_count);
    return begin + 2;
}

Container::MarshalOps::const_iterator String::load(
        void* container_ptr, size_t element_count,
        InputStream& stream,
        MarshalOps::const_iterator const begin, MarshalOps::const_iterator const end) const
{
    std::string* string_ptr =
        reinterpret_cast< std::string* >(container_ptr);

    string_ptr->clear();

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
    if (registry.has("/std/string"))
        return dynamic_cast<Container const&>(*registry.get("/std/string"));

    if (on.size() != 1)
        throw std::runtime_error("expected only one template argument for std::string");

    Type const& contained_type = *on.front();
    Type const& expected_type  = String::getElementType(registry);
    if (contained_type != expected_type)
        throw std::runtime_error("std::string can only be built on top of '" + expected_type.getName() + "' -- found " + contained_type.getName());

    String* new_type = new String(registry);
    registry.add(new_type);
    return *new_type;
}
Container::ContainerFactory String::getFactory() const { return factory; }

