header
{
    #include <utility>
    #include <list>
    #include <map>
    #include "typesolver.h"
    #include <antlr/TokenStreamSelector.hpp>

    #include "genomreader.h"

    #include <iostream>

    class Registry;
}

options
{
    language = "Cpp";
}

class GenomParser extends Parser;

options
{
    k = 2;
    buildAST = false;
    importVocab = STDC;
    exportVocab = Genom;
}

{
protected:
    Registry* m_registry;
    antlr::TokenStreamSelector* m_selector;

public:
    typedef Genom::StringList StringList;
    typedef Genom::StringMap  StringMap;

    typedef Genom::SDIRef     SDIRef;
    typedef Genom::SDIRefList SDIRefList;

    typedef Genom::Module     GenomModule;
    typedef Genom::Request     GenomRequest;
    typedef Genom::Poster     GenomPoster;
    typedef Genom::ExecTask     GenomExecTask;

    virtual void genomModule(const GenomModule& module) {}
    virtual void genomRequest(const GenomRequest& request) {}
    virtual void genomPoster(const GenomPoster& poster) {}
    virtual void genomExecTask(const GenomExecTask& task) {}

    void setSelector(antlr::TokenStreamSelector* selector) { m_selector = selector; }
    void setRegistry(Registry* registry) { m_registry = registry; }
}

translation_unit
:
    { std::cout << "Beginning C++ parsing" << std::endl; }
    {
        m_selector -> push("cpp");
        TypeSolver solver(getInputState(), m_registry);
        solver.init();
        solver.fragment();
        m_selector -> pop();
    }
    { std::cout << "End of C++ parsing, beginning GenoM section" << std::endl; }
    (genom_definition)*
    EOF
    { std::cout << "End of parsing" << std::endl; }
;

genom_definition
:
    module_definition (SEMICOLON)?
    (
        (request_definition
        | poster_definition 
        | exec_task_definition)
        (SEMICOLON)?
    )+
;

module_definition
{ 
    GenomModule module; 
    int module_id;
}
:
        "module" name:ID LCURLY { module.name = name->getText(); }
            ( "number" COLON module_id=int_constant SEMICOLON { module.id = module_id; }
              "internal_data" COLON data:ID SEMICOLON { module.data = data->getText(); } )+
        RCURLY
        { genomModule(module); }
;

request_definition
{ 
    GenomRequest request; 
    SDIRef strpair;
    StringList strlist;
    std::string ftype;
}
: // manque input_info
    "request" name:ID LCURLY { request.name = name->getText(); }
        ( "doc"  COLON doc:StringLiteral SEMICOLON { request.doc = doc->getText(); }
        | "type" COLON 
            ( "control" | "exec" ) { request.type = LT(1)->getText(); }
            SEMICOLON
        | "exec_task" COLON task:ID SEMICOLON { request.task = task->getText(); }

        | "input" COLON  strpair=sdi_ref SEMICOLON { request.input = strpair; }
        | "output" COLON strpair=sdi_ref SEMICOLON { request.output = strpair; }

        | "posters_input" COLON strlist=id_list SEMICOLON { request.posters = strlist; }

        | ftype=request_function COLON fname:ID SEMICOLON
            { request.functions.insert( make_pair(ftype, fname->getText()) ); }
        | "fail_msg" COLON strlist=id_list SEMICOLON { request.fail_msg = strlist; }
        | "incompatible_with" COLON strlist=id_list SEMICOLON { request.incompat = strlist; }
        )+
    RCURLY
    { genomRequest(request); }
;

poster_definition
{
    GenomPoster poster;
    SDIRefList sdilist;
    StringList typelist;
}
:
    "poster" name:ID { poster.name = name->getText(); }
    LCURLY 
        ( "update" COLON 
            ( "user" { poster.update = GenomPoster::User; } 
            | "auto" { poster.update = GenomPoster::Auto; } ) SEMICOLON

        | "data"         COLON sdilist = sdiref_list SEMICOLON { poster.data = sdilist; }
        | "exec_task"    COLON exec_task:ID          SEMICOLON { poster.exec_task = exec_task->getText(); } 
        | "type"         COLON typelist = id_list    SEMICOLON { poster.typelist = typelist; }
        | "c_create_fun" COLON create_fun:ID         SEMICOLON { poster.create_func = create_fun->getText(); }
        )+
    RCURLY
    { genomPoster(poster); }
;

exec_task_definition
{
    GenomExecTask task;
    int num;
    StringList module_list, fail_list;
    SDIRefList poster_list;
}
:
    "exec_task" name:ID { task.name = name->getText(); }
    LCURLY
        ( "period"      COLON num=int_constant SEMICOLON { task.period = num; }
        | "delay"       COLON num=int_constant SEMICOLON { task.delay = num; }
        | "priority"    COLON num=int_constant SEMICOLON { task.priority = num; }
        | "stack_size"  COLON num=int_constant SEMICOLON { task.stack_size = num; }

        | "c_init_func" COLON init_func:ID  SEMICOLON { task.c_init_func = init_func->getText(); }
        | "c_func"      COLON func:ID       SEMICOLON { task.c_func = func->getText(); }
        | "c_end_func"  COLON end_func:ID   SEMICOLON { task.c_end_func = end_func->getText(); }

        | "cs_client_from"      COLON module_list=id_list     SEMICOLON { task.cs_client_from = module_list; }
        | "poster_client_from"  COLON poster_list=sdiref_list SEMICOLON { task.poster_client_from = poster_list; }

        | "fail_msg"            COLON fail_list=id_list SEMICOLON { task.fail_msg = fail_list; }
        )+
    RCURLY
    { genomExecTask(task); }
;


sdi_ref returns [GenomParser::SDIRef ref]
: 
    name:ID COLON COLON sdiname:ID 
    { ref=make_pair(name->getText(), sdiname->getText()); } 
;

request_function returns [std::string function_type]
: 
(   "c_control_fun" | "c_exec_func_start" 
|   "c_exec_func" | "c_exec_func_end" | "c_exec_func_inter" | "c_exec_func_fail" 
)   { function_type=LT(1)->getText(); }
;

id_list returns [GenomParser::StringList list]
:
    id:ID { list.push_back(id->getText()); }
    ( COMMA other_id:ID { list.push_back(id->getText()); } )*
;

sdiref_list returns [GenomParser::SDIRefList list]
{ SDIRef pair; }
:
    pair=sdi_ref { list.push_back(pair); }
    ( COMMA pair=sdi_ref { list.push_back(pair); } )*
;
    

int_constant returns [ int value ]
:       
    dec:DecimalNumber { value = atoi(dec->getText().data()); }
;


class GenomLexer extends Lexer;

options
{
    k = 2;
    importVocab = Genom;
    testLiterals = false;
}

COMMA: ',';
COLON: ':';
LCURLY: '{';
RCURLY: '}';
SEMICOLON: ';';

ID
options { testLiterals=true; }
:
    ( 'a'..'z' | 'A'..'Z' | '_' )
    ( 'a'..'z' | 'A'..'Z' | '_' | '0'..'9' )*
;

DecimalNumber
:
    ( '0' .. '9' )+
;

StringLiteral
:	
    '"'
		(~('"' | '\r' | '\n' | '\\'))*
		'"'
;

Whitespace
:	
    (	(' ' |'\t' | '\f')
			// handle newlines
		|	(	"\r\n"  // MS
			|	'\r'    // Mac
			|	'\n'    // Unix 
			)	
    )
		{_ttype = ANTLR_USE_NAMESPACE(antlr)Token::SKIP;}
;
	
protected
Vocabulary
	:	'\3'..'\377'
	;

