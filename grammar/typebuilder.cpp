#include "typebuilder.h"
#include "type.h"

#include <numeric>
#include <sstream>

static std::string join(const std::string& acc, const std::string& add)
{
    if (acc.empty()) return add;
    return acc + ' ' + add;
}

TypeBuilder::TypeBuilder(const std::list<std::string>& name)
{
    std::string basename = accumulate(name.begin(), name.end(), std::string(), join);
    m_type = Type::fromName(basename);
    if (!m_type) throw NotFound(basename);
}
const Type* TypeBuilder::getType() const throw() { return m_type; }

void TypeBuilder::addPointer(int level) throw()
{
    while (level)
    {
        const Type* new_type = Type::fromName(m_type -> getName() + "*");
        if (! new_type)
            new_type = new Pointer(m_type);
        m_type = new_type;

        --level;
    }
}

void TypeBuilder::addArray(int new_dim) throw()
{
    // May seem weird, but we have to handle
    // int test[4][5] as a 4 item array of a 5 item array of int
    typedef std::list<const Array *> ArrayList;
    ArrayList array_chain;

    const Type* base_type;
    for (base_type = m_type; base_type -> getCategory() == Type::Array; base_type = base_type -> getNextType())
    {
        const Array* array = static_cast<const Array*>(base_type);
        array_chain.push_front(array);
    }

    std::string new_dim_string;
    {
        std::ostringstream stream;
        stream << "[" << new_dim << "]";
        new_dim_string = stream.str();
    }

    std::string basename = base_type -> getName() + new_dim_string;
    const Type* new_type = Type::fromName(basename);
    if (new_type)
    {
        while (new_type)
        {
            const Array* array = array_chain.front();
            array_chain.pop_front();

            base_type = new_type;
            new_type = Type::fromName(basename + array -> getDimString());
        }
    }
    else
        base_type = new Array(base_type, new_dim);

    while (!array_chain.empty())
    {
        const Array* array = array_chain.front();
        array_chain.pop_front();

        base_type = new Array(base_type, array -> getDimension());
    }
    
    m_type = base_type;
}


