#ifndef TYPELIB_VALUE_OPS_HH
#define TYPELIB_VALUE_OPS_HH

#include <typelib/memory_layout.hh>
#include <typelib/value.hh>
#include <iosfwd>

namespace Typelib {
void init(Value v);
void init(Value v, MemoryLayout const &ops);
void init(uint8_t *data, MemoryLayout const &ops);

void zero(Value v);
void zero(Value v, MemoryLayout const &ops);
void zero(uint8_t *data, MemoryLayout const &ops);

void destroy(Value v);
void destroy(Value v, MemoryLayout const &ops);
void destroy(uint8_t *data, MemoryLayout const &ops);

void copy(Value dst, Value src);
void copy(void *dst, void *src, Type const &type);

bool compare(Value dst, Value src);
bool compare(void *dst, void *src, Type const &type);

void display(std::ostream &io, MemoryLayout::const_iterator const begin,
             MemoryLayout::const_iterator const end);

std::vector<uint8_t> dump(Value v);

void dump(Value v, std::vector<uint8_t> &buffer);
void dump(Value v, std::vector<uint8_t> &buffer, MemoryLayout const &ops);
void dump(uint8_t const *v, std::vector<uint8_t> &buffer,
          MemoryLayout const &ops);

void dump(Value v, std::ostream &stream);
void dump(Value v, std::ostream &stream, MemoryLayout const &ops);
void dump(uint8_t const *v, std::ostream &stream, MemoryLayout const &ops);

void dump(Value v, int fd);
void dump(Value v, int fd, MemoryLayout const &ops);
void dump(uint8_t const *v, int fd, MemoryLayout const &ops);

void dump(Value v, FILE *fd);
void dump(Value v, FILE *fd, MemoryLayout const &ops);
void dump(uint8_t const *v, FILE *fd, MemoryLayout const &ops);

int dump(Value v, uint8_t *buffer, unsigned int buffer_size);
int dump(Value v, uint8_t *buffer, unsigned int buffer_size,
         MemoryLayout const &ops);
int dump(uint8_t const *v, uint8_t *buffer, unsigned int buffer_size,
         MemoryLayout const &ops);

struct OutputStream {
    virtual void write(uint8_t const *data, size_t size) = 0;
};
void dump(Value v, OutputStream &stream);
void dump(Value v, OutputStream &stream, MemoryLayout const &ops);
void dump(uint8_t const *v, OutputStream &stream, MemoryLayout const &ops);

size_t getDumpSize(Value v);
size_t getDumpSize(Value v, MemoryLayout const &ops);
size_t getDumpSize(uint8_t const *v, MemoryLayout const &ops);

struct InputStream {
    virtual void read(uint8_t *data, size_t size) = 0;
};
void load(Value v, InputStream &stream);
void load(Value v, InputStream &stream, MemoryLayout const &ops);
void load(uint8_t *v, Type const &type, InputStream &stream,
          MemoryLayout const &ops);

void load(Value v, std::vector<uint8_t> const &buffer);
void load(Value v, std::vector<uint8_t> const &buffer, MemoryLayout const &ops);
void load(uint8_t *v, Type const &type, std::vector<uint8_t> const &buffer,
          MemoryLayout const &ops);

void load(Value v, uint8_t const *buffer, unsigned int buffer_size);
void load(Value v, uint8_t const *buffer, unsigned int buffer_size,
          MemoryLayout const &ops);
void load(uint8_t *v, Type const &type, uint8_t const *buffer,
          unsigned int buffer_size, MemoryLayout const &ops);
}

#endif
