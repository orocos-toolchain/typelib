#include <boost/test/auto_unit_test.hpp>
#include <vector>

#include <test/testsuite.hh>
#include <utilmm/configfile/configset.hh>
#include <utilmm/stringtools.hh>
#include <typelib/pluginmanager.hh>
#include <typelib/importer.hh>
#include <typelib/typemodel.hh>
#include <typelib/registry.hh>
#include <typelib/value.hh>
#include "test_cimport.1"
using namespace Typelib;
using namespace std;
using utilmm::split;
using utilmm::join;

BOOST_AUTO_TEST_CASE( test_vector_assumptions )
{
    // It is expected that ->size() depends on the type of the vector elements,
    // but that the size can be offset by using the ratio of sizeof()
    vector<int32_t> values;
    values.resize(10);
    BOOST_REQUIRE_EQUAL(10, values.size());
    BOOST_REQUIRE_EQUAL(5, reinterpret_cast< vector<int64_t>& >(values).size());
}

static void import_test_types(Registry& registry)
{
    static const char* test_file = TEST_DATA_PATH("test_cimport.h");

    utilmm::config_set config;
    PluginManager::self manager;
    Importer* importer = manager->importer("c");
    config.set("include", TEST_DATA_PATH(".."));
    config.set("define", "GOOD");
    BOOST_REQUIRE_NO_THROW( importer->load(test_file, config, registry) );
}

struct AssertValueVisit : public ValueVisitor
{
    vector<int32_t> values;
    bool visit_(int32_t& v)
    {
        values.push_back(v);
        return true;
    }
};

BOOST_AUTO_TEST_CASE( test_vector )
{
    Registry registry;
    import_test_types(registry);

    Container const& container = Container::createContainer(registry, "/std/vector", *registry.get("B"));

    void* v_memory = malloc(sizeof(std::vector<B>));
    std::vector<B>* v = new(v_memory) std::vector<B>();
    v->resize(10);
    BOOST_REQUIRE_EQUAL(10, container.getElementCount(v));

    container.destroy(v);
    free(v);
}

BOOST_AUTO_TEST_CASE( test_vector_visit )
{ 
    Registry registry;
    import_test_types(registry);

    Container const& container = Container::createContainer(registry, "/std/vector", *registry.get("int32_t"));

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

