/*
 * This file is part of Opentube - Open video hosting engine
 *
 * Copyright (C) 2011 - VladX; http://vladx.net/
 *
 * Opentube is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Opentube is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Opentube; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */

#include "common_functions.h"
#include "templates.h"
#include <pthread.h>
#include <sys/stat.h>
#include "libs/zlib.h"
#include "libs/pcre.h"

#define CALC_CHECKSUM(F, L) adler32(1L, (const uchar *) F, L)

enum tpl_block_types
{
	TPL_TYPE_TEXT,
	TPL_TYPE_VAR,
	TPL_TYPE_BLOCK,
	TPL_TYPE_TERM
};

struct tpl_block
{
	size_t content_offset;
	union
	{
		size_t content_length;
		uint32_t checksum;
	};
	uint32_t block_checksum;
	enum tpl_block_types type;
};

struct compiled_template
{
	template_t tpl;
	time_t mtime;
	uint32_t checksum;
};

struct assoc_variable
{
	const char * value;
	uint value_len;
	uint32_t checksum;
	bool allocated;
};

struct context
{
	struct assoc_variable * vars;
	size_t vars_len;
};

struct block_context
{
	struct context ctx;
	uint32_t checksum;
};

char * cur_template_dir = NULL;
static struct compiled_template * compiled_templates = NULL;
static uint compiled_templates_num = 0;
static const pcre * regexp_block_code;

static inline size_t calculate_vector_size (struct tpl_block * vector)
{
	size_t vector_size;
	struct tpl_block * v;
	
	for (vector_size = 1, v = vector; v->type != TPL_TYPE_TERM; v++, vector_size++);
	
	return vector_size;
}

static void adjust_offsets (struct tpl_block * vector, size_t add)
{
	size_t vector_size, i;
	
	vector_size = calculate_vector_size(vector);
	
	if (!add)
		add = sizeof(struct tpl_block) * vector_size;
	
	for (i = 0; i < vector_size - 1; i++)
		vector[i].content_offset += add;
}

static char * gather_into_single_file (const char * file, str_big_t * content)
{
	str_big_t include_content;
	size_t offset = 0;
	char * found, * end, * include_file, * temp;
	static const char cmd_include[] = "{include ";
	
	for (;;)
	{
		found = include_file = strstr(content->str + offset, cmd_include);
		
		if (found == NULL)
			break;
		
		include_file += sizeof(cmd_include) - 1;
		
		if (* include_file == '"')
			include_file++;
		
		end = strchr(include_file, '}');
		
		if (end == NULL)
			break;
		
		if (* (end - 1) == '"')
			end--;
		
		if (end - include_file < 0)
		{
			end++;
			include_file--;
		}
		
		* end = '\0';
		
		if (!load_file_contents(include_file, &include_content))
		{
			perr("Error in template file \"%s\": can't include \"%s\"", file, include_file);
			
			return NULL;
		}
		
		if (* (end + 1) == '}')
			end++;
		
		end++;
		
		temp = content->str;
		content->len = (content->len + include_content.len) - (end - found);
		content->str = allocator_malloc(content->len + 1);
		offset = found - temp;
		memcpy(content->str, temp, offset);
		memcpy(content->str + offset, include_content.str, include_content.len);
		memcpy(content->str + offset + include_content.len, end, strlen(end) + 1);
		allocator_free(temp);
		allocator_free(include_content.str);
	}
	
	return content->str;
}

static struct tpl_block * parse_variables (str_big_t * content)
{
	struct tpl_block * vector = NULL;
	size_t offset = 0, vector_size = 0;
	char * found, * end, * braceend;
	static const char cmd_var[] = "{$ ";
	
	for (;;)
	{
		found = strstr(content->str + offset, cmd_var);
		
		if (found == NULL)
			break;
		
		if (found != content->str + offset)
		{
			vector = allocator_realloc(vector, sizeof(struct tpl_block) * (++vector_size));
			vector[vector_size - 1].type = TPL_TYPE_TEXT;
			vector[vector_size - 1].content_offset = offset;
			vector[vector_size - 1].content_length = found - (content->str + offset);
		}
		
		found += sizeof(cmd_var) - 1;
		
		for (end = found; * end != '}'; end++)
			if (!IS_VALID_TPL_VARIABLE_CHARACTER(* end))
			{
				allocator_free(vector);
				err("Invalid character \"0x%x\" in variable name", * end);
				
				return NULL;
			}
		
		if (end == found)
		{
			allocator_free(vector);
			err("Empty variable name", * end);
			
			return NULL;
		}
		
		braceend = end + 1;
		
		if (* (end - 1) == ' ')
			end--;
		
		vector = allocator_realloc(vector, sizeof(struct tpl_block) * (++vector_size));
		vector[vector_size - 1].type = TPL_TYPE_VAR;
		vector[vector_size - 1].content_offset = found - content->str;
		vector[vector_size - 1].content_length = end - found;
		vector[vector_size - 1].checksum = CALC_CHECKSUM(found, end - found);
		
		offset = braceend - content->str;
	}
	
	vector_size += 2;
	vector = allocator_realloc(vector, sizeof(struct tpl_block) * vector_size);
	vector[vector_size - 2].type = TPL_TYPE_TEXT;
	vector[vector_size - 2].content_offset = offset;
	vector[vector_size - 2].content_length = content->len - offset;
	vector[vector_size - 1].type = TPL_TYPE_TERM;
	vector[vector_size - 1].content_offset = sizeof(struct tpl_block) * vector_size + content->len;
	vector[vector_size - 1].content_length = vector[vector_size - 1].content_offset;
	
	return vector;
}

static struct tpl_block * concat_vectors (struct tpl_block * v1, struct tpl_block * v2)
{
	struct tpl_block * res;
	size_t v1_size, v2_size;
	
	if (v1 == NULL && v2 == NULL)
		return NULL;
	
	if (v2 == NULL)
		return v1;
	
	if (v1 == NULL)
		return v2;
	
	v1_size = calculate_vector_size(v1) - 1;
	v2_size = calculate_vector_size(v2) - 1;
	
	res = allocator_malloc(sizeof(struct tpl_block) * (v1_size + v2_size + 1));
	res[v1_size + v2_size].type = TPL_TYPE_TERM;
	res[v1_size + v2_size].content_offset = 0;
	res[v1_size + v2_size].content_length = 0;
	memcpy(res, v1, sizeof(struct tpl_block) * v1_size);
	memcpy(res + v1_size, v2, sizeof(struct tpl_block) * v2_size);
	
	allocator_free(v1);
	allocator_free(v2);
	
	return res;
}

static struct tpl_block * process_blocks (str_big_t * content, struct tpl_block * vector, size_t vector_size, size_t i)
{
	struct tpl_block * cur = &(vector[i]);
	struct tpl_block * next;
	struct tpl_block * res_vector = NULL, * tmp_vector = NULL, * tmp_vector_upper;
	str_big_t block_content;
	size_t k, prev_offset = cur->content_offset;
	
	for (;;)
	{
		new_iter:
		
		i++;
		
		if (i >= vector_size)
			break;
		
		next = &(vector[i]);
		
		if (res_vector)
			for (k = 0; k < calculate_vector_size(res_vector) - 1; k++)
				if (res_vector[k].block_checksum == next->block_checksum)
					goto new_iter;
		
		if (cur->content_offset < next->content_offset && cur->content_offset + cur->content_length > next->content_offset + next->content_length)
			tmp_vector = process_blocks(content, vector, vector_size, i);
		else
			break;
		
		if (tmp_vector == NULL)
		{
			allocator_free(res_vector);
			
			return NULL;
		}
		
		block_content = * content;
		block_content.str += prev_offset;
		block_content.len = next->content_offset - prev_offset;
		
		if (block_content.str[block_content.len - 1] == '}')
		{
			while (block_content.str[block_content.len - 1] != '{')
				block_content.len--;
			
			block_content.len--;
		}
		
		if (block_content.len > 0)
		{
			tmp_vector_upper = parse_variables(&block_content);
			
			if (tmp_vector_upper == NULL)
			{
				allocator_free(res_vector);
				allocator_free(tmp_vector);
				
				return NULL;
			}
			
			if (prev_offset != 0)
				adjust_offsets(tmp_vector_upper, prev_offset);
			
			for (k = 0; k < calculate_vector_size(tmp_vector_upper) - 1; k++)
				tmp_vector_upper[k].block_checksum = cur->block_checksum;
			
			tmp_vector = concat_vectors(tmp_vector_upper, tmp_vector);
		}
		
		res_vector = concat_vectors(res_vector, tmp_vector);
		
		prev_offset = next->content_offset + next->content_length;
		
		if (content->str[prev_offset] == '{')
		{
			while (content->str[prev_offset] != '}')
				prev_offset++;
			
			prev_offset++;
		}
	}
	
	block_content = * content;
	block_content.str += prev_offset;
	block_content.len = (cur->content_offset + cur->content_length) - prev_offset;
	
	if (block_content.len > 0)
	{
		tmp_vector = parse_variables(&block_content);
		
		if (tmp_vector == NULL)
		{
			allocator_free(res_vector);
			allocator_free(tmp_vector);
			
			return NULL;
		}
		
		if (prev_offset != 0)
			adjust_offsets(tmp_vector, prev_offset);
		
		for (k = 0; k < calculate_vector_size(tmp_vector) - 1; k++)
			tmp_vector[k].block_checksum = cur->block_checksum;
		
		res_vector = concat_vectors(res_vector, tmp_vector);
	}
	
	return res_vector;
}

static template_t parse_blocks (str_big_t * content)
{
	struct tpl_block * vector = NULL, * result_vector = NULL;
	size_t offset = 0, vector_size = 0, i, content_length, content_offset;
	int ovector[15];
	int r;
	uint32_t checksum;
	
	vector = allocator_realloc(vector, sizeof(struct tpl_block) * (++vector_size));
	vector[0].type = TPL_TYPE_BLOCK;
	vector[0].content_offset = 0;
	vector[0].content_length = content->len;
	vector[0].block_checksum = 0;
	
	for (;;)
	{
		r = pcre_exec(regexp_block_code, NULL, content->str, content->len, offset, 0, ovector, ARRAY_LENGTH(ovector));
		
		if (r < 0)
			break;
		
		assert(r == 3);
		
		checksum = CALC_CHECKSUM(content->str + ovector[2], ovector[3] - ovector[2]);
		content_offset = ovector[4];
		content_length = ovector[5] - ovector[4];
		
		for (i = 0; i < vector_size; i++)
		{
			if (content_offset >= vector[i].content_offset && ovector[5] >= vector[i].content_offset + vector[i].content_length && content_offset <= vector[i].content_offset + vector[i].content_length)
			{
				allocator_free(vector);
				err("Block \"%.*s\" is overlapped by other block", ovector[3] - ovector[2], content->str + ovector[2]);
				
				return NULL;
			}
			
			if (vector[i].block_checksum == checksum)
			{
				allocator_free(vector);
				err("Duplicate block \"%.*s\"", ovector[3] - ovector[2], content->str + ovector[2]);
				
				return NULL;
			}
		}
		
		vector = allocator_realloc(vector, sizeof(struct tpl_block) * (++vector_size));
		vector[vector_size - 1].type = TPL_TYPE_BLOCK;
		vector[vector_size - 1].content_offset = content_offset;
		vector[vector_size - 1].content_length = content_length;
		vector[vector_size - 1].block_checksum = checksum;
		
		offset = content_offset;
	}
	
	result_vector = process_blocks(content, vector, vector_size, 0);
	
	if (result_vector)
	{
		vector_size = calculate_vector_size(result_vector);
		result_vector = allocator_realloc(result_vector, sizeof(struct tpl_block) * vector_size + content->len);
		memcpy(result_vector + vector_size, content->str, content->len);
		vector_size = 0;
	}
	
	allocator_free(vector);
	
	return result_vector;
}

template_t tpl_compile (const char * file)
{
	str_big_t content;
	umlong cwd;
	struct compiled_template * compiled_template = NULL;
	struct stat st;
	char * single;
	template_t ret = NULL;
	static pthread_mutex_t mutex[1] = {PTHREAD_MUTEX_INITIALIZER};
	uint i;
	
	for (i = 0; i < compiled_templates_num; i++)
		if (CALC_CHECKSUM(file, strlen(file)) == compiled_templates[i].checksum)
		{
			compiled_template = &(compiled_templates[i]);
			
			break;
		}
	
	if (config.tpl_cache_update == 0 && compiled_template)
		return compiled_template->tpl;
	
	memset(&content, 0, sizeof(content));
	pthread_mutex_lock(mutex);
	cwd = save_cwd();
	
	if (chdir(cur_template_dir) == -1)
	{
		perr("chdir(%s) failed", cur_template_dir);
		ret = NULL;
		goto _return_;
	}
	
	if (stat(file, &st) != -1 && config.tpl_cache_update == 1 && compiled_template && st.st_mtime <= compiled_template->mtime)
	{
		ret = compiled_template->tpl;
		goto _return_;
	}
	
	if (!load_file_contents(file, &content))
	{
		perr("Can't load \"%s\"", file);
		ret = NULL;
		goto _return_;
	}
	
	single = gather_into_single_file(file, &content);
	
	if (single == NULL)
	{
		ret = NULL;
		goto _return_;
	}
	
	ret = parse_blocks(&content);
	
	if (ret)
	{
		adjust_offsets(ret, 0);
		
		if (compiled_template == NULL)
		{
			compiled_templates = allocator_realloc(compiled_templates, ++compiled_templates_num);
			compiled_template = &(compiled_templates[compiled_templates_num - 1]);
		}
		else
			allocator_free(compiled_template->tpl);
		
		compiled_template->checksum = CALC_CHECKSUM(file, strlen(file));
		compiled_template->mtime = st.st_mtime;
		compiled_template->tpl = ret;
	}
	
	_return_:
	
	restore_cwd(cwd);
	pthread_mutex_unlock(mutex);
	allocator_free(content.str);
	
	return ret;
}

static struct assoc_variable * global_vars_vector = NULL;
static size_t global_vars_vector_size = 0;
static pthread_spinlock_t global_vars_spin[1];

static inline struct assoc_variable * get_global_var (uint32_t checksum)
{
	mlong i;
	
	for (i = global_vars_vector_size - 1; i >= 0; i--)
		if (checksum == global_vars_vector[i].checksum)
			return &(global_vars_vector[i]);
	
	return NULL;
}

void tpl_set_global_var (const char * name, const char * value)
{
	char * value_dup;
	struct assoc_variable * var;
	size_t name_len = strlen(name), value_len = strlen(value);
	uint32_t checksum = CALC_CHECKSUM(name, name_len);
	
	pthread_spin_lock(global_vars_spin);
	
	var = get_global_var(checksum);
	
	value_dup = allocator_malloc(value_len);
	memcpy(value_dup, value, value_len);
	
	if (var)
	{
		if (var->allocated)
			allocator_free(var->value);
		
		var->value = value_dup;
		var->value_len = value_len;
		var->allocated = true;
	}
	else
	{
		global_vars_vector = allocator_realloc(global_vars_vector, sizeof(struct assoc_variable) * (++global_vars_vector_size));
		global_vars_vector[global_vars_vector_size - 1].checksum = checksum;
		global_vars_vector[global_vars_vector_size - 1].value = value_dup;
		global_vars_vector[global_vars_vector_size - 1].value_len = value_len;
		global_vars_vector[global_vars_vector_size - 1].allocated = true;
	}
	
	pthread_spin_unlock(global_vars_spin);
}

/* Takes as parameters statically allocated strings. This frees us from having to allocate new memory for cloning. */
void tpl_set_global_var_static (const char * name, const char * value)
{
	struct assoc_variable * var;
	size_t name_len = strlen(name), value_len = strlen(value);
	uint32_t checksum = CALC_CHECKSUM(name, name_len);
	
	pthread_spin_lock(global_vars_spin);
	
	var = get_global_var(checksum);
	
	if (var)
	{
		if (var->allocated)
			allocator_free(var->value);
		
		var->value = value;
		var->value_len = value_len;
		var->allocated = false;
	}
	else
	{
		global_vars_vector = allocator_realloc(global_vars_vector, sizeof(struct assoc_variable) * (++global_vars_vector_size));
		global_vars_vector[global_vars_vector_size - 1].checksum = checksum;
		global_vars_vector[global_vars_vector_size - 1].value = value;
		global_vars_vector[global_vars_vector_size - 1].value_len = value_len;
		global_vars_vector[global_vars_vector_size - 1].allocated = false;
	}
	
	pthread_spin_unlock(global_vars_spin);
}

template_context_t tpl_context_create (void)
{
	return memset(allocator_malloc(sizeof(struct context)), 0, sizeof(struct context));
}

void tpl_context_destroy (template_context_t ctx_void)
{
	struct context * ctx = ctx_void;
	mlong i;
	
	for (i = ctx->vars_len - 1; i >= 0; i--)
		if (ctx->vars[i].allocated)
			allocator_free(ctx->vars[i].value);
	
	allocator_free(ctx->vars);
	allocator_free(ctx);
}

template_context_t tpl_block_context_create (const char * name)
{
	struct block_context * ctx = allocator_malloc(sizeof(struct block_context));
	
	memset(ctx, 0, sizeof(struct block_context));
	ctx->checksum = CALC_CHECKSUM(name, strlen(name));
	
	return ctx;
}

void tpl_block_context_destroy (template_context_t ctx_void)
{
	tpl_context_destroy(ctx_void);
}

static inline struct assoc_variable * get_var (struct context * ctx, uint32_t checksum)
{
	mlong i;
	
	for (i = ctx->vars_len - 1; i >= 0; i--)
		if (checksum == ctx->vars[i].checksum)
			return &(ctx->vars[i]);
	
	return NULL;
}

void tpl_set_var (template_context_t ctx_void, const char * name, const char * value)
{
	char * value_dup;
	size_t name_len = strlen(name), value_len = strlen(value);
	uint32_t checksum = CALC_CHECKSUM(name, name_len);
	struct context * ctx = ctx_void;
	struct assoc_variable * var = get_var(ctx, checksum);
	
	value_dup = allocator_malloc(value_len);
	memcpy(value_dup, value, value_len);
	
	if (var)
	{
		if (var->allocated)
			allocator_free(var->value);
		
		var->value = value_dup;
		var->value_len = value_len;
		var->allocated = true;
	}
	else
	{
		ctx->vars = allocator_realloc(ctx->vars, sizeof(struct assoc_variable) * (++(ctx->vars_len)));
		ctx->vars[ctx->vars_len - 1].checksum = checksum;
		ctx->vars[ctx->vars_len - 1].value = value_dup;
		ctx->vars[ctx->vars_len - 1].value_len = value_len;
		ctx->vars[ctx->vars_len - 1].allocated = true;
	}
}

void tpl_set_var_static (template_context_t ctx_void, const char * name, const char * value)
{
	size_t name_len = strlen(name), value_len = strlen(value);
	uint32_t checksum = CALC_CHECKSUM(name, name_len);
	struct context * ctx = ctx_void;
	struct assoc_variable * var = get_var(ctx, checksum);
	
	if (var)
	{
		if (var->allocated)
			allocator_free(var->value);
		
		var->value = value;
		var->value_len = value_len;
		var->allocated = false;
	}
	else
	{
		ctx->vars = allocator_realloc(ctx->vars, sizeof(struct assoc_variable) * (++(ctx->vars_len)));
		ctx->vars[ctx->vars_len - 1].checksum = checksum;
		ctx->vars[ctx->vars_len - 1].value = value;
		ctx->vars[ctx->vars_len - 1].value_len = value_len;
		ctx->vars[ctx->vars_len - 1].allocated = false;
	}
}

static inline void append_to_buffer (buf_t * buf, const void * ptr, uint len)
{
	buf_expand(buf, len);
	memcpy((uchar *) buf->data + (buf->cur_len - len), ptr, len);
}

void tpl_complete (template_t tpl, template_context_t ctx, template_context_t * block_ctx_vector, size_t vector_size, buf_t * out)
{
	struct tpl_block * vector;
	struct assoc_variable * var;
	template_context_t local_ctx;
	size_t i;
	
	for (vector = tpl; vector->type != TPL_TYPE_TERM; vector++)
	{
		local_ctx = NULL;
		
		if (vector->block_checksum)
		{
			for (i = 0; i < vector_size; i++)
				if (((struct block_context **) block_ctx_vector)[i]->checksum == vector->block_checksum)
				{
					local_ctx = block_ctx_vector[i];
					
					goto x;
				}
			
			continue;
		}
		
		x:
		
		switch (vector->type)
		{
			case TPL_TYPE_TEXT:
				append_to_buffer(out, (uchar *) tpl + vector->content_offset, vector->content_length);
				break;
			
			case TPL_TYPE_VAR:
				if (local_ctx)
				{
					var = get_var(local_ctx, vector->checksum);
					
					if (var)
					{
						append_to_buffer(out, var->value, var->value_len);
						
						break;
					}
				}
				
				if (ctx)
				{
					var = get_var(ctx, vector->checksum);
					
					if (var)
					{
						append_to_buffer(out, var->value, var->value_len);
						
						break;
					}
				}
				
				var = get_global_var(vector->checksum);
				
				if (var)
				{
					append_to_buffer(out, var->value, var->value_len);
					
					break;
				}
				
				break;
			
			case TPL_TYPE_BLOCK:
			case TPL_TYPE_TERM:
				break;
		}
	}
}

void tpl_init (void)
{
	if (cur_template_dir == NULL)
	{
		const char * errptr;
		int erroffset;
		
		cur_template_dir = (char *) allocator_malloc(config.data.len + config.template_name.len + 12);
		strcpy(cur_template_dir, (char *) config.data.str);
		strcat(cur_template_dir, "/templates/");
		strcat(cur_template_dir, (char *) config.template_name.str);
		if (!is_directory_exists(cur_template_dir, IOAR_R))
			eerr(-1, "Can't access to template directory \"%s\". Create it or check permissions.", cur_template_dir);
		debug_print_1("Template directory is \"%s\"", cur_template_dir);
		
		pthread_spin_init(global_vars_spin, PTHREAD_PROCESS_PRIVATE);
		
		pcre_malloc = ALLOCATOR_MALLOC_FN;
		pcre_free = ALLOCATOR_FREE_FN;
		
		regexp_block_code = pcre_compile("{block ([\\w _-]+) *}(.*?){end \\1 *}", PCRE_DOTALL | PCRE_MULTILINE | PCRE_UTF8, &errptr, &erroffset, NULL);
		
		if (regexp_block_code == NULL)
			eerr(-1, "PCRE: %s at offset %d", errptr, erroffset);
	}
}

void tpl_destroy (void)
{
	
}
