#include "plugin.hh"

using namespace std;

Plugin::Plugin(const std::string& name, const std::string& type)
    : m_name(name), m_type(type) {}
Plugin::~Plugin() {}

std::string Plugin::getName() const { return m_name; }
std::string Plugin::getType() const { return m_type; }
std::list<std::string> Plugin::getOptions() const { return list<string>(); }


