#include "state.h"

#include <errno.h>
#include "log.h"
#include "thread_var.h"
#include "object/certificate.h"

/**
 * The current state of the validation cycle.
 *
 * It is one of the core objects in this project. Every time a trust anchor
 * triggers a validation cycle, the validator creates one of these objects and
 * uses it to traverse the tree and keep track of validated data.
 */
struct validation {
	struct tal *tal;

	/** https://www.openssl.org/docs/man1.1.1/man3/X509_STORE_load_locations.html */
	X509_STORE *store;

	/** Certificates we've already validated. */
	STACK_OF(X509) *trusted;
	/**
	 * The resources owned by the certificates from @trusted.
	 *
	 * (One for each certificate; these two stacks should practically always
	 * have the same size. The reason why I don't combine them is because
	 * libcrypto's validation function wants the stack of X509 and I'm not
	 * creating it over and over again.)
	 *
	 * (This is a SLIST and not a STACK_OF because the OpenSSL stack
	 * implementation is different than the LibreSSL one, and the latter is
	 * seemingly not intended to be used outside of its library.)
	 */
	struct restack *rsrcs;

	/* Did the TAL's public key match the root certificate's public key? */
	enum pubkey_state pubkey_state;
};

/*
 * It appears that this function is called by LibreSSL whenever it finds an
 * error while validating.
 * It is expected to return "okay" status: Nonzero if the error should be
 * ignored, zero if the error is grounds to abort the validation.
 *
 * Note to myself: During my tests, this function was called in
 * X509_verify_cert(ctx) -> check_chain_extensions(0, ctx),
 * and then twice again in
 * X509_verify_cert(ctx) -> internal_verify(1, ctx).
 *
 * Regarding the ok argument: I'm not 100% sure that I get it; I don't
 * understand why this function would be called with ok = 1.
 * http://openssl.cs.utah.edu/docs/crypto/X509_STORE_CTX_set_verify_cb.html
 * The logic I implemented is the same as the second example: Always ignore the
 * error that's troubling the library, otherwise try to be as unintrusive as
 * possible.
 */
static int
cb(int ok, X509_STORE_CTX *ctx)
{
	int error;

	/*
	 * We need to handle two new critical extensions (IP Resources and ASN
	 * Resources), so unknown critical extensions are fine as far as
	 * LibreSSL is concerned.
	 * Unfortunately, LibreSSL has no way of telling us *which* is the
	 * unknown critical extension, but since RPKI defines its own set of
	 * valid extensions, we'll have to figure it out later anyway.
	 */
	error = X509_STORE_CTX_get_error(ctx);
	return (error == X509_V_ERR_UNHANDLED_CRITICAL_EXTENSION) ? 1 : ok;
}

/** Creates a struct validation, puts it in thread local, and returns it. */
int
validation_prepare(struct validation **out, struct tal *tal)
{
	struct validation *result;
	int error;

	result = malloc(sizeof(struct validation));
	if (!result)
		return -ENOMEM;

	error = state_store(result);
	if (error)
		goto abort1;

	result->tal = tal;

	result->store = X509_STORE_new();
	if (!result->store) {
		error = crypto_err("X509_STORE_new() returned NULL");
		goto abort1;
	}

	X509_STORE_set_verify_cb(result->store, cb);

	result->trusted = sk_X509_new_null();
	if (result->trusted == NULL) {
		error = crypto_err("sk_X509_new_null() returned NULL");
		goto abort2;
	}

	result->rsrcs = restack_create();
	if (!result->rsrcs) {
		error = -ENOMEM;
		goto abort3;
	}

	result->pubkey_state = PKS_UNTESTED;

	*out = result;
	return 0;

abort3:
	sk_X509_pop_free(result->trusted, X509_free);
abort2:
	X509_STORE_free(result->store);
abort1:
	free(result);
	return error;
}

void
validation_destroy(struct validation *state)
{
	int cert_num;

	cert_num = sk_X509_num(state->trusted);
	if (cert_num != 0) {
		pr_err("Error: validation state has %d certificates. (0 expected)",
		    cert_num);
	}

	restack_destroy(state->rsrcs);
	sk_X509_pop_free(state->trusted, X509_free);
	X509_STORE_free(state->store);
	free(state);
}

struct tal *
validation_tal(struct validation *state)
{
	return state->tal;
}

X509_STORE *
validation_store(struct validation *state)
{
	return state->store;
}

STACK_OF(X509) *
validation_certs(struct validation *state)
{
	return state->trusted;
}

struct restack *
validation_resources(struct validation *state)
{
	return state->rsrcs;
}

void validation_pubkey_valid(struct validation *state)
{
	state->pubkey_state = PKS_VALID;
}

void validation_pubkey_invalid(struct validation *state)
{
	state->pubkey_state = PKS_INVALID;
}

enum pubkey_state validation_pubkey_state(struct validation *state)
{
	return state->pubkey_state;
}

int
validation_push_cert(struct validation *state, X509 *cert, bool is_ta)
{
	struct resources *resources;
	int ok;
	int error;

	resources = resources_create();
	if (resources == NULL)
		return -ENOMEM;

	error = certificate_get_resources(cert, resources);
	if (error)
		goto fail;

	/*
	 * rfc7730#section-2.2
	 * "The INR extension(s) of this trust anchor MUST contain a non-empty
	 * set of number resources."
	 * The "It MUST NOT use the "inherit" form of the INR extension(s)"
	 * part is already handled in certificate_get_resources().
	 */
	if (is_ta && resources_empty(resources))
		return pr_err("Trust Anchor certificate does not define any number resources.");

	ok = sk_X509_push(state->trusted, cert);
	if (ok <= 0) {
		error = crypto_err(
		    "Couldn't add certificate to trusted stack: %d", ok);
		goto fail;
	}

	restack_push(state->rsrcs, resources);
	return 0;

fail:
	resources_destroy(resources);
	return error;
}

int
validation_pop_cert(struct validation *state)
{
	struct resources *resources;

	if (sk_X509_pop(state->trusted) == NULL)
		return pr_crit("Attempted to pop empty certificate stack");

	resources = restack_pop(state->rsrcs);
	if (resources == NULL)
		return pr_crit("Attempted to pop empty resource stack");
	resources_destroy(resources);

	return 0;
}

X509 *
validation_peek_cert(struct validation *state)
{
	return sk_X509_value(state->trusted, sk_X509_num(state->trusted) - 1);
}

struct resources *
validation_peek_resource(struct validation *state)
{
	return restack_peek(state->rsrcs);
}