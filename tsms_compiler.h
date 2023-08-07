#ifndef TSMS_COMPILER_H
#define TSMS_COMPILER_H

#define TSMS_EXTEND_COMPILER_TOKEN \
	TSMS_COMPILER_TOKEN_TYPE type; \
	pString value;

#define TSMS_EXTEND_COMPILER_SPLIT_TOKEN \
	TSMS_EXTEND_COMPILER_TOKEN \
	TSMS_LP children; \
	pCompilerSplitToken parent;

#define TSMS_EXTEND_COMPILER_SENTENCE \
	TSMS_LP rvalue;                      \
	TSMS_COMPILER_SENTENCE_TYPE type;

typedef struct TSMS_COMPILER tCompiler;
typedef tCompiler* pCompiler;

typedef struct TSMS_COMPILER_PRE_PROGRAM tCompilerPreProgram;
typedef tCompilerPreProgram* pCompilerPreProgram;

typedef struct TSMS_COMPILER_TOKEN tCompilerToken;
typedef tCompilerToken* pCompilerToken;

typedef struct TSMS_COMPILER_SPLIT_TOKEN tCompilerSplitToken;
typedef tCompilerSplitToken* pCompilerSplitToken;

typedef struct TSMS_COMPILER_BLOCK_TOKEN tCompilerBlockToken;
typedef tCompilerBlockToken* pCompilerBlockToken;

typedef struct TSMS_COMPILER_DEFINE_TOKEN tCompilerDefineToken;
typedef tCompilerDefineToken* pCompilerDefineToken;

typedef struct TSMS_COMPILER_PROGRAM tCompilerProgram;
typedef tCompilerProgram* pCompilerProgram;

typedef struct TSMS_COMPILER_TOKEN_DEFINITION tCompilerTokenDefinition;

typedef struct TSMS_COMPILER_SENTENCE tCompilerSentence;
typedef tCompilerSentence* pCompilerSentence;

typedef struct TSMS_COMPILER_ASSIGNMENT_SENTENCE tCompilerAssignmentSentence;
typedef tCompilerAssignmentSentence* pCompilerAssignmentSentence;

typedef struct TSMS_COMPILER_BLOCK_SENTENCE tCompilerBlockSentence;
typedef tCompilerBlockSentence* pCompilerBlockSentence;

typedef struct TSMS_COMPILER_DEFINE_SENTENCE tCompilerDefineSentence;
typedef tCompilerDefineSentence* pCompilerDefineSentence;

typedef enum {
	TSMS_COMPILER_TOKEN_TYPE_BLOCK = 1,
	TSMS_COMPILER_TOKEN_TYPE_CHAR = 2,
	TSMS_COMPILER_TOKEN_TYPE_STRING = 4,
	TSMS_COMPILER_TOKEN_TYPE_NUMBER = 8,
	TSMS_COMPILER_TOKEN_TYPE_KEYWORD = 16,
	TSMS_COMPILER_TOKEN_TYPE_UNDEFINE = 32,
	TSMS_COMPILER_TOKEN_TYPE_MERGED_KEYWORD = 64,
	TSMS_COMPILER_TOKEN_TYPE_SPLIT = 128,
	TSMS_COMPILER_TOKEN_TYPE_DEFINE = 256,
	TSMS_COMPILER_TOKEN_TYPE_LINE_COMMENT = 512,
	TSMS_COMPILER_TOKEN_TYPE_BLOCK_COMMENT = 1024
} TSMS_COMPILER_TOKEN_TYPE;

typedef enum {
	TSMS_COMPILER_SENTENCE_TYPE_ASSIGNMENT,
	TSMS_COMPILER_SENTENCE_TYPE_BLOCK,
	TSMS_COMPILER_SENTENCE_TYPE_RVALUE,
	TSMS_COMPILER_SENTENCE_TYPE_DEFINE
} TSMS_COMPILER_SENTENCE_TYPE;

#include "tsms.h"
#include "tsms_int_list.h"

typedef bool (*TSMS_COMPILER_DEFINE_VALIDATOR)(pCompilerSplitToken token, TSMS_LP tokens, TSMS_POS index, tCompilerTokenDefinition * definition, TSMS_SIZE size);

struct TSMS_COMPILER {
	bool ignoreComment;
};

struct TSMS_COMPILER_PRE_PROGRAM {
	pCompilerToken token;
};

struct TSMS_COMPILER_TOKEN {
	TSMS_EXTEND_COMPILER_TOKEN
};

struct TSMS_COMPILER_SPLIT_TOKEN {
	TSMS_EXTEND_COMPILER_SPLIT_TOKEN
};

struct TSMS_COMPILER_BLOCK_TOKEN {
	TSMS_EXTEND_COMPILER_SPLIT_TOKEN
};

struct TSMS_COMPILER_DEFINE_TOKEN {
	TSMS_EXTEND_COMPILER_SPLIT_TOKEN
	TSMS_ILP blocks;
	bool seperate;
};

struct TSMS_COMPILER_PROGRAM {
	pCompilerBlockSentence sentence;
	pCompilerBlockToken token;
};

struct TSMS_COMPILER_TOKEN_DEFINITION {
	TSMS_COMPILER_TOKEN_TYPE type;
	pString value;
	bool isBlock;
};

struct TSMS_COMPILER_SENTENCE {
	TSMS_EXTEND_COMPILER_SENTENCE
};

struct TSMS_COMPILER_ASSIGNMENT_SENTENCE {
	TSMS_EXTEND_COMPILER_SENTENCE
	TSMS_LP lvalue;
};

struct TSMS_COMPILER_BLOCK_SENTENCE {
	TSMS_EXTEND_COMPILER_SENTENCE
};

struct TSMS_COMPILER_DEFINE_SENTENCE {
	TSMS_EXTEND_COMPILER_SENTENCE
	TSMS_ILP blocks;
	pString name;
};

pCompiler TSMS_COMPILER_create();

pCompilerPreProgram TSMS_COMPILER_compile(pCompiler compiler, pString source);

TSMS_RESULT TSMS_COMPILER_release(pCompiler compiler);

TSMS_RESULT TSMS_COMPILER_PRE_PROGRAM_release(pCompilerPreProgram program);

TSMS_RESULT TSMS_COMPILER_TOKEN_release(pCompilerToken token);

TSMS_RESULT TSMS_COMPILER_TOKEN_releaseByType(pCompilerToken token);

TSMS_RESULT TSMS_COMPILER_SPLIT_TOKEN_release(pCompilerSplitToken token);

TSMS_RESULT TSMS_COMPILER_BLOCK_TOKEN_release(pCompilerBlockToken token);

TSMS_RESULT TSMS_COMPILER_DEFINE_TOKEN_release(pCompilerDefineToken token);

TSMS_RESULT TSMS_COMPILER_PROGRAM_remove(pCompilerPreProgram program, TSMS_COMPILER_TOKEN_TYPE type, pString value);

TSMS_RESULT TSMS_COMPILER_PROGRAM_split(pCompilerPreProgram program, TSMS_COMPILER_TOKEN_TYPE type, pString value, TSMS_COMPILER_TOKEN_TYPE splitType, pString splitValue);

TSMS_RESULT TSMS_COMPILER_PROGRAM_define(pCompilerPreProgram program, pString value, tCompilerTokenDefinition * definition, TSMS_SIZE size);

TSMS_RESULT TSMS_COMPILER_PROGRAM_defineWithCondition(pCompilerPreProgram program, pString value, tCompilerTokenDefinition * definition, TSMS_SIZE size, TSMS_COMPILER_DEFINE_VALIDATOR validator, bool seperate);

TSMS_RESULT TSMS_COMPILER_PROGRAM_mergeKeyword(pCompilerPreProgram program, pString * keywords, TSMS_SIZE size);

pCompilerProgram TSMS_COMPILER_PROGRAM_compile(pCompilerPreProgram program);

TSMS_RESULT TSMS_COMPILER_PROGRAM_print(pCompilerPreProgram program);

TSMS_RESULT TSMS_COMPILER_PROGRAM_release(pCompilerProgram program);

#endif //TSMS_COMPILER_H