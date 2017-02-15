/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

/* addon.cpp */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <Application.h>
#include <Entry.h>
#include <Path.h>
#include <List.h>
#include <Directory.h>
#include <Roster.h>

#include "addon.h"

#if __INTEL__
#define COMPILER_NAME			"gcc"
#define COMPILER_ADDON_OPTION	"-nostart"
#define COMPILER_OPTIMIZATION	"-O3"
#define COMPILER_EXTRA_CRAP		""
#elif __POWERPC__
#define COMPILER_NAME			"mwcc"
#define COMPILER_ADDON_OPTION	"-export whack_frame -export whack_expression -xms"
#define COMPILER_OPTIMIZATION	"-O7"
#define COMPILER_EXTRA_CRAP	"/boot/develop/lib/ppc/glue-noinit.a " \
							"/boot/develop/lib/ppc/init_term_dyn.o " \
							"/boot/develop/lib/ppc/start_dyn.o " \
							"-lroot"
#else
#error Running on unsupported processors is unsupported.
#endif

static int
find_template_file(char *buffer)
{
	app_info ai;
	BPath p;
	
	be_app->GetAppInfo(&ai);
	BEntry e(&ai.ref);
	e.GetPath(&p);
	p.GetParent(&p);
	p.Append("template");

	strcpy(buffer, p.Path());
	
	return 1;
}

static int32 addons_made = 0;

whack_addon *
build_addon(const char *expression)
{
	FILE *fp, *tfp;
	char tfile[B_PATH_NAME_LENGTH];
	char srcfile[64], outfile[64];
	char buffer[1024];
	whack_addon *a;
		
	if (addons_made++ % CLEANING_FREQUENCY == 1)
		clean_temporary_files();
		
	/* XXX should be caching this */
	if (find_template_file(tfile) == 0)
		return NULL;

	if ((tfp = fopen(tfile, "r")) == NULL)
		return NULL;
		
	/* open a temporary source file, define the user's expression,
	 * and tack the template file onto the end of it.
	 */
	strcpy(outfile, "/tmp/whackXXXXXX");
	mktemp(outfile);
	strcpy(srcfile, outfile);
	strcat(srcfile, ".cpp");
	
	if ((fp = fopen(srcfile, "w")) == NULL) {
		fclose(tfp);
		return NULL;
	}

	fprintf(fp, "#define __EXPRESSION__	(%s)\n", expression);
	fprintf(fp, "#define __EXPRESSION_STR__ \"%s\"\n", expression);
	
	while (fgets(buffer, sizeof(buffer), tfp))
		fputs(buffer, fp);
	
	fclose(fp);
	fclose(tfp);
	
	/* invoke the compiler */
	sprintf(buffer, "%s %s %s -o %s %s %s",
			COMPILER_NAME, COMPILER_OPTIMIZATION, COMPILER_ADDON_OPTION,
			outfile, srcfile, COMPILER_EXTRA_CRAP);

	if (system(buffer) != 0) {
		printf("couldn't execute compiler\n");
		goto bail;
	}

	/* load the add-on it produced, find the symbol, populate the
	 * whack_addon, and give it back.
	 */
	if ((a = (whack_addon *)malloc(sizeof(whack_addon))) == NULL) {
		printf("malloc failed!\n");
		goto bail;
	}

	a->image = load_add_on(outfile);
	if (a->image <= 0) {
		printf("couldn't load addon\n");
		goto bail2;
	}

	if (get_image_symbol(a->image, "whack_frame", B_SYMBOL_TYPE_TEXT,
						(void **)&(a->whacker)) != B_OK) {
		printf("couldn't find whack_frame in addon\n");
		goto bail3;
	}

	const char *(*get_exp)(void);
	if (get_image_symbol(a->image, "whack_expression", B_SYMBOL_TYPE_TEXT,
						(void **)&get_exp) != B_OK) {
		printf("couldn't find whack_expression in addon\n");
		goto bail3;
	}
	
	strcpy(a->expression, get_exp());
	strcpy(a->path, outfile);
	strcpy(a->name, DEFAULT_NAME);

	return a;
		
bail3:
	unload_add_on(a->image);		
bail2:
	free(a);
bail:
	return NULL;
}

void
destroy_addon(whack_addon *addon)
{
	/* sanity check */
	if (addon == NULL)
		return;
		
	unload_add_on(addon->image);
	free(addon);	
}

/* the name of this is dangerously close to load_add_on...oh well */
whack_addon *
load_addon(const char *path)
{
	whack_addon *a;
	char *cp;

	if ((a = (whack_addon *)malloc(sizeof(whack_addon))) == NULL)
		return NULL;
	
	strcpy(a->path, path);
	if ((cp = strrchr(a->path, '/')) == NULL)
		strcpy(a->name, DEFAULT_NAME);
	else
		strcpy(a->name, cp+1);
		
	if ((a->image = load_add_on(a->path)) <= 0)
		goto bail;

	if (get_image_symbol(a->image, "whack_frame", B_SYMBOL_TYPE_TEXT,
						(void **)&(a->whacker)) != B_OK)
		goto bail2;

	const char *(*hack)(void);
	if (get_image_symbol(a->image, "whack_expression", B_SYMBOL_TYPE_TEXT,
						(void **)&hack) != B_OK)
		goto bail2;

	strcpy(a->expression, hack());
									
	return a;
								
bail2:
	unload_add_on(a->image);	
bail:
	free(a);
	return NULL;
}

status_t
save_addon(whack_addon *addon, const char *directory)
{
	BDirectory old_d, new_d;
	BEntry entry;
	char file[B_FILE_NAME_LENGTH];
	status_t err;
			
	BPath p(addon->path);
	strcpy(file, p.Leaf());
	p.GetParent(&p);

	if ((err = old_d.SetTo(p.Path())) != B_OK)
		goto exit;

	if ((err = new_d.SetTo(directory)) != B_OK)
		goto exit;
	
	if ((err = old_d.FindEntry(file, &entry)) != B_OK)
		goto exit;

	if ((err = entry.MoveTo(&new_d)) != B_OK)
		goto exit;

	err = entry.Rename(addon->name);
	if (err == B_FILE_EXISTS)
		err = B_OK;
	if (err != B_OK)
		goto exit;

	new_d.GetEntry(&entry);
	entry.GetPath(&p);
			
	sprintf(addon->path, "%s/%s", p.Path(), addon->name);
	
exit:
	return err;
}

void
clean_temporary_files(void)
{
	BDirectory dir("/tmp");
	BEntry entry;
	char name[B_FILE_NAME_LENGTH];

	while (dir.GetNextEntry(&entry) == B_OK) {
		entry.GetName(name);
		if (!strncmp(name, "whack", strlen("whack")))
			entry.Remove();
	}
}
