/*
 * Copyright (C) 2010-2011 Alex Smith
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/**
 * @file
 * @brief		Menu interface.
 */

#include <lib/string.h>
#include <lib/utility.h>

#include <assert.h>
#include <loader.h>
#include <memory.h>
#include <menu.h>
#include <time.h>
#include <ui.h>

/** Structure containing a menu entry. */
typedef struct menu_entry {
	ui_entry_t header;		/**< UI entry header. */
	list_t link;			/**< Link to menu entries list. */
	char *name;			/**< Name of the entry. */
	environ_t *env;			/**< Environment for the entry. */
} menu_entry_t;

/** List of menu entries. */
static LIST_DECLARE(menu_entries);

/** Selected menu entry. */
static menu_entry_t *selected_menu_entry = NULL;

/** Add a new menu entry.
 * @param args		Arguments to the command.
 * @param env		Environment to operate on.
 * @return		Whether successful. */
static bool config_cmd_entry(value_list_t *args, environ_t *env) {
	menu_entry_t *entry;

	assert(env == root_environ);

	if(args->count != 2 || args->values[0].type != VALUE_TYPE_STRING ||
	   args->values[1].type != VALUE_TYPE_COMMAND_LIST) {
		dprintf("config: entry: invalid arguments\n");
		return false;
	}

	entry = kmalloc(sizeof(menu_entry_t));
	list_init(&entry->link);
	entry->name = kstrdup(args->values[0].string);
	entry->env = environ_create();

	/* Execute the command list. */
	if(!command_list_exec(args->values[1].cmds, entry->env)) {
		//environ_destroy(entry->env);
		kfree(entry->name);
		kfree(entry);
		return false;
	}

	list_append(&menu_entries, &entry->link);
	return true;
}
DEFINE_COMMAND("entry", config_cmd_entry);

/** Find the default menu entry.
 * @return		Default entry. */
static menu_entry_t *menu_find_default(void) {
	menu_entry_t *entry;
	value_t *value;
	uint64_t i = 0;

	if((value = environ_lookup(root_environ, "default"))) {
		LIST_FOREACH(&menu_entries, iter) {
			entry = list_entry(iter, menu_entry_t, link);
			if(value->type == VALUE_TYPE_INTEGER) {
				if(i == value->integer) {
					return entry;
				}
			} else if(value->type == VALUE_TYPE_STRING) {
				if(strcmp(entry->name, value->string) == 0) {
					return entry;
				}
			}

			i++;
		}
	}

	/* No default entry found, return the first list entry. */
	return list_entry(menu_entries.next, menu_entry_t, link);
}

/** Check if the menu can be displayed.
 * @return		Whether the menu can be displayed. */
static bool menu_can_display(void) {
	value_t *value;

	if(!main_console) {
		return false;
	} else if((value = environ_lookup(root_environ, "hidden")) && value->boolean) {
		/* Menu hidden, wait half a second for Esc to be pressed. */
		spin(500000);
		while(main_console->check_key()) {
			if(main_console->get_key() == '\e') {
				return true;
			}
		}

		return false;
	} else {
		return true;
	}
}

/** Select a menu entry.
 * @param _entry	Entry that was selected.
 * @return		Always returns INPUT_CLOSE. */
static input_result_t menu_entry_select(ui_entry_t *_entry) {
	menu_entry_t *entry = (menu_entry_t *)_entry;
	selected_menu_entry = entry;
	return INPUT_CLOSE;
}

/** Configure a menu entry.
 * @param _entry	Entry that was selected.
 * @return		Always returns INPUT_RENDER. */
static input_result_t menu_entry_configure(ui_entry_t *_entry) {
	menu_entry_t *entry = (menu_entry_t *)_entry;
	loader_type_t *type = loader_type_get(entry->env);

	type->configure(entry->env);
	return INPUT_RENDER;
}

/** Show a debug log window.
 * @param _entry	Entry that was selected.
 * @return		Always returns INPUT_RENDER. */
static input_result_t menu_entry_debug(ui_entry_t *_entry) {
	ui_window_t *window;

	/* Create the debug log window. */
	window = ui_textview_create("Debug Log", debug_log);
	ui_window_display(window, 0);
	return INPUT_RENDER;
}

/** Actions for a menu entry. */
static ui_action_t menu_entry_actions[] = {
	{ "Boot", '\n', menu_entry_select },
	{ "Debug Log", CONSOLE_KEY_F2, menu_entry_debug },
};

/** Actions for a configurable menu entry. */
static ui_action_t configurable_menu_entry_actions[] = {
	{ "Boot", '\n', menu_entry_select },
	{ "Configure", CONSOLE_KEY_F1, menu_entry_configure },
	{ "Debug Log", CONSOLE_KEY_F2, menu_entry_debug },
};

/** Render a menu entry.
 * @param _entry	Entry to render. */
static void menu_entry_render(ui_entry_t *_entry) {
	menu_entry_t *entry = (menu_entry_t *)_entry;
	kprintf("%s", entry->name);
}

/** Menu entry UI entry type. */
static ui_entry_type_t menu_entry_type = {
	.actions = menu_entry_actions,
	.action_count = ARRAYSZ(menu_entry_actions),
	.render = menu_entry_render,
};

/** Configurable menu entry UI entry type. */
static ui_entry_type_t configurable_menu_entry_type = {
	.actions = configurable_menu_entry_actions,
	.action_count = ARRAYSZ(configurable_menu_entry_actions),
	.render = menu_entry_render,
};

/** Display the menu interface.
 * @return		Environment for the entry to boot. */
environ_t *menu_display(void) {
	menu_entry_t *entry;
	ui_window_t *window;
	int timeout = 0;
	value_t *value;

	/* If no menu entries are defined, assume that the top-level environment
	 * has been configured with something to boot. */
	if(list_empty(&menu_entries)) {
		return root_environ;
	}

	/* Find the default entry. */
	selected_menu_entry = menu_find_default();

	if(menu_can_display()) {
		/* Construct the menu. */
		window = ui_list_create("Boot Menu", false);
		LIST_FOREACH(&menu_entries, iter) {
			entry = list_entry(iter, menu_entry_t, link);

			/* If the entry's loader type has a configure function,
			 * use the configurable entry type. */
			if(loader_type_get(entry->env)->configure) {
				ui_entry_init(&entry->header, &configurable_menu_entry_type);
			} else {
				ui_entry_init(&entry->header, &menu_entry_type);
			}
			ui_list_insert(window, &entry->header, entry == selected_menu_entry);
		}

		/* Display it. The selected entry pointer will be updated. */
		if((value = environ_lookup(root_environ, "timeout")) && value->type == VALUE_TYPE_INTEGER) {
			timeout = value->integer;
		}
		ui_window_display(window, timeout);
	}

	dprintf("loader: booting menu entry '%s'\n", selected_menu_entry->name);
	return selected_menu_entry->env;
}
