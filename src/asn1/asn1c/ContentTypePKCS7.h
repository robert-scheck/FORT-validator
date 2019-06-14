/*
 * Generated by asn1c-0.9.29 (http://lionet.info/asn1c)
 * From ASN.1 module "CryptographicMessageSyntax2004"
 * 	found in "rfc5652-12.1.asn1"
 * 	`asn1c -Werror -fcompound-names -fwide-types -D asn1/asn1c -no-gen-PER -no-gen-example`
 */

#ifndef	_ContentTypePKCS7_H_
#define	_ContentTypePKCS7_H_


#include "asn1/asn1c/asn_application.h"

/* Including external dependencies */
#include "asn1/asn1c/OCTET_STRING.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ContentTypePKCS7 */
typedef OCTET_STRING_t	 ContentTypePKCS7_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_ContentTypePKCS7;
asn_struct_free_f ContentTypePKCS7_free;
asn_struct_print_f ContentTypePKCS7_print;
asn_constr_check_f ContentTypePKCS7_constraint;
ber_type_decoder_f ContentTypePKCS7_decode_ber;
der_type_encoder_f ContentTypePKCS7_encode_der;
xer_type_decoder_f ContentTypePKCS7_decode_xer;
xer_type_encoder_f ContentTypePKCS7_encode_xer;
oer_type_decoder_f ContentTypePKCS7_decode_oer;
oer_type_encoder_f ContentTypePKCS7_encode_oer;

#ifdef __cplusplus
}
#endif

#endif	/* _ContentTypePKCS7_H_ */
#include "asn1/asn1c/asn_internal.h"