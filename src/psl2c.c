/*
 * Copyright(c) 2014-2015 Tim Ruehsen
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * This file is part of libpsl.
 *
 * Precompile Public Suffix List into a C source file
 *
 * Changelog
 * 22.03.2014  Tim Ruehsen  created
 *
 */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#ifdef HAVE_ALLOCA_H
#	include <alloca.h>
#endif

#if defined(BUILTIN_GENERATOR_LIBICU) || defined(BUILTIN_GENERATOR_LIBIDN2) || defined(BUILTIN_GENERATOR_LIBIDN)
#	define _GENERATE_BUILTIN_DATA
#endif

#include <libpsl.h>

/* here we include the library source code to have access to internal functions and data structures */
#define _LIBPSL_INCLUDED_BY_PSL2C
#	include "psl.c"
#undef _LIBPSL_INCLUDED_BY_PSL2C

#ifdef _GENERATE_BUILTIN_DATA

#if 0
static int _check_psl(const psl_ctx_t *psl)
{
	int it, pos, err = 0;

	/* check if plain suffix also appears in exceptions */
	for (it = 0; it < psl->suffixes->cur; it++) {
		_psl_entry_t *e = _vector_get(psl->suffixes, it);

		if (!e->wildcard && _vector_find(psl->suffix_exceptions, e) >= 0) {
			fprintf(stderr, "Found entry '%s' also in exceptions\n", e->label);
			err = 1;
		}
	}

	/* check if exception also appears in suffix list as plain entry */
	for (it = 0; it < psl->suffix_exceptions->cur; it++) {
		_psl_entry_t *e2, *e = _vector_get(psl->suffix_exceptions, it);

		if ((e2 = _vector_get(psl->suffixes, pos = _vector_find(psl->suffixes, e)))) {
			if (!e2->wildcard) {
				fprintf(stderr, "Found exception '!%s' also as suffix\n", e->label);
				err = 1;
			}
			/* Two same domains in a row are allowed: wildcard and non-wildcard.
			 * Binary search find either of them, so also check previous and next entry. */
			else if (pos > 0 && _suffix_compare(e, e2 = _vector_get(psl->suffixes, pos - 1)) == 0 && !e2->wildcard) {
				fprintf(stderr, "Found exception '!%s' also as suffix\n", e->label);
				err = 1;
			}
			else if (pos < psl->suffixes->cur - 1 && _suffix_compare(e, e2 = _vector_get(psl->suffixes, pos + 1)) == 0 && !e2->wildcard) {
				fprintf(stderr, "Found exception '!%s' also as suffix\n", e->label);
				err = 1;
			}
		}
	}

	/* check if non-wildcard entry is already covered by wildcard entry */
	for (it = 0; it < psl->suffixes->cur; it++) {
		const char *p;
		_psl_entry_t *e = _vector_get(psl->suffixes, it);

		if (e->nlabels > 1 && !e->wildcard && (p = strchr(e->label, '.'))) {
			_psl_entry_t *e2, *e3, suffix;

			suffix.label = p + 1;
			suffix.length = strlen(p + 1);
			suffix.nlabels = e->nlabels - 1;

			e2 = _vector_get(psl->suffixes, pos = _vector_find(psl->suffixes, &suffix));

			if (e2) {
				if (e2->wildcard) {
					fprintf(stderr, "Found superfluous '%s' already covered by '*.%s'\n", e->label, e2->label);
					err = 1;
				}
				/* Two same domains in a row are allowed: wildcard and non-wildcard.
				* Binary search find either of them, so also check previous and next entry. */
				else if (pos > 0 && _suffix_compare(e2, e3 = _vector_get(psl->suffixes, pos - 1)) == 0 && e3->wildcard) {
					fprintf(stderr, "Found superfluous '%s' already covered by '*.%s'\n", e->label, e2->label);
					err = 1;
				}
				else if (pos < psl->suffixes->cur - 1 && _suffix_compare(e2, e3 = _vector_get(psl->suffixes, pos + 1)) == 0 && e3->wildcard) {
					fprintf(stderr, "Found superfluous '%s' already covered by '*.%s'\n", e->label, e2->label);
					err = 1;
				}
			}
		}
	}

	return err;
}
#endif

static void _print_psl_entries_dafsa(FILE *fpout, const _psl_vector_t *v)
{
	FILE *fp;
	int it;

#ifdef BUILTIN_GENERATOR_LIBICU
	do {
		UVersionInfo version_info;
		char version[U_MAX_VERSION_STRING_LENGTH];

		u_getVersion(version_info);
		u_versionToString(version_info, version);
		fprintf(fpout, "/* automatically generated by psl2c (punycode generated with libicu/%s) */\n", version);
	} while (0);
#elif defined(BUILTIN_GENERATOR_LIBIDN2)
	fprintf(fpout, "/* automatically generated by psl2c (punycode generated with libidn2/%s) */\n", idn2_check_version(NULL));
#elif defined(BUILTIN_GENERATOR_LIBIDN)
	fprintf(fpout, "/* automatically generated by psl2c (punycode generated with libidn/%s) */\n", stringprep_check_version(NULL));
#else
	fprintf(fpout, "/* automatically generated by psl2c (punycode generated internally) */\n");
#endif

	if ((fp = fopen("in.tmp", "w"))) {
		for (it = 0; it < v->cur; it++) {
			_psl_entry_t *e = _vector_get(v, it);
			unsigned char *s = (unsigned char *)e->label_buf;

			/* search for non-ASCII label and skip it */
			while (*s && *s < 128) s++;
			if (*s) continue;

			fprintf(fp, "%s, %X\n", e->label_buf, (int) (e->flags & 0x0F));
		}

		fclose(fp);
	}

	if ((it = system(MAKE_DAFSA " in.tmp out.tmp")))
		fprintf(stderr, "Failed to execute " MAKE_DAFSA "\n");

	if ((fp = fopen("out.tmp", "r"))) {
		char buf[256];

		while (fgets(buf, sizeof(buf), fp))
			fputs(buf, fpout);

		fclose(fp);
	}

	unlink("in.tmp");
	unlink("out.tmp");
}
#endif /* _GENERATE_BUILTIN_DATA */

static int _print_psl_entries_dafsa_binary(const char *fname, const _psl_vector_t *v)
{
	FILE *fp;
	int ret = 0, it, rc;
	char cmd[256];

	if ((fp = fopen("in.tmp", "w"))) {
		for (it = 0; it < v->cur; it++) {
			_psl_entry_t *e = _vector_get(v, it);
			unsigned char *s = (unsigned char *)e->label_buf;

			/* search for non-ASCII label and skip it */
			while (*s && *s < 128) s++;
			if (*s) continue;

			fprintf(fp, "%s, %X\n", e->label_buf, (int) (e->flags & 0x0F));
		}

		fclose(fp);
	} else {
		fprintf(stderr, "Failed to write open 'in.tmp'\n");
		return 3;
	}

	snprintf(cmd, sizeof(cmd), MAKE_DAFSA " --binary in.tmp %s", fname);
	if ((rc = system(cmd))) {
		fprintf(stderr, "Failed to execute '%s' (%d)\n", cmd, rc);
		ret = 2;
	}

	unlink("in.tmp");
	return ret;
}

static void usage(void)
{
	fprintf(stderr, "Usage: psl2c [--binary] <infile> <outfile>\n");
	fprintf(stderr, "  <infile>  is the 'public_suffix_list.dat', lowercase UTF-8 encoded\n");
	fprintf(stderr, "  <outfile> is the the filename to be generated from <infile>\n");
	fprintf(stderr, "  --binary  Generate binary DAFSA output (default: C code for psl.c)\n");
	exit(1);
}

int main(int argc, const char **argv)
{
	FILE *fpout;
	psl_ctx_t *psl;
	int ret = 0, argpos = 1, binary = 0;

	if (argc < 3)
		usage();

	if (strcmp(argv[argpos], "--binary") == 0) {
		argpos++;
		binary = 1;
	}

	if (argc - argpos != 2)
		usage();

	if (binary) {
		if (!(psl = psl_load_file(argv[argpos])))
			return 2;

		ret = _print_psl_entries_dafsa_binary(argv[argpos + 1], psl->suffixes);

		psl_free(psl);
		return ret;
	}

#ifdef _GENERATE_BUILTIN_DATA
	if (!(psl = psl_load_file(argv[argpos])))
		return 2;

	/* look for ambiguous or double entries */
/*	if (_check_psl(psl)) {
		psl_free(psl);
		return 5;
	}
*/
	if ((fpout = fopen(argv[argpos + 1], "w"))) {
		FILE *pp;
		struct stat st;
		size_t cmdsize = 16 + strlen(argv[argpos]);
		char *cmd = alloca(cmdsize), checksum[64] = "";
		char *abs_srcfile;
		const char *source_date_epoch = NULL;

		_print_psl_entries_dafsa(fpout, psl->suffixes);

		snprintf(cmd, cmdsize, "sha1sum %s", argv[argpos]);
		if ((pp = popen(cmd, "r"))) {
			if (fscanf(pp, "%63[0-9a-zA-Z]", checksum) < 1)
				*checksum = 0;
			pclose(pp);
		}

		if (stat(argv[argpos], &st) != 0)
			st.st_mtime = 0;
		fprintf(fpout, "static time_t _psl_file_time = %lu;\n", st.st_mtime);

		if ((source_date_epoch = getenv("SOURCE_DATE_EPOCH")))
			fprintf(fpout, "static time_t _psl_compile_time = %lu;\n", atol(source_date_epoch));
		else
			fprintf(fpout, "static time_t _psl_compile_time = %lu;\n", time(NULL));
		fprintf(fpout, "static int _psl_nsuffixes = %d;\n", psl->nsuffixes);
		fprintf(fpout, "static int _psl_nexceptions = %d;\n", psl->nexceptions);
		fprintf(fpout, "static int _psl_nwildcards = %d;\n", psl->nwildcards);
		fprintf(fpout, "static const char _psl_sha1_checksum[] = \"%s\";\n", checksum);

		/* We need an absolute path here, else psl_builtin_outdated() won't work reliable */
		/* Caveat: symbolic links are resolved by realpath() */
		if ((abs_srcfile = realpath(argv[argpos], NULL))) {
			fprintf(fpout, "static const char _psl_filename[] = \"%s\";\n", abs_srcfile);
			free(abs_srcfile);
		} else
			fprintf(fpout, "static const char _psl_filename[] = \"%s\";\n", argv[argpos]);

		if (fclose(fpout) != 0)
			ret = 4;
	} else {
		fprintf(stderr, "Failed to write open '%s'\n", argv[argpos + 1]);
		ret = 3;
	}

	psl_free(psl);
#else
	if ((fpout = fopen(argv[argpos + 1], "w"))) {
		fprintf(fpout, "static const unsigned char kDafsa[1];\n");
		fprintf(fpout, "static time_t _psl_file_time;\n");
		fprintf(fpout, "static time_t _psl_compile_time;\n");
		fprintf(fpout, "static int _psl_nsuffixes = 0;\n");
		fprintf(fpout, "static int _psl_nexceptions = 0;\n");
		fprintf(fpout, "static int _psl_nwildcards = 0;\n");
		fprintf(fpout, "static const char _psl_sha1_checksum[] = \"\";\n");
		fprintf(fpout, "static const char _psl_filename[] = \"\";\n");

		if (fclose(fpout) != 0)
			ret = 4;
	} else {
		fprintf(stderr, "Failed to write open '%s'\n", argv[argpos + 1]);
		ret = 3;
	}
#endif /* GENERATE_BUILTIN_DATA */

	return ret;
}
