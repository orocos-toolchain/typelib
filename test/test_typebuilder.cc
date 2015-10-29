#include <boost/test/auto_unit_test.hpp>

#include <lang/csupport/standard_types.hh>

#include <test/testsuite.hh>
#include <typelib/typebuilder.hh>
using namespace Typelib;

BOOST_AUTO_TEST_CASE( test_TypeBuilder_parseName_extracts_arrays )
{
    TypeBuilder::ParsedTypename result = TypeBuilder::parseTypename("/a/type[10]");
    BOOST_REQUIRE_EQUAL("/a/type", result.first);
    BOOST_REQUIRE_EQUAL(1, result.second.size());
    BOOST_REQUIRE_EQUAL(Type::Array, result.second.front().category);
    BOOST_REQUIRE_EQUAL(10, result.second.front().size);
}

BOOST_AUTO_TEST_CASE( test_TypeBuilder_parseName_extracts_pointers )
{
    TypeBuilder::ParsedTypename result = TypeBuilder::parseTypename("/a/type*");
    BOOST_REQUIRE_EQUAL("/a/type", result.first);
    BOOST_REQUIRE_EQUAL(1, result.second.size());
    BOOST_REQUIRE_EQUAL(Type::Pointer, result.second.front().category);
    BOOST_REQUIRE_EQUAL(1, result.second.front().size);
}

BOOST_AUTO_TEST_CASE( test_TypeBuilder_parseName_extracts_mixtures_of_arrays_and_pointers )
{
    TypeBuilder::ParsedTypename result = TypeBuilder::parseTypename("/a/type*[10][20]");
    BOOST_REQUIRE_EQUAL("/a/type", result.first);
    BOOST_REQUIRE_EQUAL(3, result.second.size());

    TypeBuilder::ModifierList::const_iterator it = result.second.begin();
    {
        TypeBuilder::Modifier m = *it;
        BOOST_REQUIRE_EQUAL(Type::Pointer, m.category);
        BOOST_REQUIRE_EQUAL(1, m.size);
    }
    {
        TypeBuilder::Modifier m = *(++it);
        BOOST_REQUIRE_EQUAL(Type::Array, m.category);
        BOOST_REQUIRE_EQUAL(10, m.size);
    }
    {
        TypeBuilder::Modifier m = *(++it);
        BOOST_REQUIRE_EQUAL(Type::Array, m.category);
        BOOST_REQUIRE_EQUAL(20, m.size);
    }
}

BOOST_AUTO_TEST_CASE( test_TypeBuilder_parseName_ignores_arrays_and_pointers_in_template_markers )
{
    TypeBuilder::ParsedTypename result = TypeBuilder::parseTypename("/a/type</char[10]>");
    BOOST_REQUIRE_EQUAL("/a/type</char[10]>", result.first);
}

BOOST_AUTO_TEST_CASE( test_TypeBuilder_getBaseTypename_extracts_arrays )
{
    BOOST_REQUIRE_EQUAL("/a/type", TypeBuilder::getBaseTypename("/a/type[10]"));
}

BOOST_AUTO_TEST_CASE( test_TypeBuilder_getBaseTypename_extracts_pointers )
{
    BOOST_REQUIRE_EQUAL("/a/type", TypeBuilder::getBaseTypename("/a/type*"));
}

BOOST_AUTO_TEST_CASE( test_TypeBuilder_getBaseTypename_extracts_mixtures_of_arrays_and_pointers )
{
    BOOST_REQUIRE_EQUAL("/a/type", TypeBuilder::getBaseTypename("/a/type*[10][20]"));
}

BOOST_AUTO_TEST_CASE( test_TypeBuilder_getBaseTypename_ignores_arrays_and_pointers_in_template_markers )
{
    BOOST_REQUIRE_EQUAL("/a/type</char[10]>", TypeBuilder::getBaseTypename("/a/type</char[10]>"));
}

