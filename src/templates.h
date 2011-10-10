/*
 * This file is part of Opentube - Open video hosting engine
 *
 * Copyright (C) 2011 - Xpast; http://xpast.me/; <vvladxx@gmail.com>
 *
 * Opentube is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Opentube is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Opentube.  If not, see <http://www.gnu.org/licenses/>.
 */

#define IS_VALID_TPL_VARIABLE_CHARACTER(X) (((X) >= 'a' && (X) <= 'z') || ((X) >= 'A' && (X) <= 'Z') || ((X) >= '0' && (X) <= '9') || (X) == '-' || (X) == '_' || (X) == ' ')

typedef void * template_t;

typedef void * template_context_t;

template_context_t tpl_context_create (void);

void tpl_context_destroy (template_context_t ctx);

template_context_t tpl_block_context_create (const char * name);

void tpl_block_context_destroy (template_context_t ctx);

void tpl_set_global_var (const char * name, const char * value);

void tpl_set_global_var_static (const char * name, const char * value);

void tpl_set_var (template_context_t ctx, const char * name, const char * value);

void tpl_set_var_static (template_context_t ctx, const char * name, const char * value);

template_t tpl_compile (const char * file);

void tpl_complete (template_t tpl, template_context_t ctx, template_context_t * block_ctx_vector, size_t vector_size, buf_t * out);

void tpl_init (void);

void tpl_destroy (void);
