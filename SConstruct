
BuildDir('build', '.', duplicate=0)
target = Dir('build/target')
Export('target')

SConscriptChdir(1)
SConscript(Split('build/grammar/SConscript build/typesolver/SConscript'))

