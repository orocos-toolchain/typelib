#include "../NamingConversions.hpp"
#include <algorithm>
#include <iostream>
#include <vector>

std::vector<std::string> template_tokenizer(std::string name);

void test_template_tokenizer(const std::string &val) {
    std::cout << "test_template_tokenizer: '" << val << "'\n";

    std::vector<std::string> retval = template_tokenizer(val);
    // c++11, wooohooot!
    for (auto str : retval) {
        std::cout << "    '" << str << "'\n";
    }
}

int main(int argc, char *argv[]) {

    // should not touch this string
    test_template_tokenizer("this is a test string");
    // getting serious
    test_template_tokenizer("std::basic_ostream<char>");
    // the following ones are coming from the "test_typelib_gccxml.rb"
    test_template_tokenizer("std::vector<std::pair<double, double> >");
    test_template_tokenizer("std::vector<int, std::allocator<int> >");
    test_template_tokenizer("std::vector <std::vector <int "
                            ",std::allocator<int>>, std::allocator< "
                            "std::vector<int , std::allocator <int>>      > >");
    test_template_tokenizer(
        "std::vector <std::vector <int ,std::allocator<int>>, std::allocator< "
        "std::vector<int , std::allocator <int>>      > >::size_t");

    return 0;
}

