import re
import os.path
import string

def antlr_generator(target, source, env, for_signature):
    output_path = os.path.dirname(str(target[0]))
    super = env['ANTLRSUPER']

    command=""
    if super:
        command = "cantlr ${ANTLRFLAGS} -glib ${ANTLRSUPER} "
    else:
        command = "cantlr ${ANTLRFLAGS} "
        
    return command + "-o %s %s " % (output_path, string.join([str(x) for x in source]))

def antlr_emitter(target, source, env):
    target=[]
    re_def = re.compile("class\s+([^\s]+)\s+extends\s+([^\s]+)");
    re_vocab = re.compile("\s*exportVocab\s+=\s+([^\s]+)\s*;");
    
    gfile=open(source[0].srcnode().abspath)
    for line in gfile:
        matched = re_def.match(line)
        if re_def.match(line):
            groups = matched.groups()
            target.extend( [groups[0] + ".cpp", groups[0] + ".hpp"] )
        else:
            matched = re_vocab.match(line)
            if matched:
                vocab = matched.groups()[0] + "TokenTypes"
                target.extend( [vocab + ".hpp", vocab + ".txt" ] )

    return target, source

antlr_builder = Builder(generator = antlr_generator,
              src_suffix = '.g',
              emitter = antlr_emitter)

env = Environment();
env['ANTLRSUPER'] = ""
env.Append(BUILDERS = {'Antlr' : antlr_builder});


env.BuildDir('build', '.', duplicate=1)
target = env.Dir('build/target')
Export('target', 'env')

SConscript(Split('build/typesolver/SConscript build/typelib/SConscript'))
Alias('test', 'typesolver/test')

