#ifndef TSMS_COMPILER_H
#define TSMS_COMPILER_H

typedef struct TSMS_COMPILER tCompiler;
typedef tCompiler* pCompiler;

typedef struct TSMS_COMPILER_PRE_PROGRAM tCompilerPreProgram;
typedef tCompilerPreProgram* pCompilerPreProgram;

typedef struct TSMS_COMPILER_TOKEN tCompilerToken;
typedef tCompilerToken* pCompilerToken;

typedef struct TSMS_COMPILER_PROGRAM tCompilerProgram;
typedef tCompilerProgram* pCompilerProgram;

typedef enum {
	TSMS_COMPILER_TOKEN_TYPE_BLOCK,
	TSMS_COMPILER_TOKEN_TYPE_CHAR,
	TSMS_COMPILER_TOKEN_TYPE_STRING,
	TSMS_COMPILER_TOKEN_TYPE_NUMBER,
	TSMS_COMPILER_TOKEN_TYPE_KEYWORD,
	TSMS_COMPILER_TOKEN_TYPE_UNDEFINED,
	TSMS_COMPILER_TOKEN_TYPE_SPLITED
} TSMS_COMPILER_TOKEN_TYPE;

#include "tsms.h"

struct TSMS_COMPILER {
	bool acceptLittleQuote;
};

struct TSMS_COMPILER_PRE_PROGRAM {
	pCompilerToken token;
};

struct TSMS_COMPILER_TOKEN {
	TSMS_COMPILER_TOKEN_TYPE type;
	pString value;
	TSMS_LP children;
	pCompilerToken parent;
};

struct TSMS_COMPILER_PROGRAM {
};

pCompiler TSMS_COMPILER_create();

pCompilerPreProgram TSMS_COMPILER_compile(pCompiler compiler, pString source);

TSMS_RESULT TSMS_COMPILER_release(pCompiler compiler);

TSMS_RESULT TSMS_COMPILER_PROGRAM_release(pCompilerPreProgram program);

TSMS_RESULT TSMS_COMPILER_TOKEN_release(pCompilerToken token);

TSMS_RESULT TSMS_COMPILER_PROGRAM_remove(pCompilerPreProgram program, TSMS_COMPILER_TOKEN_TYPE type, pString value);

TSMS_RESULT TSMS_COMPILER_PROGRAM_split(pCompilerPreProgram program, TSMS_COMPILER_TOKEN_TYPE type, TSMS_COMPILER_TOKEN_TYPE splitType, pString splitValue);

#endif //TSMS_COMPILER_H
