#ifndef TSMS_COMPILER_H
#define TSMS_COMPILER_H

#define TSMS_EXTEND_COMPILER_TOKEN \
	TSMS_COMPILER_TOKEN_TYPE type; \
	pString value;

#define TSMS_EXTEND_COMPILER_SPLITED_TOKEN \
	TSMS_EXTEND_COMPILER_TOKEN \
	TSMS_LP children;

typedef struct TSMS_COMPILER tCompiler;
typedef tCompiler* pCompiler;

typedef struct TSMS_COMPILER_PRE_PROGRAM tCompilerPreProgram;
typedef tCompilerPreProgram* pCompilerPreProgram;

typedef struct TSMS_COMPILER_TOKEN tCompilerToken;
typedef tCompilerToken* pCompilerToken;

typedef struct TSMS_COMPILER_SPLITED_TOKEN tCompilerSplitedToken;
typedef tCompilerSplitedToken* pCompilerSplitedToken;

typedef struct TSMS_COMPILER_BLOCK_TOKEN tCompilerBlockToken;
typedef tCompilerBlockToken* pCompilerBlockToken;

typedef struct TSMS_COMPILER_DEFINE_TOKEN tCompilerDefineToken;
typedef tCompilerDefineToken* pCompilerDefineToken;

typedef struct TSMS_COMPILER_PROGRAM tCompilerProgram;
typedef tCompilerProgram* pCompilerProgram;

typedef struct TSMS_COMPILER_TOKEN_DEFINITION tCompilerTokenDefinition;

typedef struct TSMS_COMPILER_TOKEN_SENTENCE tCompilerTokenSentence;
typedef tCompilerTokenSentence* pCompilerTokenSentence;

typedef enum {
	TSMS_COMPILER_TOKEN_TYPE_BLOCK,
	TSMS_COMPILER_TOKEN_TYPE_CHAR,
	TSMS_COMPILER_TOKEN_TYPE_STRING,
	TSMS_COMPILER_TOKEN_TYPE_NUMBER,
	TSMS_COMPILER_TOKEN_TYPE_KEYWORD,
	TSMS_COMPILER_TOKEN_TYPE_UNDEFINED,
	TSMS_COMPILER_TOKEN_TYPE_MERGED_KEYWORD,
	TSMS_COMPILER_TOKEN_TYPE_SPLITED,
	TSMS_COMPILER_TOKEN_TYPE_DEFINE
} TSMS_COMPILER_TOKEN_TYPE;

#include "tsms.h"
#include "tsms_int_list.h"

struct TSMS_COMPILER {
	bool acceptLittleQuote;
};

struct TSMS_COMPILER_PRE_PROGRAM {
	pCompilerToken token;
};

struct TSMS_COMPILER_TOKEN {
	TSMS_EXTEND_COMPILER_TOKEN
};

struct TSMS_COMPILER_SPLITED_TOKEN {
	TSMS_EXTEND_COMPILER_SPLITED_TOKEN
};

struct TSMS_COMPILER_BLOCK_TOKEN {
	TSMS_EXTEND_COMPILER_SPLITED_TOKEN
	pCompilerBlockToken parent;
};

struct TSMS_COMPILER_DEFINE_TOKEN {
	TSMS_EXTEND_COMPILER_SPLITED_TOKEN
	TSMS_ILP blocks;
};

struct TSMS_COMPILER_PROGRAM {
	pCompilerTokenSentence sentence;
};

struct TSMS_COMPILER_TOKEN_DEFINITION {
	TSMS_COMPILER_TOKEN_TYPE type;
	pString value;
	bool isBlock;
};

struct TSMS_COMPILER_TOKEN_SENTENCE {
	TSMS_LP lvalue;
	TSMS_LP rvalue;
	TSMS_LP children;
};

pCompiler TSMS_COMPILER_create();

pCompilerPreProgram TSMS_COMPILER_compile(pCompiler compiler, pString source);

TSMS_RESULT TSMS_COMPILER_release(pCompiler compiler);

TSMS_RESULT TSMS_COMPILER_PROGRAM_release(pCompilerPreProgram program);

TSMS_RESULT TSMS_COMPILER_TOKEN_release(pCompilerToken token);

TSMS_RESULT TSMS_COMPILER_TOKEN_releaseByType(pCompilerToken token);

TSMS_RESULT TSMS_COMPILER_SPLITED_TOKEN_release(pCompilerSplitedToken token);

TSMS_RESULT TSMS_COMPILER_DEFINE_TOKEN_release(pCompilerDefineToken token);

TSMS_RESULT TSMS_COMPILER_PROGRAM_remove(pCompilerPreProgram program, TSMS_COMPILER_TOKEN_TYPE type, pString value);

TSMS_RESULT TSMS_COMPILER_PROGRAM_split(pCompilerPreProgram program, TSMS_COMPILER_TOKEN_TYPE type, pString value, TSMS_COMPILER_TOKEN_TYPE splitType, pString splitValue);

TSMS_RESULT TSMS_COMPILER_PROGRAM_define(pCompilerPreProgram program, pString value, tCompilerTokenDefinition * definition, TSMS_SIZE size);

TSMS_RESULT TSMS_COMPILER_PROGRAM_mergeKeyword(pCompilerPreProgram program, pString * keywords, TSMS_SIZE size);

pCompilerProgram TSMS_COMPILER_PROGRAM_compile(pCompilerPreProgram program);

TSMS_RESULT TSMS_COMPILER_PROGRAM_print(pCompilerPreProgram program);

#endif //TSMS_COMPILER_H
