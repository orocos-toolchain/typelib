#include "endianness.hh"
#include <iostream>
#include <boost/tuple/tuple.hpp>

namespace Typelib {
    size_t const CompileEndianSwapVisitor::FLAG_SKIP;
    size_t const CompileEndianSwapVisitor::FLAG_ARRAY;
    size_t const CompileEndianSwapVisitor::FLAG_END;
    size_t const CompileEndianSwapVisitor::FLAG_SWAP_4;
    size_t const CompileEndianSwapVisitor::FLAG_SWAP_8;

    size_t const CompileEndianSwapVisitor::SizeOfEnum;

    void CompileEndianSwapVisitor::skip(int skip_size)
    { 
	size_t size = m_compiled.size();
	if (m_compiled[size - 2] == FLAG_SKIP)
	    m_compiled[size - 1] += skip_size;
	else
	{
	    m_compiled.push_back(FLAG_SKIP);
	    m_compiled.push_back(skip_size);
	}
    }

    bool CompileEndianSwapVisitor::visit_ (Numeric const& type)
    {
	switch(type.getSize())
	{
	case 1:
	    skip(1);
	    return true;
	case 2:
	    m_compiled.push_back(m_output_index + 1);
	    m_compiled.push_back(m_output_index);
	    return true;

	case 4:
	    m_compiled.push_back(FLAG_SWAP_4);
	    return true;

	case 8:
	    m_compiled.push_back(FLAG_SWAP_8);
	    return true;

	default:
	    throw UnsupportedEndianSwap();
	}
    }

    bool CompileEndianSwapVisitor::visit_ (Enum const& type)
    {
	for (int i = SizeOfEnum - 1; i >= 0; --i)
	    m_compiled.push_back(m_output_index + i);
	return true;
    }

    bool CompileEndianSwapVisitor::visit_ (Pointer const& type)
    { throw UnsupportedEndianSwap(); }
    bool CompileEndianSwapVisitor::visit_ (Array const& type)
    {
	if (type.getIndirection().getCategory() == Type::Array)
	{
	    size_t current_size = m_compiled.size();
	    visit_(dynamic_cast<Array const&>(type.getIndirection()));
	    m_compiled[current_size + 1] *= type.getDimension();
	    return true;
	}

	m_compiled.push_back(FLAG_ARRAY);
	m_compiled.push_back(type.getDimension());
	m_compiled.push_back(type.getIndirection().getSize());

	size_t current_size = m_compiled.size();
	TypeVisitor::visit_(type);

	if (m_compiled.size() == current_size + 2 && m_compiled[current_size] == FLAG_SKIP)
	{
	    m_compiled[current_size - 3] = FLAG_SKIP;
	    m_compiled[current_size - 2] = m_compiled[current_size + 1] * type.getDimension();
	    m_compiled.pop_back();
	    m_compiled.pop_back();
	    m_compiled.pop_back();
	}
	else
	    m_compiled.push_back(FLAG_END);

	return true;
    }

    bool CompileEndianSwapVisitor::visit_ (Compound const& type)
    {
	size_t base_index = m_output_index;

	typedef Compound::FieldList Fields;
	Fields const& fields(type.getFields());
	Fields::const_iterator const end = fields.end();

	for (Fields::const_iterator it = fields.begin(); it != end; ++it)
	{
	    size_t new_index = base_index + it->getOffset();
	    if (new_index < m_output_index)
		continue;
	    else if (new_index > m_output_index)
		skip(new_index - m_output_index);

	    m_output_index = new_index;
	    dispatch(it->getType());
	    m_output_index = new_index + it->getType().getSize();
	}
	return true;
    }

    void CompileEndianSwapVisitor::apply(Type const& type)
    {
	m_output_index = 0;
	m_compiled.clear();
	TypeVisitor::apply(type);
    }

    std::pair<size_t, std::vector<size_t>::const_iterator> 
	CompileEndianSwapVisitor::swap(
	    size_t output_offset, size_t input_offset,
	    std::vector<size_t>::const_iterator it,
	    std::vector<size_t>::const_iterator end,
	    Value in, Value out)
    {
	uint8_t* input_buffer = reinterpret_cast<uint8_t*>(in.getData());
	uint8_t* output_buffer = reinterpret_cast<uint8_t*>(out.getData());
	while(it != end)
	{
	    switch(*it)
	    {
	    case FLAG_SKIP:
		{
		    size_t skip_size = *(++it);
		    for (size_t i = 0; i < skip_size; ++i)
		    {
			output_buffer[output_offset] = input_buffer[output_offset];
			output_offset++;
		    }
		}
		break;
	    case FLAG_SWAP_4:
		{
		    reinterpret_cast<uint32_t&>(output_buffer[output_offset]) = 
			utilmm::endian::swap(reinterpret_cast<uint32_t const&>(input_buffer[output_offset]));
		    output_offset += 4;
		}
		break;
	    case FLAG_SWAP_8:
		{
		    reinterpret_cast<uint64_t&>(output_buffer[output_offset]) = 
			utilmm::endian::swap(reinterpret_cast<uint64_t const&>(input_buffer[output_offset]));
		    output_offset += 8;
		}
		break;
	    case FLAG_ARRAY:
		{
		    size_t array_size = *(++it);
		    size_t element_size = *(++it);

		    // And swap as many elements as needed
		    ++it;

		    std::vector<size_t>::const_iterator array_end;
		    for (size_t i = 0; i < array_size; ++i)
		    {
			boost::tie(output_offset, array_end) = 
			    swap(output_offset, input_offset + element_size * i, 
				it, end, in, out);
		    }
		    it = array_end;
		}
		break;
	    case FLAG_END:
		return make_pair(output_offset, it);
	    default:
		{
		    output_buffer[output_offset] = input_buffer[input_offset + *it];
		    ++output_offset;
		}
		break;
	    }

	    ++it;
	}

	return make_pair(output_offset, it);
    }

    void CompileEndianSwapVisitor::display()
    {
	std::string indent = "";
	std::vector<size_t>::const_iterator it, end;
	for (it = m_compiled.begin(); it != m_compiled.end(); ++it)
	{
	    switch(*it)
	    {
		case FLAG_SKIP:
		    std::cout << std::endl << indent << "SKIP " << *(++it) << std::endl;
		    break;
		case FLAG_ARRAY:
		    std::cout << std::endl << indent << "ARRAY " << *(++it) << " " << *(++it) << std::endl;
		    indent += "  ";
		    break;
		case FLAG_END:
		    indent.resize(indent.length() - 2);
		    std::cout << std::endl << indent << "END" << std::endl;
		    break;
		default:
		    std::cout << " " << *it;
	    }
	}
    }
}

