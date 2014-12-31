#include <stdio.h>
#include <stdlib.h>
#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>

#include "utils.h"

#ifdef DEBUG
	#define dbg_printf(...) (fprintf(stderr, __FILE__ ":%d:%s:", __LINE__, __func__), (fprintf(stderr, __VA_ARGS__)))
#else
	#define dbg_printf(...)
#endif

//---------------------------------------------------------
//string
//---------------------------------------------------------
#define STRSIZE 256

typedef struct {
	int length;
	int size;
	char *str;
} str_with_len, * str_with_len_p;

str_with_len_p init_str_with_len(str_with_len_p str) {
	if (str == NULL) {
		dbg_printf("failed to %s\n", __func__);
		exit(EXIT_FAILURE);
	}

	char *tmp = malloc(sizeof(char[STRSIZE]));
	if (tmp == NULL) {
		dbg_printf("failed to malloc\n");
		exit(EXIT_FAILURE);
	}

	str->str = tmp;
	str->length = 0;
	str->size = STRSIZE;
	memset(str->str, 0, sizeof(str->str[0]) * STRSIZE);

	return str;
}

str_with_len_p fini_str_with_len(str_with_len_p str) {
	if (str == NULL) {
		dbg_printf("failed to %s\n", __func__);
		exit(EXIT_FAILURE);
	}

	free(str->str);

	str->str = NULL;
	str->length = 0;
	str->size = 0;

	return str;
}

str_with_len_p str_with_len_ncat(str_with_len_p dest, char *src, size_t len) {
	if (len > (dest->size - dest->length)) {
		char *tmp = realloc(dest->str, dest->size + STRSIZE);
		if (tmp == NULL) {
			dbg_printf("realloc failed\n");
			exit(EXIT_FAILURE);
		}
		dest->str = tmp;
		dest->size += STRSIZE;
		dbg_printf("realloced\n");
	}
	strncat(dest->str + dest->length, src, len);
	dest->length += len;

	return dest;
}

str_with_len_p read_from(FILE *fp, str_with_len_p dest) {
	if(fp == NULL) {
		fini_str_with_len(dest);
		return dest;
	}

	if(dest->str == NULL) {
		init_str_with_len(dest);
	}

	char tmp[STRSIZE] = "";
	char *ret = fgets(tmp, sizeof(tmp), fp);
	if (ret == NULL) {
		//dbg_printf("error occured or reach to EOF\n");
		return NULL;
	}
	
	int len = strlen(tmp);
	str_with_len_ncat(dest, tmp, len);
	return dest;
}

//---------------------------------------------------------
//cell
//---------------------------------------------------------
/*
terms:
	car_p
	car_p cdr_p
term:
	'S'|'K'|'I'|atom
	'(' cdr_p ')'

*/

typedef enum {
	// COMB,
	PAIR,
	ATOM,
	UINT,
	BROKEN_HEART = 0x7,
} cell_type;

//typedef struct struct_cell * cell_p;
typedef struct struct_cell {
	struct struct_cell *car_p;
	struct struct_cell *cdr_p;
} cell_t, * cell_p;

#define car(c) ((c)->car_p)
#define cdr(c) ((c)->cdr_p)

static inline cell_p mask_cell(cell_p const c, cell_type const mask) {
	return (cell_p)((uintptr_t)(c) | mask);
}

static inline cell_p unmask_cell(cell_p const c, cell_type const mask) {
	return (cell_p)((uintptr_t)(c) & ~mask);
}

static inline cell_p unmask_cell_all(cell_p const c) {
	return (cell_p)((uintptr_t)(c) & ~BROKEN_HEART);
}

static inline int is_cell_type(cell_p const c, cell_type const t) {
	return (uintptr_t)(c) & t;
}

static inline int is_pair(cell_p const c) {
	return ((uintptr_t)(c) & 0x7) == 0;
}

static inline int is_atom(cell_p const c) {
	return is_cell_type(c, ATOM);
}

static inline char *to_atom(cell_p const c) {
	static char atom[4];
	uint32_t tmp;
	tmp = (((uint32_t)((uintptr_t)c)) >> 3) | (0x7ul * ((uint32_t)((uintptr_t)c) >> 31)) << 29;
	atom[0] = (char)((tmp >>  0) & 0xff);
	atom[1] = (char)((tmp >>  8) & 0xff);
	atom[2] = (char)((tmp >> 16) & 0xff);
	atom[3] = (char)((tmp >> 24) & 0xff);
	return atom;
}

static inline int is_uint(cell_p const c) {
	return is_cell_type(c, UINT);
}

static inline uint32_t to_uint(cell_p const c) {
	return ((uint32_t)((uintptr_t)c)) >> 3;
}

static inline int is_nil(cell_p const c) {
	return is_pair(c) && !car(unmask_cell_all(c));
}

cell_t *cell_heap = NULL;
#define ast_root (car(cell_heap))

#define MEMO_SIZE 0x10000
size_t heap_size = MEMO_SIZE, next_heap_size = (MEMO_SIZE * 3) / 2, heap_used = 0;

cell_t *alloc_heap(size_t const size) {
	cell_t *tmp = (cell_t *)aligned_alloc(alignof(8), sizeof(cell_t) * size);
	if (tmp == NULL) {
		dbg_printf("failed to malloc\n");
		exit(EXIT_FAILURE);
	}
	dbg_printf("alloced pointer = %p, heap_size = %u\n", (void*)tmp, size);
	memset(tmp, 0, sizeof(cell_t) * size);
	return tmp;
}

cell_t *init_cell_heap(void) {
	return cell_heap = alloc_heap(MEMO_SIZE);
}

cell_p gc_run(cell_t *new_heap) {
	new_heap[0] = *cell_heap;
	*cell_heap = (cell_t){(cell_p)BROKEN_HEART, new_heap};
	cell_p free_cell = new_heap, scan = new_heap;
	free_cell++;

	for(; free_cell != scan;scan++) {
		if (car(scan)) {
			if (is_pair(car(scan)) && (uintptr_t)car(car(scan)) != BROKEN_HEART) {
				*free_cell = *car(scan);
				car(car(scan)) = (cell_p)BROKEN_HEART;
				cdr(car(scan)) = free_cell;
				free_cell++;
			}
			car(scan) = is_pair(car(scan))?cdr(car(scan)):car(scan);
		}
		if (cdr(scan)) {
			if (is_pair(cdr(scan)) && (uintptr_t)car(cdr(scan)) != BROKEN_HEART) {
				*free_cell = *cdr(scan);
				car(cdr(scan)) = (cell_p)BROKEN_HEART;
				cdr(cdr(scan)) = free_cell;
				free_cell++;
			}
			cdr(scan) = is_pair(cdr(scan))?cdr(cdr(scan)):cdr(scan);
		}
	}
	free(cell_heap);
	cell_heap = new_heap;
	dbg_printf("%d,%d\n", heap_used, free_cell - cell_heap);
	heap_size = next_heap_size;
	next_heap_size = (heap_size * 3) / 2;
	return cell_heap;
}

cell_p alloc_cell(size_t const n) {
	if (!cell_heap) {
		init_cell_heap();
	}
	if (n && heap_used >= heap_size - 1) {
		gc_run(alloc_heap(next_heap_size));
	}
	if (n == 0) {
		for (size_t i = 0; i <= heap_used; i++) {
			dbg_printf(" % 3d(%p)={%p, %p};\n", i, (void*)&cell_heap[i],(void*)cell_heap[i].car_p, (void*)cell_heap[i].cdr_p);
		}
		free(cell_heap);
		return NULL;
	}
	heap_used += n;
	cell_heap[heap_used] = (cell_t){NULL, NULL};
	return cell_heap + heap_used;
	//return n?alloc_heap(n):0;
}

cell_p make_atom(char const *c) {
	/*
	for (cell_p list = obarray; list; list = unmask_cell_all(list)->cdr_p) {
		if (!is_atom(list)) {
			continue;
		}
		if (!strncmp(to_atom(list), c, sizeof(list->car_atom))) {
			return list;
		}
	}
	*/
	//cell_p atm = alloc_cell(1);
	/*
	atm->cdr_p = obarray?obarray:NULL;
	obarray = mask_cell(atm, ATOM);
	*/
	//strncpy(atm->car_atom, c, sizeof(atm->car_atom));
	uint32_t atm = (((uint32_t)c[0] << 0) | ((uint32_t)c[1] << 8) | ((uint32_t)c[2] << 16) | ((c[3]) << 24)) << 3;
	return mask_cell((cell_p)((uintptr_t)atm), ATOM);
}

static inline cell_p make_uint(uint32_t const ui) {
	/*
	for (cell_p list = obarray; list; list = unmask_cell_all(list)->cdr_p) {
		if (!is_uint(list)) {
			continue;
		}
		if (to_uint(list) == ui) {
			return list;
		}
	}
	*/
	//cell_p uint = alloc_cell(1);
	/*
	uint->cdr_p = obarray?obarray:NULL;
	obarray = mask_cell(uint, UINT);
	*/
	//uint->car_uint = ui;
	return mask_cell((cell_p)(uintptr_t)(ui << 3), UINT);
}
//---------------------------------------------------------
//parser
//---------------------------------------------------------
typedef struct {
	cell_type type;
	union {
		char atom[4];
		uint32_t uint;
	};
} token;

char const *brackets[][2] = {
	{ "(", ")" },
	{ "[", "]" },
	{ "{", "}" },
};

bool is_lbracket(token tok) {
	for (size_t i = 0; i < sizeof(brackets) / sizeof(brackets[0]); i++) {
		if (!strncmp(tok.atom, brackets[i][0], brackets[i][0][0]?strlen(brackets[i][0]):1)) return true;
	}
	return false;
}

char const *matchingbracket(token tok) {
	for (size_t i = 0; i < sizeof(brackets) / sizeof(brackets[0]); i++) {
		if (!strncmp(tok.atom, brackets[i][0], brackets[i][0][0]?strlen(brackets[i][0]):1)) return brackets[i][1];
	};
	return "\0";
}

static inline char const *next_token_index(char const *str) {
	do{
		str = utf8_next_char(str);
	} while (isspace(*str));
	return str;
}

token succ_token(char const **str, bool succ) {
	if (isspace(**str)) *str = next_token_index(*str);
	token tok = {PAIR, {""}};
	size_t cbyte = utf8_skip_data[**(unsigned char**)str];
	if (cbyte == 1 && isdigit(**str)) {
		tok.type = UINT;
		char *tmp;
		tok.uint = strtoul(*str, &tmp, 0);
		*str = succ?tmp:*str;
	} else if (cbyte == 1 && isalpha(**str)) {
		tok.type = ATOM;
		tok.atom[0] = **str;
		*str = succ?(*str)+1:*str;
	} else if (cbyte > 1) {
		tok.type = ATOM;
		strncpy(tok.atom, *str, **str?cbyte:1);
		*str = succ?utf8_next_char(*str):*str;
	} else {
		strncpy(tok.atom, *str, **str?cbyte:1);
		*str = succ?utf8_next_char(*str):*str;
	}
	return tok;
}

token get_token(char const **str) {
	return succ_token(str, true);
}

token next_token(char const **str) {
	return succ_token(str, false);
}

cell_p parse_ski(cell_p root, char const **str, char const closing_char[4]) {
	if (!*str) return root;

   	cell_p top = root, fst = NULL, scd = NULL;
	token tok = {PAIR, {""}};
	for (tok = get_token(str); tok.type != PAIR || strncmp(tok.atom, closing_char, sizeof(closing_char) / sizeof(closing_char[0])); tok = get_token(str)) {
		if (tok.type == UINT) {
			car(top) = make_uint(tok.uint);
			//fst = make_uint(tok.uint);
		} else if (tok.type == ATOM) {
			car(top) = make_atom(tok.atom);
			//fst = make_atom(tok.atom);
		} else if (is_lbracket(tok)) {
			cell_p tmp = parse_ski(alloc_cell(1), str, matchingbracket(tok));
			car(top) = tmp;
			//fst = parse_ski(alloc_cell(1), str, matchingbracket(tok));
		} else {
			dbg_printf("parse error:%s (0x%"PRIx32")\n", tok.atom, tok.uint);
			exit(EXIT_FAILURE);
		}
		if (next_token(str).type == PAIR && !strncmp((next_token(str)).atom, closing_char, sizeof(closing_char) / sizeof(closing_char[0]))) {
			//car(top) = fst;
			cdr(top) = NULL;
			//scd = NULL;
			continue;
		}
		cell_p tmp = alloc_cell(1);
		//scd = alloc_cell(1);
		cdr(top) = tmp;
		//top = cons(fst, top);
		//top = cdr(top);
		//car(top) = fst;
		//cdr(top) = scd;
		top = cdr(top);
	}
	return root;
}

cell_p make_ski_ast(cell_p root, char const **str, char const closing_char[4]) {
	if (!*str) return root;

	token tok = {PAIR, {""}};
	tok = get_token(str);
	if (tok.type == PAIR && !strncmp(tok.atom, closing_char, sizeof(closing_char) / sizeof(closing_char[0]))) {
		return root;
	}
	if (tok.type == UINT) {
		car(root) = make_uint(tok.uint);
	} else if (tok.type == ATOM) {
		car(root) = make_atom(tok.atom);
	} else if (is_lbracket(tok)) {
		car(root) = make_ski_ast(alloc_cell(1), str, matchingbracket(tok));
	} else {
		dbg_printf("parse error:%s (0x%"PRIx32")\n", tok.atom, tok.uint);
		exit(EXIT_FAILURE);
	}
	if (next_token(str).type == PAIR && !strncmp((next_token(str)).atom, closing_char, sizeof(closing_char) / sizeof(closing_char[0]))) {
		cdr(root) = NULL;
		get_token(str);
		return root;
	}
	cdr(root) = make_ski_ast(alloc_cell(1), str, closing_char);
	return root;
}

cell_p *simplify_ast(cell_p *root) {
	if (!car(*root)) return NULL;
	cell_p top = *root;
	if (!is_nil(car(top)) && is_pair(car(top))) {
		for (top = car(top); cdr(top); top = cdr(top));
		cdr(top) = cdr(*root);
		*root = car(*root);
	} else {
		for (; cdr(top); top = cdr(top)) {
			if (is_pair(car(cdr(top)))) {
				simplify_ast(&car(cdr(top)));
			}
		}
		return root;
	}
	return simplify_ast(root);
}

int show_ast(cell_p root, int count) {
	if(!root || car(root) == NULL) {
		return 0;
	}
	if (is_atom(car(root))) {
		printf("%1.4s", to_atom(car(root)));
	} else if (is_uint(car(root))) {
		printf("%"PRIu32" ", to_uint(car(root)));
	} else {
		putchar("([{"[count%3]);
		show_ast(car(root), count+1);
		putchar(")]}"[count%3]);
	}
	if (cdr(root) != NULL) {
		show_ast(cdr(root), count);
	}
	return !000;
}

bool is_exist(cell_p root, size_t rec) {
	for(root = cdr(root);rec;rec--) {
		if(!root || !car(root)) {
			return false;
		}
		root = cdr(root);
	}
	return true;
}

cell_p take(cell_p root, size_t n) {
	for (;n;n--) root = cdr(root);
	return root;
}

cell_p copy_ast(cell_p from){
	cell_p new_ast = alloc_cell(1);
	for (cell_p to = new_ast; from; to = cdr(to), from = cdr(from)) {
			car(to) = is_pair(car(from))?copy_ast(car(from)):car(from);
		cdr(to) = cdr(from)?alloc_cell(1):0;
	}
	return new_ast;
}

cell_p *reduce_ast(cell_p *root) {
	if (!root || !*root) return NULL;
	if (is_pair(*root)) simplify_ast(root);
	char atom[4] = "";
	strncpy(atom, to_atom(car(*root)), sizeof(atom) / sizeof(atom[0]));
	switch (atom[0]) {
		case 's' :
		case 'S' :
		if (is_exist(*root, 3)) {
			cell_p x = take(*root, 1);
			cell_p y = take(*root, 2);
			cell_p z1 = take(*root, 3);
			cell_p z2 = take(*root, 3);
			if (is_pair(car(z1))) {
				//z copy suru
				z2 = copy_ast(z1);
			}
			cell_p w = take(*root, 4);
			*root = x;
							cdr(x) = alloc_cell(1);
						car(cdr(x)) = car(z1);
						cdr(cdr(x)) = alloc_cell(1);
					car(cdr(cdr(x))) = alloc_cell(1);
				car(car(cdr(cdr(x)))) = car(y);
				cdr(car(cdr(cdr(x)))) = alloc_cell(1);
			car(cdr(car(cdr(cdr(x))))) = car(z2);
			cdr(cdr(car(cdr(cdr(x))))) = NULL;
					cdr(cdr(cdr(x))) = w;
			return root;
		}
		break;

		case 'k' :
		case 'K' :
		if (is_exist(*root, 2)) {
			cell_p y = take(*root, 2);
			*root = cdr(*root);
			cdr(*root) = cdr(y);
			return root;
		}
		break;

		case 'i' :
		case 'I' :
		if (is_exist(*root, 1)) {
			*root = cdr(*root);
			return root;
		}
		break;

		default :
		//printf("atom = %s\n", atom);
		return cdr(*root) && is_pair(car(cdr(*root)))?reduce_ast(&car(cdr(*root))):NULL;
	}
	return NULL;
}

int main (void) {
	str_with_len str = {0, 0, NULL};
	for(;read_from(stdin, &str););
	//printf("%d:%d\n%s",str.length,str.size,str.str);
	//cell_t root = {NULL, NULL};
	init_cell_heap();
	//parse_ski(ast_root = alloc_cell(1), &(char const *){str.str}, "\0");
	make_ski_ast(ast_root = alloc_cell(1), &(char const *){str.str}, "\0");
	simplify_ast(&ast_root);
	//show_ast(ast_root, 0);
	//alloc_cell(0);
	//return 0;
	for (; reduce_ast(&ast_root); ) {
		show_ast(ast_root, 0);
		puts("");
		simplify_ast(&ast_root);
		show_ast(ast_root, 0);
		puts("");
	}
	alloc_cell(0);
	return 0;
}
