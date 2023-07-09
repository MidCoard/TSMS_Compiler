#include <ctype.h>
#include "tsms_compiler.h"
#include "tsms_util.h"
#include "tsms_int_stack.h"

typedef enum {
	_TOKEN_TYPE_KEYWORD, _TOKEN_TYPE_QUOTE, _TOKEN_TYPE_LITTLE_QUOTE, _TOKEN_TYPE_UNDEFINED
} _tokenType;

struct _token {
	_tokenType type;
	pString value;
};

static char _compilerKeyword[] = {',', '.', ';', ':',
								  '(', ')', '[', ']', '{', '}', '+', '-', '*', '/', '%', '&', '|', '^', '~', '!', '=', '<', '>', '?', '\n', '\r', '\t', 0};

static bool __tsms_internal_is_keyword(char c) {
	return strchr(_compilerKeyword, c) != TSMS_NULL;
}

static struct _token * __tsms_internal_create_internal_token(_tokenType type, pString value) {
	struct _token * token = TSMS_malloc(sizeof(struct _token));
	token->type = type;
	token->value = value;
	return token;
}

static pCompilerToken __tsms_internal_create_token(TSMS_COMPILER_TOKEN_TYPE type, pString value) {
	pCompilerToken token = (pCompilerToken)TSMS_malloc(sizeof(tCompilerToken));
	token->type = type;
	token->value = value;
	token->children = TSMS_NULL;
	token->parent = TSMS_NULL;
	return token;
}

static pCompilerToken __tsms_internal_create_block_token(pCompilerToken parent, pString value) {
	pCompilerToken token = (pCompilerToken)TSMS_malloc(sizeof(tCompilerToken));
	token->type = TSMS_COMPILER_TOKEN_TYPE_BLOCK;
	token->value = value;
	token->children = TSMS_LIST_create(10);
	token->parent = parent;
	return token;
}

static pCompilerToken __tsms_internal_create_split_token(TSMS_LP tokens) {
	pCompilerToken token = (pCompilerToken)TSMS_malloc(sizeof(tCompilerToken));
	token->type = TSMS_COMPILER_TOKEN_TYPE_SPLITED;
	token->value = TSMS_NULL;
	token->children = tokens;
	token->parent = TSMS_NULL;
	return token;
}

static bool __tsms_internal_is_number(pString str) {
	for (TSMS_POS i = 0; i < str->length; i++)
		if (!isdigit(str->cStr[i]))
			return false;
	return true;
}

static bool __tsms_internal_is_left_bracket(char c) {
	return c == '(' || c == '[' || c == '{';
}

static bool __tsms_internal_is_right_bracket(char c) {
	return c == ')' || c == ']' || c == '}';
}

static bool __tsms_internal_is_left_right_match(char left, char right) {
	switch (left) {
		case '(':
			return right == ')';
		case '[':
			return right == ']';
		case '{':
			return right == '}';
		default:
			return false;
	}
}

static void __tsms_internal_remove_token(pCompilerToken token, TSMS_COMPILER_TOKEN_TYPE type, pString value) {
	if (token->type == TSMS_COMPILER_TOKEN_TYPE_BLOCK)
		for (TSMS_POS i = 0; i < token->children->length; i++) {
			pCompilerToken child = token->children->list[i];
			if (child->type == type && TSMS_STRING_equals(child->value, value)) {
				TSMS_COMPILER_TOKEN_release(child);
				TSMS_LIST_remove(token->children, i);
				i--;
			} else
				__tsms_internal_remove_token(child, type, value);
		}
}

static void __tsms_internal_split_token(pCompilerToken token, TSMS_COMPILER_TOKEN_TYPE type, TSMS_COMPILER_TOKEN_TYPE splitType, pString splitValue) {
	if (token->type == TSMS_COMPILER_TOKEN_TYPE_BLOCK || token->type == TSMS_COMPILER_TOKEN_TYPE_SPLITED) {
		if (token->type == type) {
			TSMS_LP tokens = TSMS_LIST_create(10);
			TSMS_LP current = TSMS_LIST_create(10);
			for (TSMS_POS i = 0; i < token->children->length; i++) {
				pCompilerToken subChild = token->children->list[i];
				if (subChild->type == splitType && TSMS_STRING_equals(subChild->value, splitValue)) {
					if (current->length > 0) {
						pCompilerToken newToken = __tsms_internal_create_split_token(current);
						current = TSMS_LIST_create(10);
						TSMS_LIST_add(tokens, newToken);
					}
				} else {
					TSMS_LIST_add(current, subChild);
				}
			}
			if (current->length > 0) {
				pCompilerToken newToken = __tsms_internal_create_split_token(current);
				TSMS_LIST_add(tokens, newToken);
			} else TSMS_LIST_release(current);
			TSMS_LIST_release(token->children);
			token->children = tokens;
		}

		for (TSMS_POS i = 0; i < token->children->length; i++) {
			pCompilerToken child = token->children->list[i];
			__tsms_internal_split_token(child, type, splitType, splitValue);
			if (child->type == type) {
				if (type != TSMS_COMPILER_TOKEN_TYPE_BLOCK && type != TSMS_COMPILER_TOKEN_TYPE_SPLITED) {
					if (splitType == TSMS_COMPILER_TOKEN_TYPE_STRING) {
						TSMS_LIST_remove(token->children, i);
						TSMS_LP list = TSMS_STRING_splitByString(child->value, splitValue);
						TSMS_SIZE counter = 0;
						for (TSMS_POS j = 0; j < list->length; j++) {
							pString str = list->list[j];
							pString trim = TSMS_STRING_trim(str);
							if (trim->length > 0) {
								pCompilerToken newToken = __tsms_internal_create_token(type, trim);
								TSMS_LIST_insert(token->children, i + counter, newToken);
								counter++;
							} else TSMS_STRING_release(trim);
							TSMS_STRING_release(str);
						}
						i += counter - 1;
						TSMS_LIST_release(list);
						TSMS_COMPILER_TOKEN_release(child);
					} else if (splitType == TSMS_COMPILER_TOKEN_TYPE_CHAR) {
						TSMS_LIST_remove(token->children, i);
						char c = splitValue->cStr[0];
						TSMS_LP list = TSMS_STRING_split(child->value, c);
						TSMS_SIZE counter = 0;
						for (TSMS_POS j = 0; j < list->length; j++) {
							pString str = list->list[j];
							pString trim = TSMS_STRING_trim(str);
							if (trim->length > 0) {
								pCompilerToken newToken = __tsms_internal_create_token(type, trim);
								TSMS_LIST_insert(token->children, i + counter, newToken);
								counter++;
							} else TSMS_STRING_release(trim);
							TSMS_STRING_release(str);
						}
						i += counter - 1;
						TSMS_LIST_release(list);
						TSMS_COMPILER_TOKEN_release(child);
					}
				}
			}
		}
	}
}

pCompiler TSMS_COMPILER_create() {
	pCompiler compiler = (pCompiler)TSMS_malloc(sizeof(tCompiler));
	compiler->acceptLittleQuote = false;
	return compiler;
}

pCompilerPreProgram TSMS_COMPILER_compile(pCompiler compiler, pString source) {
	pCompilerPreProgram program = (pCompilerPreProgram)TSMS_malloc(sizeof(tCompilerPreProgram));
	pString trim = TSMS_STRING_trim(source);
	TSMS_LP tokens = TSMS_LIST_create(10);

	bool inQuote = false;
	bool inLittleQuote = false;
	bool inEscape = false;
	pString temp = TSMS_STRING_create();
	for (TSMS_POS i = 0; i < trim->length; i++) {
		char c = trim->cStr[i];
		if (inEscape) {
			switch (c) {
				case 'n':
					TSMS_STRING_appendChar(temp, '\n');
					break;
				case 'r':
					TSMS_STRING_appendChar(temp, '\r');
					break;
				case 't':
					TSMS_STRING_appendChar(temp, '\t');
					break;
				case 'b':
					TSMS_STRING_appendChar(temp, '\b');
					break;
				case 'f':
					TSMS_STRING_appendChar(temp, '\f');
					break;
				case 'v':
					TSMS_STRING_appendChar(temp, '\v');
					break;
				case '0':
					TSMS_STRING_appendChar(temp, '\0');
					break;
				case 'a':
					TSMS_STRING_appendChar(temp, '\a');
					break;
				case '\'':
				case '"':
				case '\\':
				default:
					TSMS_STRING_appendChar(temp, c);
					break;
			}
			inEscape = false;
		} else if (c == '\\') {
			inEscape = true;
		} else if (c == '"' && !inLittleQuote) {
			inQuote = !inQuote;
			if (!inQuote) {
				TSMS_LIST_add(tokens, __tsms_internal_create_internal_token(_TOKEN_TYPE_QUOTE, TSMS_STRING_clone(temp)));
				TSMS_STRING_clear(temp);
			}
		} else if (c == '\'' && !inQuote) {
			inLittleQuote = !inLittleQuote;
			if (!inLittleQuote) {
				TSMS_LIST_add(tokens,
				              __tsms_internal_create_internal_token(_TOKEN_TYPE_LITTLE_QUOTE, TSMS_STRING_clone(temp)));
				TSMS_STRING_clear(temp);
			}
		} else if (__tsms_internal_is_keyword(c)) {
			if (temp->length > 0) {
				TSMS_LIST_add(tokens,
				              __tsms_internal_create_internal_token(_TOKEN_TYPE_UNDEFINED, TSMS_STRING_trim(temp)));
				TSMS_STRING_clear(temp);
			}
			TSMS_LIST_add(tokens,
			              __tsms_internal_create_internal_token(_TOKEN_TYPE_KEYWORD, TSMS_STRING_createWithChar(c)));
		} else {
			TSMS_STRING_appendChar(temp, c);
		}
	}

	if (inEscape || inQuote || inLittleQuote) {
		TSMS_STRING_release(temp);
		TSMS_STRING_release(trim);
		for (TSMS_POS i = 0; i < tokens->length; i++) {
			struct _token * token = tokens->list[i];
			TSMS_STRING_release(token->value);
			free(token);
		}
		TSMS_LIST_release(tokens);
		TSMS_COMPILER_PROGRAM_release(program);
		return TSMS_NULL;
	}

	if (temp->length > 0) {
		TSMS_LIST_add(tokens, __tsms_internal_create_internal_token(_TOKEN_TYPE_UNDEFINED, TSMS_STRING_clone(temp)));
		TSMS_STRING_clear(temp);
	}

	pCompilerToken currentToken = __tsms_internal_create_block_token(TSMS_NULL, TSMS_NULL);
	program->token = currentToken;

	TSMS_ISTP stack = TSMS_INT_STACK_create();

	for (TSMS_POS i = 0; i < tokens->length; i++) {
		struct _token * token = tokens->list[i];

		_tokenType type = token->type;
		pString value = token->value;
		if (type == _TOKEN_TYPE_QUOTE) {
			TSMS_LIST_add(currentToken->children, __tsms_internal_create_token(TSMS_COMPILER_TOKEN_TYPE_STRING,
			                                                            TSMS_STRING_clone(value)));
		} else if (type == _TOKEN_TYPE_LITTLE_QUOTE) {
			if (value->length == 1)
				TSMS_LIST_add(currentToken->children, __tsms_internal_create_token(TSMS_COMPILER_TOKEN_TYPE_CHAR,
				                                                            TSMS_STRING_clone(value)));
			else
				TSMS_LIST_add(currentToken->children, __tsms_internal_create_token(TSMS_COMPILER_TOKEN_TYPE_STRING,
				                                                            TSMS_STRING_clone(value)));
		} else if (type == _TOKEN_TYPE_UNDEFINED) {
			if (value->length > 0)
				if (__tsms_internal_is_number(value))
					TSMS_LIST_add(currentToken->children, __tsms_internal_create_token(TSMS_COMPILER_TOKEN_TYPE_NUMBER,
					                                                            TSMS_STRING_clone(value)));
				else
					TSMS_LIST_add(currentToken->children, __tsms_internal_create_token(TSMS_COMPILER_TOKEN_TYPE_UNDEFINED,
				                                                            TSMS_STRING_clone(value)));
		} else if (type == _TOKEN_TYPE_KEYWORD) {
			char c = value->cStr[0];
			if (__tsms_internal_is_left_bracket(c)) {
				TSMS_INT_STACK_push(stack, c);
				pCompilerToken newToken = __tsms_internal_create_block_token(currentToken, TSMS_STRING_clone(value));
				TSMS_LIST_add(currentToken->children, newToken);
				currentToken = newToken;
			} else if (__tsms_internal_is_right_bracket(c)) {
				char left = TSMS_INT_STACK_peek(stack);
				if (__tsms_internal_is_left_right_match(left, c)) {
					TSMS_INT_STACK_pop(stack);
					currentToken = currentToken->parent;
				} else {
					TSMS_STRING_release(value);
					free(token);
					for (TSMS_POS j = i + 1; j < tokens->length; j++) {
						token = tokens->list[j];
						TSMS_STRING_release(token->value);
						free(token);
					}
					TSMS_INT_STACK_release(stack);
					TSMS_STRING_release(temp);
					TSMS_STRING_release(trim);
					TSMS_LIST_release(tokens);
					TSMS_COMPILER_PROGRAM_release(program);
					return TSMS_NULL;
				}
			} else
				TSMS_LIST_add(currentToken->children, __tsms_internal_create_token(TSMS_COMPILER_TOKEN_TYPE_KEYWORD,
				                                                            TSMS_STRING_clone(value)));
		}
		TSMS_STRING_release(value);
		free(token);
	}

	TSMS_INT_STACK_release(stack);

	TSMS_STRING_release(temp);
	TSMS_STRING_release(trim);
	TSMS_LIST_release(tokens);
	return program;
}

TSMS_RESULT TSMS_COMPILER_release(pCompiler compiler) {
	if (compiler == TSMS_NULL)
		return TSMS_ERROR;
	free(compiler);
	return TSMS_SUCCESS;
}

TSMS_RESULT TSMS_COMPILER_PROGRAM_release(pCompilerPreProgram program) {
	if (program == TSMS_NULL)
		return TSMS_ERROR;
	TSMS_COMPILER_TOKEN_release(program->token);
	free(program);
	return TSMS_SUCCESS;
}

TSMS_RESULT TSMS_COMPILER_TOKEN_release(pCompilerToken token) {
	if (token == TSMS_NULL)
		return TSMS_ERROR;
	if (token->children != TSMS_NULL)
		for (TSMS_POS i = 0; i < token->children->length; i++) {
			pCompilerToken child = token->children->list[i];
			TSMS_COMPILER_TOKEN_release(child);
		}
	TSMS_LIST_release(token->children);
	TSMS_STRING_release(token->value);
	free(token);
	return TSMS_SUCCESS;
}

TSMS_RESULT TSMS_COMPILER_PROGRAM_remove(pCompilerPreProgram program, TSMS_COMPILER_TOKEN_TYPE type, pString value) {
	if (program == TSMS_NULL)
		return TSMS_ERROR;
	__tsms_internal_remove_token(program->token, type, value);
	return TSMS_SUCCESS;
}

TSMS_RESULT TSMS_COMPILER_PROGRAM_split(pCompilerPreProgram program, TSMS_COMPILER_TOKEN_TYPE type, TSMS_COMPILER_TOKEN_TYPE splitType, pString splitValue) {
	if (program == TSMS_NULL)
		return TSMS_ERROR;
	__tsms_internal_split_token(program->token, type, splitType, splitValue);
	return TSMS_SUCCESS;
}