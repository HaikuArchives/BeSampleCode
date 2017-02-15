/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <TranslationKit.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <File.h>
#include <Application.h>


BTranslatorRoster * r;

static void
list_translators()
{
	translator_id * list = NULL;
	int32 count = 0;

	status_t err = r->GetAllTranslators(&list, &count);
	if (err < B_OK) {
		goto err_1;
	}
	for (int tix=0; tix<count; tix++) {
		const char * name;
		const char * info;
		int32 version;
		if ((err = r->GetTranslatorInfo(list[tix], &name, &info, &version)) < B_OK) {
			goto err_2;
		}
		printf("Translator %ld \"%s\" version %.2f\n", list[tix], name, 
			(float)version/100.0);
		printf("info='%s'\n", info);
		int32 num_input = 0;
		const translation_format * input_formats = NULL;
		if ((err = r->GetInputFormats(list[tix], &input_formats, &num_input)) < B_OK) {
			goto err_2;
		}
		for (int iix=0; iix<num_input; iix++) {
			/* only print formats that aren't the "base" format for the group */
			if (input_formats[iix].type != input_formats[iix].group) {
				uint32 t = input_formats[iix].type;
				uint32 g = input_formats[iix].group;
				printf("input='%c%c%c%c' group='%c%c%c%c' name='%s' MIME='%s' quality=%g capability=%g\n",
					(char)t>>24, (char)t>>16, (char)t>>8, (char)t,
					(char)g>>24, (char)g>>16, (char)g>>8, (char)g,
					input_formats[iix].name, input_formats[iix].MIME,
					input_formats[iix].quality, input_formats[iix].capability);
			}
		}
		int32 num_output = 0;
		const translation_format * output_formats = NULL;
		if ((err = r->GetOutputFormats(list[tix], &output_formats, &num_output)) < B_OK) {
			goto err_2;
		}
		for (int oix=0; oix<num_output; oix++) {
			/* only print formats that aren't the "base" format for the group */
			if (output_formats[oix].type != output_formats[oix].group) {
				uint32 t = output_formats[oix].type;
				uint32 g = output_formats[oix].group;
				printf("output='%c%c%c%c' group='%c%c%c%c' name='%s' MIME='%s' quality=%g capability=%g\n",
					(char)t>>24, (char)t>>16, (char)t>>8, (char)t,
					(char)g>>24, (char)g>>16, (char)g>>8, (char)g,
					output_formats[oix].name, output_formats[oix].MIME,
					output_formats[oix].quality, output_formats[oix].capability);
			}
		}
		printf("\n");
	}
	delete[] list;
	exit(0);

err_2:
	delete[] list;
err_1:
	fprintf(stderr, "%s [%lx]\n", strerror(err), err);
	exit(1);
}


int
main(
	int argc,
	char * argv[])
{
	char * outfile = NULL;
	type_code type = 0;
	int ndone = 0;
	BApplication app("application/x-vnd.hplus-translate");

	if (argc < 2) {
		fprintf(stderr, "usage: translate { -l } | [ -t type ] [ -o file ] file ...\n");
		fprintf(stderr, "note: Translate will not cross-translate type A to type B directly.\n");
		fprintf(stderr, "      You will have to go through the 'standard' format first, which\n");
		fprintf(stderr, "      is the default without -t. To translate from TGA to BMP, do:\n");
		fprintf(stderr, "translate -o foo.bits foo.tga ; translate -t \"BMP \" -o foo.bmp foo.bits\n");
		exit(1);
	}
	r = BTranslatorRoster::Default();
	if (!r) {
		fprintf(stderr, "error: there is no translator roster!\n");
		exit(1);
	}
	for (int ix=1; ix<argc; ix++) {
		if (!strcmp(argv[ix], "-l")) {
			list_translators();
		}
		else if (!strcmp(argv[ix], "-t")) {
			ix++;
			if (!argv[ix]) {
				fprintf(stderr, "error: -t requires argument\n");
				exit(1);
			}
			if (strlen(argv[ix]) != 4) {
				char * end;
				if ((argv[ix][0] == '0') && (argv[ix][1] == 'x')) {
					type = strtol(&argv[ix][2], &end, 16);
				}
				else {
					type = strtol(argv[ix], &end, 10);
				}
				if (*end != 0) {
					fprintf(stderr, "error: -t argument should be 4-char type code or integer (possibly hex).\n");
					exit(1);
				}
			}
			type = (((unsigned char)argv[ix][0])<<24) |
				(((unsigned char)argv[ix][1])<<16) |
				(((unsigned char)argv[ix][2])<<8)|
				(((unsigned char)argv[ix][3]));
		}
		else if (!strcmp(argv[ix], "-o")) {
			ix++;
			if (!argv[ix]) {
				fprintf(stderr, "error: -o requires argument\n");
				exit(1);
			}
			outfile = argv[ix];
		}
		else {
			char outpath[1024];
			if (outfile == NULL) {
				strcpy(outpath, argv[ix]);
				strcat(outpath, ".out");
				outfile = outpath;
			}
			BFile input, output;
			if (input.SetTo(argv[ix], O_RDONLY)) {
				fprintf(stderr, "error: can't read %s\n", argv[ix]);
				exit(1);
			}
			if (output.SetTo(outfile, O_RDWR | O_CREAT | O_TRUNC)) {
				fprintf(stderr, "error: can't create %s\n", outfile);
				exit(1);
			}
			fprintf(stdout, "%s: ", argv[ix]);
			fflush(stdout);
			status_t err = r->Translate(&input, NULL, NULL, &output, type);
			if (err == B_OK) {
				fprintf(stdout, "ok\n");
				ndone++;
			}
			else {
				fprintf(stdout, "%s [%lx]\n", strerror(err), err);
			}
			outfile = NULL;
		}
	}
	if (!ndone) {
		fprintf(stderr, "warning: no files translated.\n");
	}
	return 0;
}
