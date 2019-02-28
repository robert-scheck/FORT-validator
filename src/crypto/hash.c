#include "hash.h"

#include <errno.h>
#include <openssl/evp.h>
#include <sys/stat.h>

#include "file.h"
#include "log.h"
#include "asn1/oid.h"

int
hash_is_sha256(OBJECT_IDENTIFIER_t *oid, bool *result)
{
	static const OID sha_oid = OID_SHA256;
	struct oid_arcs arcs;
	int error;

	error = oid2arcs(oid, &arcs);
	if (error)
		return error;

	*result = ARCS_EQUAL_OIDS(&arcs, sha_oid);

	free_arcs(&arcs);
	return 0;
}

static int
get_md(char const *algorithm, EVP_MD const **result)
{
	EVP_MD const *md;

	md = EVP_get_digestbyname(algorithm);
	if (md == NULL) {
		printf("Unknown message digest %s\n", algorithm);
		return -EINVAL;
	}

	*result = md;
	return 0;
}

static bool
hash_matches(unsigned char const *expected, size_t expected_len,
    unsigned char const *actual, unsigned int actual_len)
{
	return (expected_len == actual_len)
	    && (memcmp(expected, actual, expected_len) == 0);
}

static int
hash_file(char const *algorithm, struct rpki_uri const *uri,
    unsigned char *result, unsigned int *result_len)
{
	EVP_MD const *md;
	FILE *file;
	struct stat stat;
	unsigned char *buffer;
	__blksize_t buffer_len;
	size_t consumed;
	EVP_MD_CTX *ctx;
	int error;

	error = get_md(algorithm, &md);
	if (error)
		return error;

	error = file_open(uri->local, &file, &stat);
	if (error)
		return error;

	buffer_len = stat.st_blksize;
	buffer = malloc(buffer_len);
	if (buffer == NULL) {
		error = pr_enomem();
		goto end1;
	}

	ctx = EVP_MD_CTX_new();
	if (ctx == NULL) {
		error = pr_enomem();
		goto end2;
	}

	if (!EVP_DigestInit_ex(ctx, md, NULL)) {
		error = crypto_err("EVP_DigestInit_ex() failed");
		goto end3;
	}

	do {
		consumed = fread(buffer, 1, buffer_len, file);
		error = ferror(file);
		if (error) {
			pr_errno(error,
			    "File reading error. Error message (apparently)");
			goto end3;
		}

		if (!EVP_DigestUpdate(ctx, buffer, consumed)) {
			error = crypto_err("EVP_DigestUpdate() failed");
			goto end3;
		}

	} while (!feof(file));

	if (!EVP_DigestFinal_ex(ctx, result, result_len))
		error = crypto_err("EVP_DigestFinal_ex() failed");

end3:
	EVP_MD_CTX_free(ctx);
end2:
	free(buffer);
end1:
	file_close(file);
	return error;
}

/**
 * Computes the hash of the file @uri, and compares it to @expected (The
 * "expected" hash). Returns 0 if no errors happened and the hashes match.
 */
int
hash_validate_file(char const *algorithm, struct rpki_uri const *uri,
    BIT_STRING_t const *expected)
{
	unsigned char actual[EVP_MAX_MD_SIZE];
	unsigned int actual_len;
	int error;

	if (expected->bits_unused != 0)
		return pr_err("Hash string has unused bits.");

	error = hash_file(algorithm, uri, actual, &actual_len);
	if (error)
		return error;

	if (!hash_matches(expected->buf, expected->size, actual, actual_len))
		return pr_err("File does not match its hash.");

	return 0;
}

static int
hash_buffer(char const *algorithm,
    unsigned char const *content, size_t content_len,
    unsigned char *hash, unsigned int *hash_len)
{
	EVP_MD const *md;
	EVP_MD_CTX *ctx;
	int error = 0;

	error = get_md(algorithm, &md);
	if (error)
		return error;

	ctx = EVP_MD_CTX_new();
	if (ctx == NULL)
		return pr_enomem();

	if (!EVP_DigestInit_ex(ctx, md, NULL)
	    || !EVP_DigestUpdate(ctx, content, content_len)
	    || !EVP_DigestFinal_ex(ctx, hash, hash_len)) {
		error = crypto_err("Buffer hashing failed");
	}

	EVP_MD_CTX_free(ctx);
	return error;
}

/*
 * Returns 0 if @data's hash is @expected. Returns error code otherwise.
 */
int
hash_validate(char const *algorithm,
    unsigned char const *expected, size_t expected_len,
    unsigned char const *data, size_t data_len)
{
	unsigned char actual[EVP_MAX_MD_SIZE];
	unsigned int actual_len;
	int error;

	error = hash_buffer(algorithm, data, data_len, actual, &actual_len);
	if (error)
		return error;

	return hash_matches(expected, expected_len, actual, actual_len)
	    ? 0
	    : -EINVAL;
}

int
hash_validate_octet_string(char const *algorithm,
    OCTET_STRING_t const *expected,
    OCTET_STRING_t const *data)
{
	return hash_validate(algorithm, expected->buf, expected->size,
	    data->buf, data->size);
}
