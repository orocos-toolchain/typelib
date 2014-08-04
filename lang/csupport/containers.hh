#ifndef TYPELIB_LANG_C_CONTAINERS_HH
#define TYPELIB_LANG_C_CONTAINERS_HH

#include <typelib/typemodel.hh>

class Vector : public Typelib::Container {
    bool is_memcpy;
    using Typelib::Container::resize;
    void resize(std::vector<uint8_t> *ptr, size_t new_size) const;
    void copy(std::vector<uint8_t> *dst_ptr, size_t dst_idx,
              std::vector<uint8_t> *src_ptr, size_t src_idx,
              size_t count) const;
    void erase(std::vector<uint8_t> *ptr, size_t idx) const;
    std::string getIndirectTypeName(std::string const &element_name) const;

  public:
    static std::string fullName(std::string const &element_name);
    Vector(Typelib::Type const &on);

    bool isRandomAccess() const;
    void setElement(void *ptr, int idx, Typelib::Value value) const;
    Typelib::Value getElement(void *ptr, int idx) const;

    size_t getElementCount(void const *ptr) const;
    void init(void *ptr) const;
    void destroy(void *ptr) const;
    void clear(void *) const;
    long getNaturalSize() const;
    void push(void *ptr, Typelib::Value v) const;
    bool erase(void *ptr, Typelib::Value v) const;
    bool compare(void *ptr, void *other) const;
    void copy(void *dst, void *src) const;
    bool visit(void *ptr, Typelib::ValueVisitor &visitor) const;
    void delete_if_impl(void *ptr, DeleteIfPredicate &pred) const;

    MarshalOps::const_iterator dump(void const *container_ptr,
                                    size_t element_count,
                                    Typelib::OutputStream &stream,
                                    MarshalOps::const_iterator const begin,
                                    MarshalOps::const_iterator const end) const;

    MarshalOps::const_iterator load(void *container_ptr, size_t element_count,
                                    Typelib::InputStream &stream,
                                    MarshalOps::const_iterator const begin,
                                    MarshalOps::const_iterator const end) const;

    static Container const &factory(Typelib::Registry &registry,
                                    std::list<Typelib::Type const *> const &on);
    ContainerFactory getFactory() const;
};

class String : public Typelib::Container {
  public:
    String(Typelib::Registry const &registry);

    size_t getElementCount(void const *ptr) const;
    void init(void *ptr) const;
    void destroy(void *ptr) const;
    void clear(void *) const;

    long getNaturalSize() const;

    void push(void *ptr, Typelib::Value v) const;

    bool erase(void *ptr, Typelib::Value v) const;

    bool compare(void *ptr, void *other) const;

    void copy(void *dst, void *src) const;

    bool visit(void *ptr, Typelib::ValueVisitor &visitor) const;

    MarshalOps::const_iterator dump(void const *container_ptr,
                                    size_t element_count,
                                    Typelib::OutputStream &stream,
                                    MarshalOps::const_iterator const begin,
                                    MarshalOps::const_iterator const end) const;

    MarshalOps::const_iterator load(void *container_ptr, size_t element_count,
                                    Typelib::InputStream &stream,
                                    MarshalOps::const_iterator const begin,
                                    MarshalOps::const_iterator const end) const;

    void delete_if_impl(void *ptr, DeleteIfPredicate &pred) const;

    static Container const &factory(Typelib::Registry &registry,
                                    std::list<Type const *> const &on);
    ContainerFactory getFactory() const;

    static Type const &getElementType(Typelib::Registry const &registry);

    // This method is never called since we redefine modifiedDependencyAliases
    std::string getIndirectTypeName(std::string const &element_name) const {
        return "";
    }
    void modifiedDependencyAliases(Typelib::Registry &registry) const {}
};

#endif
