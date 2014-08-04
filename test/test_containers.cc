#include <boost/test/auto_unit_test.hpp>
#include <vector>

#include <test/testsuite.hh>
#include <typelib/utilmm/configset.hh>
#include <typelib/pluginmanager.hh>
#include <typelib/importer.hh>
#include <typelib/typemodel.hh>
#include <typelib/registry.hh>
#include <typelib/value.hh>
#include <typelib/value_ops.hh>
#include "test_cimport.1"
#include <string.h>
using namespace Typelib;
using namespace std;

template <int ptrsize> struct ptr_value_getter {};
template <> struct ptr_value_getter<4> { typedef uint32_t result; };
template <> struct ptr_value_getter<8> { typedef uint64_t result; };

typedef ptr_value_getter<sizeof(void *)>::result ptr_value_t;
ptr_value_t ptr_value(void *ptr) { return reinterpret_cast<ptr_value_t>(ptr); };

BOOST_AUTO_TEST_CASE(test_vector_assumptions) {
    // It is expected that ->size() depends on the type of the vector elements,
    // but that the size can be offset by using the ratio of sizeof()
    {
        vector<int32_t> values;
        values.resize(10);
        BOOST_REQUIRE_EQUAL(10, values.size());
        BOOST_REQUIRE_EQUAL(5,
                            reinterpret_cast<vector<int64_t> &>(values).size());
    }
    {
        vector<vector<double> > values;
        values.resize(10);
        BOOST_REQUIRE_EQUAL(10, values.size());
        BOOST_REQUIRE_EQUAL(10 * sizeof(vector<double>),
                            reinterpret_cast<vector<int8_t> &>(values).size());
    }

    // To resize the containers efficiently, we assume that moving the bytes
    // around is fine. This is a valid assumption only if the std::vector's
    // internal structure does not store any pointer to the memory location
    //
    // This tests that this assumption is valid
    {
        vector<vector<double> > values;
        values.resize(10);

        uint8_t *values_p = reinterpret_cast<uint8_t *>(&values);
        uint8_t *it_p = reinterpret_cast<uint8_t *>(&values);
        ptr_value_t vector_min = ptr_value(&values);
        ptr_value_t vector_max = ptr_value(&values) + sizeof(values);

        while ((it_p + sizeof(void *)) - values_p <
               static_cast<int>(sizeof(values))) {
            ptr_value_t p = *reinterpret_cast<ptr_value_t *>(it_p);
            BOOST_REQUIRE(!(p >= vector_min && p < vector_max));
            ++it_p;
        }
    }
}

static void import_test_types(Registry &registry) {
    static const char *test_file = TEST_DATA_PATH("test_cimport.tlb");

    utilmm::config_set config;
    PluginManager::self manager;
    auto_ptr<Importer> importer(manager->importer("tlb"));
    BOOST_REQUIRE_NO_THROW(importer->load(test_file, config, registry));
}

struct AssertValueVisit : public ValueVisitor {
    vector<int32_t> values;
    bool visit_(int32_t &v) {
        values.push_back(v);
        return true;
    }
};

BOOST_AUTO_TEST_CASE(test_vector_defines_aliases) {
    Registry registry;
    import_test_types(registry);
    registry.alias("/B", "/BAlias");
    Container const &container =
        Container::createContainer(registry, "/std/vector", *registry.get("B"));

    Type const *aliased = registry.get("/std/vector</BAlias>");
    BOOST_REQUIRE(aliased);
    BOOST_REQUIRE(*aliased == container);
}

BOOST_AUTO_TEST_CASE(test_vector_getElementCount) {
    Registry registry;
    import_test_types(registry);
    Container const &container =
        Container::createContainer(registry, "/std/vector", *registry.get("B"));

    std::vector<B> vector;
    vector.resize(10);
    BOOST_REQUIRE_EQUAL(10, container.getElementCount(&vector));
}

BOOST_AUTO_TEST_CASE(test_vector_init_destroy) {
    Registry registry;
    import_test_types(registry);
    Container const &container =
        Container::createContainer(registry, "/std/vector", *registry.get("B"));
    BOOST_REQUIRE_EQUAL(Type::Container, container.getCategory());

    void *v_memory = malloc(sizeof(std::vector<B>));
    container.init(v_memory);
    BOOST_REQUIRE_EQUAL(0, container.getElementCount(v_memory));
    container.destroy(v_memory);
    free(v_memory);
}

BOOST_AUTO_TEST_CASE(test_vector_push) {
    Registry registry;
    import_test_types(registry);
    Container const &container = Container::createContainer(
        registry, "/std/vector", *registry.get("int"));

    std::vector<int> vector;
    for (int value = 0; value < 10; ++value) {
        container.push(&vector, Value(&value, container.getIndirection()));
        BOOST_REQUIRE_EQUAL(value + 1, vector.size());

        for (int i = 0; i < value; ++i)
            BOOST_REQUIRE_EQUAL(i, vector[i]);
    }
}

BOOST_AUTO_TEST_CASE(test_vector_erase) {
    Registry registry;
    import_test_types(registry);
    Container const &container = Container::createContainer(
        registry, "/std/vector", *registry.get("int"));

    std::vector<int> vector;
    for (int value = 0; value < 10; ++value)
        vector.push_back(value);

    int value = 20;
    BOOST_REQUIRE(
        !container.erase(&vector, Value(&value, container.getIndirection())));
    value = 5;
    BOOST_REQUIRE(
        container.erase(&vector, Value(&value, container.getIndirection())));

    BOOST_REQUIRE_EQUAL(9, vector.size());
    for (int i = 0; i < 5; ++i)
        BOOST_REQUIRE_EQUAL(i, vector[i]);
    for (int i = 5; i < 9; ++i)
        BOOST_REQUIRE_EQUAL(i + 1, vector[i]);
}

bool test_delete_if_pred(Value v) {
    int *value = reinterpret_cast<int *>(v.getData());
    return (*value > 3) && (*value < 6);
}
BOOST_AUTO_TEST_CASE(test_vector_delete_if) {
    Registry registry;
    import_test_types(registry);
    Container const &container = Container::createContainer(
        registry, "/std/vector", *registry.get("int"));

    std::vector<int> vector;
    for (int value = 0; value < 10; ++value)
        vector.push_back(value);

    container.delete_if(&vector, test_delete_if_pred);

    BOOST_REQUIRE_EQUAL(8, vector.size());
    for (int i = 0; i < 4; ++i)
        BOOST_REQUIRE_EQUAL(i, vector[i]);
    for (int i = 4; i < 8; ++i)
        BOOST_REQUIRE_EQUAL(i + 2, vector[i]);
}

BOOST_AUTO_TEST_CASE(test_vector_getElement) {
    Registry registry;
    import_test_types(registry);

    Container const &container = Container::createContainer(
        registry, "/std/vector", *registry.get("int32_t"));

    std::vector<int32_t> v;
    v.resize(10);
    for (int i = 0; i < 10; ++i)
        v[i] = i;

    for (int i = 0; i < 10; ++i)
        BOOST_REQUIRE_EQUAL(
            i, *reinterpret_cast<int *>(container.getElement(&v, i).getData()));
}

BOOST_AUTO_TEST_CASE(test_vector_setElement) {
    Registry registry;
    import_test_types(registry);

    Container const &container = Container::createContainer(
        registry, "/std/vector", *registry.get("int32_t"));

    std::vector<int32_t> v;
    v.resize(10);

    for (int i = 0; i < 10; ++i)
        container.setElement(&v, i,
                             Typelib::Value(&i, container.getIndirection()));

    for (int i = 0; i < 10; ++i)
        BOOST_REQUIRE_EQUAL(i, v[i]);
}

BOOST_AUTO_TEST_CASE(test_vector_visit) {
    Registry registry;
    import_test_types(registry);

    Container const &container = Container::createContainer(
        registry, "/std/vector", *registry.get("int32_t"));

    std::vector<int32_t> v;
    v.resize(10);
    for (int i = 0; i < 10; ++i)
        v[i] = i;
    BOOST_REQUIRE_EQUAL(10, container.getElementCount(&v));

    // Try visiting the value
    Value value(&v, container);
    AssertValueVisit visitor;
    visitor.apply(value);

    BOOST_REQUIRE_EQUAL(10, visitor.values.size());
    for (int i = 0; i < 10; ++i)
        BOOST_REQUIRE_EQUAL(i, visitor.values[i]);
}

BOOST_AUTO_TEST_CASE(test_erase_collection_of_collections) {
    Registry registry;
    import_test_types(registry);
    Container const &inside = Container::createContainer(
        registry, "/std/vector", *registry.get("int32_t"));
    Container const &container =
        Container::createContainer(registry, "/std/vector", inside);

    std::vector<std::vector<int32_t> > v;
    v.resize(10);
    for (int i = 0; i < 10; ++i) {
        v[i].resize(10);
        for (int j = 0; j < 10; ++j)
            v[i][j] = i * 100 + j;
    }
    std::vector<int32_t> new_element;
    new_element.push_back(1000);
    v.insert(v.begin() + 5, new_element);

    BOOST_REQUIRE(container.erase(&v, Value(&new_element, inside)));

    BOOST_REQUIRE_EQUAL(10, v.size());

    for (int i = 0; i < 10; ++i) {
        BOOST_REQUIRE_EQUAL(10, v[i].size());
        for (int j = 0; j < 10; ++j)
            BOOST_REQUIRE_EQUAL(i * 100 + j, v[i][j]);
    }
}

BOOST_AUTO_TEST_CASE(test_copy_collection_of_collections) {
    Registry registry;
    import_test_types(registry);
    Container const &inside = Container::createContainer(
        registry, "/std/vector", *registry.get("int32_t"));
    Container const &container =
        Container::createContainer(registry, "/std/vector", inside);

    std::vector<std::vector<int32_t> > v;
    v.resize(10);
    for (int i = 0; i < 10; ++i) {
        v[i].resize(10);
        for (int j = 0; j < 10; ++j)
            v[i][j] = i * 100 + j;
    }

    std::vector<std::vector<int32_t> > copy;
    Typelib::copy(Value(&copy, container), Value(&v, container));

    for (int i = 0; i < 10; ++i) {
        BOOST_REQUIRE_EQUAL(10, v[i].size());
        for (int j = 0; j < 10; ++j)
            BOOST_REQUIRE_EQUAL(i * 100 + j, v[i][j]);
    }

    BOOST_REQUIRE(
        Typelib::compare(Value(&copy, container), Value(&v, container)));
}
