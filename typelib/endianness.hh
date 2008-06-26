#ifndef TYPELIB_ENDIANSWAP_HH
#define TYPELIB_ENDIANSWAP_HH

#include <typelib/value.hh>
#include <utilmm/system/endian.hh>
#include <limits>

namespace Typelib
{
    struct UnsupportedEndianSwap : public std::runtime_error
    { 
        UnsupportedEndianSwap(std::string const& what) : std::runtime_error("cannot swap " + what) { }
    };

    /* This visitor swaps the endianness of the given value in-place */
    class EndianSwapVisitor : public ValueVisitor
    {
    protected:
        bool visit_ (int8_t  & value) { value = utilmm::endian::swap(value); return true; }
        bool visit_ (uint8_t & value) { value = utilmm::endian::swap(value); return true; }
        bool visit_ (int16_t & value) { value = utilmm::endian::swap(value); return true; }
        bool visit_ (uint16_t& value) { value = utilmm::endian::swap(value); return true; }
        bool visit_ (int32_t & value) { value = utilmm::endian::swap(value); return true; }
        bool visit_ (uint32_t& value) { value = utilmm::endian::swap(value); return true; }
        bool visit_ (int64_t & value) { value = utilmm::endian::swap(value); return true; }
        bool visit_ (uint64_t& value) { value = utilmm::endian::swap(value); return true; }
        bool visit_ (float   & value) { value = utilmm::endian::swap(value); return true; }
        bool visit_ (double  & value) { value = utilmm::endian::swap(value); return true; }
        bool visit_ (Value const& v, Pointer const& t) { throw UnsupportedEndianSwap("pointers"); }
        bool visit_ (Enum::integral_type& v, Enum const& e) { v = utilmm::endian::swap(v); return true; }
    };

    /** Swaps the endianness of +v+ by using a EndianSwapVisitor */
    void endian_swap(Value v)
    {
	EndianSwapVisitor swapper;
	swapper.apply(v);
    }

    class CompileEndianSwapVisitor : public TypeVisitor 
    {
	// The current place into the output: the next element which are to be
	// byte-swapped will be written at this index in the output data
	// buffer.
	size_t m_output_index;

    public:
	// The compiled-in description of the byte-swap operation
	//
	// Given an output index, an element of compiled is the absolute
	// index of the input byte which should be written at this output index:
	//   output_buffer[output_index] = input_buffer[*it_compiled]
	//
	// After a "normal" operation (i.e. neither a skip nor an array),
	// output index is incremented and we handle the next operation
	// found in compiled
	std::vector<size_t> m_compiled;

	static size_t const FLAG_SKIP  = ((size_t) -1);
	static size_t const FLAG_ARRAY = ((size_t) -2);
	static size_t const FLAG_END   = ((size_t) -3);
	static size_t const FLAG_SWAP_4 = ((size_t) -4);
	static size_t const FLAG_SWAP_8 = ((size_t) -5);
	static const size_t SizeOfEnum = sizeof(int);;

    protected:
	void skip(int skip_size);
        bool visit_ (Numeric const& type);
        bool visit_ (Enum const& type);
        bool visit_ (Pointer const& type);
        bool visit_ (Array const& type);
	bool visit_ (Compound const& type);

    public:
	~CompileEndianSwapVisitor() { }
	void apply(Type const& type);

	void swap(Value in, Value out)
	{
	    CompileEndianSwapVisitor::swap(0, 0,
		    m_compiled.begin(), m_compiled.end(),
		    in, out);
	}

	std::pair<size_t, std::vector<size_t>::const_iterator> 
	    swap(size_t output_offset, size_t input_offset,
		std::vector<size_t>::const_iterator it,
		std::vector<size_t>::const_iterator end,
		Value in, Value out);

	void display();
    };
}

#endif

