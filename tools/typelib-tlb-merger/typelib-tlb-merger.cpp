#include "../../lang/tlb/export.hh"
#include "../../lang/tlb/import.hh"

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

    Typelib::Registry reg1;
    Typelib::Registry reg2;

    TlbImport importer;

    importer.load(input1, utilmm::config_set(), reg1);
    importer.load(input2, utilmm::config_set(), reg2);

    reg1.merge(reg2);

    TlbExport exporter;
    exporter.save(output, utilmm::config_set(), reg1);

    return 0;

}
