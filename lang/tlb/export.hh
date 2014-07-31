#ifndef TYPELIB_EXPORT_TLB_HH
#define TYPELIB_EXPORT_TLB_HH

#include <typelib/exporter.hh>

class TlbExport : public Typelib::Exporter
{
protected:
    /** Called by save to add a prelude before saving all registry types */
    virtual void begin(std::ostream& stream, Typelib::Registry const& registry);
    /** Called by save to add data after saving all registry types */
    virtual void end  (std::ostream& stream, Typelib::Registry const& registry);

public:
    virtual bool save
        ( std::ostream& stream
        , Typelib::RegistryIterator const& type);
        
    virtual void save(std::ostream& stream, const utilmm::config_set& config, const Typelib::Registry& registry);
    virtual bool save(std::ostream& stream, const Typelib::Registry& registry);
    virtual void save(const std::string& file_name, const utilmm::config_set& config, const Typelib::Registry& registry);

};

#endif

