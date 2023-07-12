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
                                  '(', ')', '[', ']', '{', '}', '+', '-', '*', '/', '%', '&', '|', '^', '~', '!', '=',
                                  '<', '>', '?', '#', '\n', '\r', '\t', 0};

TSMS_INLINE bool __tsms_internal_is_keyword(char c) {
	return strchr(_compilerKeyword, c) != TSMS_NULL;
}

TSMS_INLINE struct _token *__tsms_internal_create_internal_token(_tokenType type, pString value) {
	struct _token *token = TSMS_malloc(sizeof(struct _token));
	token->type = type;
	token->value = value;
	return token;
}

TSMS_INLINE pCompilerToken __tsms_internal_create_token(TSMS_COMPILER_TOKEN_TYPE type, pString value) {
	pCompilerToken token = (pCompilerToken) TSMS_malloc(sizeof(tCompilerToken));
	token->type = type;
	token->value = value;
	return token;
}

TSMS_INLINE pCompilerBlockToken __tsms_internal_create_block_token(pCompilerSplittedToken parent, pString value) {
	pCompilerBlockToken token = (pCompilerBlockToken) TSMS_malloc(sizeof(tCompilerBlockToken));
	token->type = TSMS_COMPILER_TOKEN_TYPE_BLOCK;
	token->value = value;
	token->children = TSMS_LIST_create(10);
	token->parent = parent;
	return token;
}

TSMS_INLINE pCompilerSplittedToken __tsms_internal_create_splitted_token(pCompilerSplittedToken parent, TSMS_LP tokens) {
	pCompilerSplittedToken token = (pCompilerSplittedToken) TSMS_malloc(sizeof(tCompilerSplittedToken));
	token->type = TSMS_COMPILER_TOKEN_TYPE_SPLITTED;
	token->value = TSMS_NULL;
	token->children = tokens;
	token->parent = parent;
	return token;
}

TSMS_INLINE pCompilerDefineToken __tsms_internal_create_define_token(pCompilerSplittedToken parent, pString value, TSMS_LP tokens, TSMS_POS i, TSMS_SIZE size, tCompilerTokenDefinition* definition) {
	pCompilerDefineToken token = (pCompilerDefineToken) TSMS_malloc(sizeof(tCompilerDefineToken));
	token->type = TSMS_COMPILER_TOKEN_TYPE_DEFINE;
	token->value = value;
	token->children = TSMS_LIST_create(10);
	token->parent = parent;
	token->blocks = TSMS_INT_LIST_create(10);
	for (TSMS_POS j = 0; j < size; j++) {
		TSMS_LIST_add(token->children, tokens->list[i + j]);
		TSMS_INT_LIST_add(token->blocks, definition[j].isBlock ? 1 : 0);
	}
	return token;
}

TSMS_INLINE bool __tsms_internal_is_number(pString str) {
	for (TSMS_POS i = 0; i < str->length; i++)
		if (!isdigit(str->cStr[i]))
			return false;
	return true;
}

TSMS_INLINE bool __tsms_internal_is_left_bracket(char c) {
	return c == '(' || c == '[' || c == '{';
}

TSMS_INLINE bool __tsms_internal_is_right_bracket(char c) {
	return c == ')' || c == ']' || c == '}';
}

TSMS_INLINE bool __tsms_internal_is_left_right_match(char left, char right) {
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

TSMS_INLINE bool __tsms_internal_is_in_array(pString* array, pString str, TSMS_SIZE size) {
	for (TSMS_POS i = 0; i < size; i++)
		if (TSMS_STRING_equals(array[i], str))
			return true;
	return false;
}

TSMS_INLINE void __tsms_internal_print_space(int count) {
	for (int i = 0; i < count * 2; i++)
		printf(" ");
}

TSMS_INLINE void __tsms_internal_print_token(pCompilerToken t, int level) {
	if (t->type == TSMS_COMPILER_TOKEN_TYPE_BLOCK) {
		pCompilerBlockToken token = (pCompilerBlockToken) t;
		__tsms_internal_print_space(level);
		if (token->value != TSMS_NULL)
			printf("Block: %s\n", token->value->cStr);
		else printf("Block: (null)\n");
		__tsms_internal_print_space(level);
		printf("Children start (%d): \n", level);
		for (TSMS_POS i = 0; i < token->children->length; i++) {
			pCompilerToken child = token->children->list[i];
			__tsms_internal_print_token(child, level + 1);
		}
		__tsms_internal_print_space(level);
		printf("Children end.\n");
	} else if (t->type == TSMS_COMPILER_TOKEN_TYPE_SPLITTED) {
		pCompilerSplittedToken token = (pCompilerSplittedToken) t;
		__tsms_internal_print_space(level);
		printf("Splitted start (%d): \n", level);
		for (TSMS_POS i = 0; i < token->children->length; i++) {
			pCompilerToken child = token->children->list[i];
			__tsms_internal_print_token(child, level + 1);
		}
		__tsms_internal_print_space(level);
		printf("Splitted end.\n");
	} else if (t->type == TSMS_COMPILER_TOKEN_TYPE_DEFINE) {
		pCompilerDefineToken token = (pCompilerDefineToken) t;
		__tsms_internal_print_space(level);
		printf("UserDefined: %s\n", token->value->cStr);
		__tsms_internal_print_space(level);
		printf("UserDefined start (%d): \n", level);
		for (TSMS_POS i = 0; i < token->children->length; i++) {
			pCompilerToken child = token->children->list[i];
			__tsms_internal_print_token(child, level + 1);
		}
		__tsms_internal_print_space(level);
		printf("UserDefined end.\n");
	} else {
		__tsms_internal_print_space(level);
		printf("TokenType: %d, Value: %s\n", t->type, t->value->cStr);
	}
}

TSMS_INLINE void __tsms_internal_remove_token(pCompilerToken t, TSMS_COMPILER_TOKEN_TYPE type, pString value) {
	if (t->type == TSMS_COMPILER_TOKEN_TYPE_BLOCK) {
		pCompilerBlockToken token = (pCompilerBlockToken) t;
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
}

TSMS_INLINE void __tsms_internal_split_token(pCompilerToken t, TSMS_COMPILER_TOKEN_TYPE type, pString value,
                                        TSMS_COMPILER_TOKEN_TYPE splitType, pString splitValue) {
	if (t->type == TSMS_COMPILER_TOKEN_TYPE_BLOCK || t->type == TSMS_COMPILER_TOKEN_TYPE_SPLITTED) {
		pCompilerSplittedToken token = (pCompilerSplittedToken) t;
		if (token->type == type && (value == TSMS_NULL || TSMS_STRING_equals(token->value, value))) {
			TSMS_LP tokens = TSMS_LIST_create(10);
			TSMS_LP current = TSMS_LIST_create(10);
			for (TSMS_POS i = 0; i < token->children->length; i++) {
				pCompilerToken subChild = token->children->list[i];
				if (subChild->type == splitType && TSMS_STRING_equals(subChild->value, splitValue)) {
					if (current->length > 0) {
						pCompilerToken newToken = __tsms_internal_create_splitted_token(token ,current);
						current = TSMS_LIST_create(10);
						TSMS_LIST_add(tokens, newToken);
					}
				} else {
					TSMS_LIST_add(current, subChild);
				}
			}
			if (current->length > 0) {
				pCompilerToken newToken = __tsms_internal_create_splitted_token(token, current);
				TSMS_LIST_add(tokens, newToken);
			} else TSMS_LIST_release(current);
			TSMS_LIST_release(token->children);
			token->children = tokens;
		}

		for (TSMS_POS i = 0; i < token->children->length; i++) {
			pCompilerToken child = token->children->list[i];
			if (child->type != TSMS_COMPILER_TOKEN_TYPE_BLOCK && child->type != TSMS_COMPILER_TOKEN_TYPE_SPLITTED) {
				if (child->type == type && (value == TSMS_NULL || TSMS_STRING_equals(child->value, value))) {
					if (splitType == TSMS_COMPILER_TOKEN_TYPE_STRING) {
						TSMS_LIST_remove(token->children, i);
						TSMS_LP list = TSMS_STRING_splitByString(child->value, splitValue);
						TSMS_POS counter = 0;
						for (TSMS_POS j = 0; j < list->length; j++) {
							pString str = list->list[j];
							pString trim = TSMS_STRING_trim(str);
							if (trim->length > 0) {
								pCompilerToken newToken;
								if (__tsms_internal_is_number(trim))
									newToken = __tsms_internal_create_token(TSMS_COMPILER_TOKEN_TYPE_NUMBER, trim);
								else
									newToken = __tsms_internal_create_token(type, trim);
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
						TSMS_POS counter = 0;
						for (TSMS_POS j = 0; j < list->length; j++) {
							pString str = list->list[j];
							pString trim = TSMS_STRING_trim(str);
							if (trim->length > 0) {
								pCompilerToken newToken;
								if (__tsms_internal_is_number(trim))
									newToken = __tsms_internal_create_token(TSMS_COMPILER_TOKEN_TYPE_NUMBER, trim);
								else
									newToken = __tsms_internal_create_token(type, trim);
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
			} else
				__tsms_internal_split_token(child, type, value, splitType, splitValue);
		}
	}
}

TSMS_INLINE bool __tsms_internal_match_define(TSMS_LP tokens, TSMS_POS index, tCompilerTokenDefinition * definition, TSMS_SIZE size) {
	for (TSMS_POS i = 0; i < size; i++) {
		pCompilerToken token = tokens->list[index + i];
		if (token->type != definition[i].type)
			return false;
		if (definition[i].value != TSMS_NULL && !TSMS_STRING_equals(token->value, definition[i].value))
			return false;
	}
	return true;
}

TSMS_INLINE void __tsms_internal_define_token(pCompilerToken t, pString value, tCompilerTokenDefinition * definition, TSMS_SIZE size) {
	if (t->type == TSMS_COMPILER_TOKEN_TYPE_BLOCK || t->type == TSMS_COMPILER_TOKEN_TYPE_SPLITTED || t->type == TSMS_COMPILER_TOKEN_TYPE_DEFINE) {
		pCompilerSplittedToken token = (pCompilerSplittedToken) t;
		TSMS_LP tokens = TSMS_LIST_create(10);
		for (TSMS_POS i = 0; i < token->children->length; i++) {
			pCompilerToken child = token->children->list[i];
			bool flag = true;
			if (t->type == TSMS_COMPILER_TOKEN_TYPE_DEFINE) {
				pCompilerDefineToken defineToken = (pCompilerDefineToken) t;
				if (!defineToken->blocks->list[i])
					flag = false;
			}
			if (flag)
				__tsms_internal_define_token(child, value, definition, size);
			if (t->type != TSMS_COMPILER_TOKEN_TYPE_DEFINE && i + size < token->children->length + 1 && __tsms_internal_match_define(token->children, i, definition, size)) {
				pCompilerToken newToken = __tsms_internal_create_define_token(token, value, token->children, i, size, definition);
				TSMS_LIST_add(tokens, newToken);
				i += size - 1;
			} else TSMS_LIST_add(tokens, child);
		}
		TSMS_LIST_release(token->children);
		token->children = tokens;
	}
}

TSMS_INLINE void __tsms_internal_merge_keyword(pCompilerToken t, pString * keywords, TSMS_SIZE size) {
	if (t->type == TSMS_COMPILER_TOKEN_TYPE_BLOCK) {
		pCompilerBlockToken token = (pCompilerBlockToken) t;
		TSMS_LP tokens = TSMS_LIST_create(10);
		for (TSMS_POS i = 0; i < token->children->length;) {
			pCompilerToken child = token->children->list[i];
			__tsms_internal_merge_keyword(child, keywords, size);
			if (child->type == TSMS_COMPILER_TOKEN_TYPE_KEYWORD) {
				pCompilerToken current;
				pString value = TSMS_STRING_clone(child->value);
				TSMS_POS pos = i;
				while (true) {
					pos++;
					if (pos >= token->children->length)
						break;
					current = token->children->list[pos];
					if (current->type != TSMS_COMPILER_TOKEN_TYPE_KEYWORD)
						break;
					TSMS_STRING_append(value, current->value);
				}
				if (value->length != 1 && __tsms_internal_is_in_array(keywords, value, size)) {
					TSMS_COMPILER_TOKEN_release(child);
					pCompilerToken newToken = __tsms_internal_create_token(TSMS_COMPILER_TOKEN_TYPE_MERGED_KEYWORD, value);
					TSMS_LIST_add(tokens, newToken);
					for (TSMS_POS j = i + 1; j < pos; j++)
						TSMS_COMPILER_TOKEN_release(token->children->list[j]);
					i = pos;
				} else {
					TSMS_LIST_add(tokens, child);
					TSMS_STRING_release(value);
					i++;
				}
			} else {
				TSMS_LIST_add(tokens, child);
				i++;
			}
		}
		TSMS_LIST_release(token->children);
		token->children = tokens;
	}
}

TSMS_INLINE pCompilerToken __tsms_internal_clone_token(pCompilerToken t, pCompilerSplittedToken parent, bool deep) {
	switch (t->type) {
		case TSMS_COMPILER_TOKEN_TYPE_CHAR:
		case TSMS_COMPILER_TOKEN_TYPE_STRING:
		case TSMS_COMPILER_TOKEN_TYPE_NUMBER:
		case TSMS_COMPILER_TOKEN_TYPE_KEYWORD:
		case TSMS_COMPILER_TOKEN_TYPE_UNDEFINE:
		case TSMS_COMPILER_TOKEN_TYPE_MERGED_KEYWORD:
			return __tsms_internal_create_token(t->type, TSMS_STRING_clone(t->value));
		case TSMS_COMPILER_TOKEN_TYPE_BLOCK: {
			pCompilerBlockToken token = (pCompilerBlockToken) t;
			pCompilerBlockToken clone = __tsms_internal_create_block_token(parent, TSMS_STRING_clone(t->value));
			if (deep)
				for (TSMS_POS i = 0; i < token->children->length; i++) {
					pCompilerToken child = token->children->list[i];
					TSMS_LIST_add(clone->children, __tsms_internal_clone_token(child, clone,  deep));
				}
			return clone;
		}
		case TSMS_COMPILER_TOKEN_TYPE_SPLITTED: {
			pCompilerSplittedToken token = (pCompilerSplittedToken) t;
			pCompilerSplittedToken clone = __tsms_internal_create_splitted_token(parent, TSMS_LIST_create(10));
			if (deep)
				for (TSMS_POS i = 0; i < token->children->length; i++) {
					pCompilerToken child = token->children->list[i];
					TSMS_LIST_add(clone->children, __tsms_internal_clone_token(child, clone, deep));
				}
			return clone;
		}
		case TSMS_COMPILER_TOKEN_TYPE_DEFINE: {
			pCompilerDefineToken token = (pCompilerDefineToken) t;
			pCompilerDefineToken clone = TSMS_malloc(sizeof(tCompilerDefineToken));
			clone->type = token->type;
			clone->value = TSMS_STRING_clone(token->value);
			clone->children = TSMS_LIST_create(10);
			for (TSMS_POS i = 0; i < token->children->length; i++) {
				pCompilerToken child = token->children->list[i];
				TSMS_LIST_add(clone->children, __tsms_internal_clone_token(child, clone, deep || token->blocks->list[i] != 1));
			}
			clone->blocks = TSMS_INT_LIST_create(10);
			for (TSMS_POS i = 0; i < token->blocks->length; i++)
				TSMS_INT_LIST_add(clone->blocks, token->blocks->list[i]);
			return (pCompilerToken) clone;
		}
	}
}

TSMS_INLINE pCompilerBlockToken __tsms_internal_find_block_token(pCompilerSplittedToken token) {
	if (token == TSMS_NULL)
		return TSMS_NULL;
	if (token->type == TSMS_COMPILER_TOKEN_TYPE_BLOCK)
		return (pCompilerBlockToken) token;
	return __tsms_internal_find_block_token(token->parent);
}

TSMS_INLINE pCompilerSplittedToken __tsms_internal_rebuild(pCompilerToken t, const pCompilerSplittedToken parent) {
	if (t->type == TSMS_COMPILER_TOKEN_TYPE_BLOCK) {
		pCompilerBlockToken token = (pCompilerBlockToken) t;
		pCompilerBlockToken block;
		if (parent == TSMS_NULL)
			block = __tsms_internal_create_block_token(TSMS_NULL, TSMS_STRING_clone(token->value));
		else {
			block = __tsms_internal_create_block_token(parent, TSMS_STRING_clone(token->value));
			TSMS_LIST_add(parent->children, block);
		}
		for (TSMS_POS i = 0; i < token->children->length; i++) {
			pCompilerToken child = token->children->list[i];
			__tsms_internal_rebuild(child, block);
		}
		if (parent == TSMS_NULL)
			return block;
	} else if (t->type == TSMS_COMPILER_TOKEN_TYPE_SPLITTED) {
		pCompilerSplittedToken token = (pCompilerSplittedToken) t;
		pCompilerSplittedToken splitted = parent;
		if (parent->type != TSMS_COMPILER_TOKEN_TYPE_SPLITTED)
			splitted = __tsms_internal_create_splitted_token(parent, TSMS_LIST_create(10));
		for (TSMS_POS i = 0; i < token->children->length; i++) {
			pCompilerToken child = token->children->list[i];
			__tsms_internal_rebuild(child, splitted);
		}
		if (parent->type != TSMS_COMPILER_TOKEN_TYPE_SPLITTED) {
			if (splitted->children->length != 0)
				TSMS_LIST_add(parent->children, splitted);
			else TSMS_COMPILER_SPLITTED_TOKEN_release(splitted);
		}
	} else if (t->type == TSMS_COMPILER_TOKEN_TYPE_DEFINE) {
		pCompilerDefineToken token = (pCompilerDefineToken) t;
		pCompilerDefineToken clone = __tsms_internal_clone_token(t, parent, false);
		for (TSMS_POS i = 0; i < clone->blocks->length; i++) {
			int block = clone->blocks->list[i];
			if (block) {
				pCompilerSplittedToken temp = __tsms_internal_rebuild(token->children->list[i], TSMS_NULL);
				TSMS_COMPILER_BLOCK_TOKEN_release(clone->children->list[i]);
				clone->children->list[i] = temp;
			}
		}

		TSMS_LIST_add(__tsms_internal_find_block_token(parent)->children, clone);
	} else {
		TSMS_LIST_add(parent->children, __tsms_internal_clone_token(t, parent, false));
	}
	return parent;
}

TSMS_INLINE pCompilerBlockSentence __tsms_internal_compile_sentence(pCompilerToken token) {
	
}

TSMS_INLINE pCompilerProgram __tsms_internal_compile_token(pCompilerToken token) {
	pCompilerProgram program = (pCompilerProgram) TSMS_malloc(sizeof(tCompilerProgram));
	pCompilerBlockToken blockToken = __tsms_internal_rebuild(token, TSMS_NULL);
	__tsms_internal_print_token(blockToken, 0);
	program->sentence = __tsms_internal_compile_sentence(token);
	return program;
}

pCompiler TSMS_COMPILER_create() {
	pCompiler compiler = (pCompiler) TSMS_malloc(sizeof(tCompiler));
	compiler->acceptLittleQuote = false;
	return compiler;
}

pCompilerPreProgram TSMS_COMPILER_compile(pCompiler compiler, pString source) {
	pCompilerPreProgram program = (pCompilerPreProgram) TSMS_malloc(sizeof(tCompilerPreProgram));
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
			if (!inQuote) {
				if (temp->length > 0) {
					TSMS_LIST_add(tokens,
					              __tsms_internal_create_internal_token(_TOKEN_TYPE_UNDEFINED, TSMS_STRING_clone(temp)));
					TSMS_STRING_clear(temp);
				}
			}
			inQuote = !inQuote;
			if (!inQuote) {
				TSMS_LIST_add(tokens,
				              __tsms_internal_create_internal_token(_TOKEN_TYPE_QUOTE, TSMS_STRING_clone(temp)));
				TSMS_STRING_clear(temp);
			}
		} else if (c == '\'' && !inQuote) {
			if (!inLittleQuote) {
				if (temp->length > 0) {
					TSMS_LIST_add(tokens,
					              __tsms_internal_create_internal_token(_TOKEN_TYPE_UNDEFINED, TSMS_STRING_clone(temp)));
					TSMS_STRING_clear(temp);
				}
			}
			inLittleQuote = !inLittleQuote;
			if (!inLittleQuote) {
				TSMS_LIST_add(tokens,
				              __tsms_internal_create_internal_token(_TOKEN_TYPE_LITTLE_QUOTE, TSMS_STRING_clone(temp)));
				TSMS_STRING_clear(temp);
			}
		} else if (inQuote || inLittleQuote || !__tsms_internal_is_keyword(c)) {
			TSMS_STRING_appendChar(temp, c);
		} else {
			if (temp->length > 0) {
				TSMS_LIST_add(tokens,
				              __tsms_internal_create_internal_token(_TOKEN_TYPE_UNDEFINED, TSMS_STRING_trim(temp)));
				TSMS_STRING_clear(temp);
			}
			TSMS_LIST_add(tokens,
			              __tsms_internal_create_internal_token(_TOKEN_TYPE_KEYWORD, TSMS_STRING_createWithChar(c)));
		}
	}

	if (inEscape || inQuote || inLittleQuote) {
		TSMS_STRING_release(temp);
		TSMS_STRING_release(trim);
		for (TSMS_POS i = 0; i < tokens->length; i++) {
			struct _token *token = tokens->list[i];
			TSMS_STRING_release(token->value);
			free(token);
		}
		TSMS_LIST_release(tokens);
		TSMS_COMPILER_PRE_PROGRAM_release(program);
		return TSMS_NULL;
	}

	if (temp->length > 0) {
		TSMS_LIST_add(tokens, __tsms_internal_create_internal_token(_TOKEN_TYPE_UNDEFINED, TSMS_STRING_clone(temp)));
		TSMS_STRING_clear(temp);
	}

	pCompilerBlockToken currentToken = __tsms_internal_create_block_token(TSMS_NULL, TSMS_STRING_static("{"));
	program->token = currentToken;

	TSMS_ISTP stack = TSMS_INT_STACK_create();

	for (TSMS_POS i = 0; i < tokens->length; i++) {
		struct _token *token = tokens->list[i];

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
			if (value->length > 0 && __tsms_internal_is_number(value))
				TSMS_LIST_add(currentToken->children, __tsms_internal_create_token(TSMS_COMPILER_TOKEN_TYPE_NUMBER,
				                                                                   TSMS_STRING_clone(value)));
			else
				TSMS_LIST_add(currentToken->children,
				              __tsms_internal_create_token(TSMS_COMPILER_TOKEN_TYPE_UNDEFINE,
				                                           TSMS_STRING_clone(value)));
		} else if (type == _TOKEN_TYPE_KEYWORD) {
			char c = value->cStr[0];
			if (__tsms_internal_is_left_bracket(c)) {
				TSMS_INT_STACK_push(stack, c);
				pCompilerBlockToken newToken = __tsms_internal_create_block_token(currentToken, TSMS_STRING_clone(value));
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
					TSMS_COMPILER_PRE_PROGRAM_release(program);
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

TSMS_RESULT TSMS_COMPILER_PRE_PROGRAM_release(pCompilerPreProgram program) {
	if (program == TSMS_NULL)
		return TSMS_ERROR;
	TSMS_COMPILER_TOKEN_release(program->token);
	free(program);
	return TSMS_SUCCESS;
}

TSMS_RESULT TSMS_COMPILER_TOKEN_release(pCompilerToken token) {
	if (token == TSMS_NULL)
		return TSMS_ERROR;
	TSMS_STRING_release(token->value);
	free(token);
	return TSMS_SUCCESS;
}

TSMS_RESULT TSMS_COMPILER_TOKEN_releaseByType(pCompilerToken token) {
	if (token == TSMS_NULL)
		return TSMS_ERROR;
	switch (token->type) {
		case TSMS_COMPILER_TOKEN_TYPE_STRING:
		case TSMS_COMPILER_TOKEN_TYPE_CHAR:
		case TSMS_COMPILER_TOKEN_TYPE_NUMBER:
		case TSMS_COMPILER_TOKEN_TYPE_UNDEFINE:
		case TSMS_COMPILER_TOKEN_TYPE_KEYWORD:
			return TSMS_COMPILER_TOKEN_release(token);
		case TSMS_COMPILER_TOKEN_TYPE_BLOCK:
			return TSMS_COMPILER_BLOCK_TOKEN_release(token);
		case TSMS_COMPILER_TOKEN_TYPE_SPLITTED:
			return TSMS_COMPILER_SPLITTED_TOKEN_release(token);
		case TSMS_COMPILER_TOKEN_TYPE_DEFINE:
			return TSMS_COMPILER_DEFINE_TOKEN_release(token);
		default:
			return TSMS_ERROR;
	}
}

TSMS_RESULT TSMS_COMPILER_SPLITTED_TOKEN_release(pCompilerSplittedToken token) {
	if (token == TSMS_NULL)
		return TSMS_ERROR;
	for (TSMS_POS i = 0; i < token->children->length; i++) {
		pCompilerToken child = token->children->list[i];
		TSMS_COMPILER_TOKEN_releaseByType(child);
	}
	TSMS_LIST_release(token->children);
	return TSMS_COMPILER_TOKEN_release(token);
}

TSMS_RESULT TSMS_COMPILER_BLOCK_TOKEN_release(pCompilerBlockToken token) {
	if (token == TSMS_NULL)
		return TSMS_ERROR;
	return TSMS_COMPILER_SPLITTED_TOKEN_release(token);
}

TSMS_RESULT TSMS_COMPILER_DEFINE_TOKEN_release(pCompilerDefineToken token) {
	if (token == TSMS_NULL)
		return TSMS_ERROR;
	TSMS_INT_LIST_release(token->blocks);
	return TSMS_COMPILER_SPLITTED_TOKEN_release(token);
}

TSMS_RESULT TSMS_COMPILER_PROGRAM_remove(pCompilerPreProgram program, TSMS_COMPILER_TOKEN_TYPE type, pString value) {
	if (program == TSMS_NULL)
		return TSMS_ERROR;
	__tsms_internal_remove_token(program->token, type, value);
	return TSMS_SUCCESS;
}

TSMS_RESULT TSMS_COMPILER_PROGRAM_split(pCompilerPreProgram program, TSMS_COMPILER_TOKEN_TYPE type, pString value,
                                        TSMS_COMPILER_TOKEN_TYPE splitType, pString splitValue) {
	if (program == TSMS_NULL)
		return TSMS_ERROR;
	__tsms_internal_split_token(program->token, type, value, splitType, splitValue);
	return TSMS_SUCCESS;
}

TSMS_RESULT TSMS_COMPILER_PROGRAM_define(pCompilerPreProgram program, pString value, tCompilerTokenDefinition * definition, TSMS_SIZE size) {
	if (program == TSMS_NULL)
		return TSMS_ERROR;
	__tsms_internal_define_token(program->token, value, definition, size);
	return TSMS_SUCCESS;
}

TSMS_RESULT TSMS_COMPILER_PROGRAM_mergeKeyword(pCompilerPreProgram program, pString * keywords, TSMS_SIZE size) {
	if (program == TSMS_NULL)
		return TSMS_ERROR;
	__tsms_internal_merge_keyword(program->token, keywords, size);
	return TSMS_SUCCESS;
}

pCompilerProgram TSMS_COMPILER_PROGRAM_compile(pCompilerPreProgram program) {
	if (program == TSMS_NULL)
		return TSMS_NULL;
	pCompilerProgram p = __tsms_internal_compile_token(program->token);
	if (p != TSMS_NULL)
		TSMS_COMPILER_PRE_PROGRAM_release(program);
	return p;
}

TSMS_RESULT TSMS_COMPILER_PROGRAM_print(pCompilerPreProgram program) {
	if (program == TSMS_NULL)
		return TSMS_ERROR;
	__tsms_internal_print_token(program->token, 0);
	return TSMS_SUCCESS;
}