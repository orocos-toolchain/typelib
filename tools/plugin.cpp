#include "plugin.h"

using namespace std;

Plugin::Plugin(const std::string& name, Type type)
    : m_name(name), m_type(type) {}
Plugin::~Plugin() {}

std::string   Plugin::getName() const { return m_name; }
Plugin::Type  Plugin::getType() const { return m_type; }
std::list<std::string> Plugin::getOptions() const { return list<string>(); }

