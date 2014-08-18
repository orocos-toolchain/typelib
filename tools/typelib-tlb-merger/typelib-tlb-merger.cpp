#include "../../lang/tlb/export.hh"
#include "../../lang/tlb/import.hh"
#include "../../lang/csupport/standard_types.hh"

#include <iostream>
#include <fstream>

int main(int argc, const char **argv) {

    if(argc != 4)
    {
        std::cerr << "Usage : " << argv[0]
                  << " <Input1.tlb> <Input2.tlb> <Output.tlb>" << std::endl;
        return 0;
    }

    std::string input1(argv[1]);
    std::string input2(argv[2]);
    std::string output(argv[3]);

    Typelib::Registry reg_out;
    Typelib::CXX::addStandardTypes(reg_out);

    Typelib::Registry reg1;
    Typelib::Registry reg2;

    TlbImport importer;

    importer.load(input1, utilmm::config_set(), reg1);
    importer.load(input2, utilmm::config_set(), reg2);

    reg_out.merge(reg1);
    reg_out.merge(reg2);

    TlbExport exporter;
    exporter.save(output, utilmm::config_set(), reg_out);

    return 0;

}
