#ifndef __TYPEBUILDER_H__
#define __TYPEBUILDER_H__

#include <string>
#include <list>

class Type;

class TypeBuilder
{
    std::string m_basename;
    const Type* m_type;
    
public:
    class NotFound
    {
        std::string m_name;
    public:
        NotFound(const std::string& name) : m_name(name) {}
        std::string toString() const { return "Type " + m_name + " not found"; }
    };

    TypeBuilder(const std::list<std::string>& base);

    void addPointer(int level) throw();
    void addArray(int size) throw();

    const Type* getType() const throw();
};

#endif

