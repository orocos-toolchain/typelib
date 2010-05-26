#ifndef TYPELIB_LANG_C_CONTAINERS_HH
#define TYPELIB_LANG_C_CONTAINERS_HH

#include <typelib/typemodel.hh>

class Vector : public Typelib::Container
{
    bool is_memcpy;
    void resize(std::vector<uint8_t>* ptr, size_t new_size) const;
    void copy(std::vector<uint8_t>* dst_ptr, size_t dst_idx, std::vector<uint8_t>* src_ptr, size_t src_idx, size_t count) const;
    void erase(std::vector<uint8_t>* ptr, size_t idx) const;

public:
    static std::string fullName(Typelib::Type const& on);
    Vector(Typelib::Type const& on);

    size_t getElementCount(void const* ptr) const;
    void init(void* ptr) const;
    void destroy(void* ptr) const;
    void clear(void*) const;
    long getNaturalSize() const;
    void insert(void* ptr, Typelib::Value v) const;
    bool erase(void* ptr, Typelib::Value v) const;
    bool compare(void* ptr, void* other) const;
    void copy(void* dst, void* src) const;
    bool visit(void* ptr, Typelib::ValueVisitor& visitor) const;
    void delete_if_impl(void* ptr, DeleteIfPredicate& pred) const;

    MarshalOps::const_iterator dump(
            void const* container_ptr, size_t element_count, Typelib::ValueOps::OutputStream& stream,
            MarshalOps::const_iterator const begin, MarshalOps::const_iterator const end) const;

    MarshalOps::const_iterator load(
            void* container_ptr, size_t element_count,
            Typelib::ValueOps::InputStream& stream,
            MarshalOps::const_iterator const begin, MarshalOps::const_iterator const end) const;

    static Container const& factory(Typelib::Registry& registry, std::list<Typelib::Type const*> const& on);
    ContainerFactory getFactory() const;
};

class String : public Typelib::Container
{
public:
    String(Typelib::Registry const& registry);

    size_t getElementCount(void const* ptr) const;
    void init(void* ptr) const;
    void destroy(void* ptr) const;
    void clear(void*) const;

    long getNaturalSize() const;

    void insert(void* ptr, Typelib::Value v) const;

    bool erase(void* ptr, Typelib::Value v) const;

    bool compare(void* ptr, void* other) const;

    void copy(void* dst, void* src) const;

    bool visit(void* ptr, Typelib::ValueVisitor& visitor) const;

    MarshalOps::const_iterator dump(
            void const* container_ptr, size_t element_count, Typelib::ValueOps::OutputStream& stream,
            MarshalOps::const_iterator const begin, MarshalOps::const_iterator const end) const;

    MarshalOps::const_iterator load(
            void* container_ptr, size_t element_count,
            Typelib::ValueOps::InputStream& stream,
            MarshalOps::const_iterator const begin, MarshalOps::const_iterator const end) const;

    void delete_if_impl(void* ptr, DeleteIfPredicate& pred) const;

    static Container const& factory(Typelib::Registry& registry, std::list<Type const*> const& on);
    ContainerFactory getFactory() const;
};


#endif

