#ifndef SRC_SLURM_db_slurm_H_
#define SRC_SLURM_db_slurm_H_

#include <stdbool.h>
#include "rtr/db/vrp.h"

/* Flags to get data from structs */
#define SLURM_COM_FLAG_NONE		0x00
#define SLURM_COM_FLAG_ASN		0x01
#define SLURM_COM_FLAG_COMMENT		0x02

#define SLURM_PFX_FLAG_PREFIX		0x04
#define SLURM_PFX_FLAG_MAX_LENGTH	0x08

#define SLURM_BGPS_FLAG_SKI		0x04
#define SLURM_BGPS_FLAG_ROUTER_KEY	0x08

struct slurm_prefix {
	uint8_t		data_flag;
	struct vrp	vrp;
};

struct slurm_bgpsec {
	uint8_t		data_flag;
	uint32_t	asn;
	unsigned char	*ski;
	unsigned char	*router_public_key;
};

struct db_slurm;

typedef int (*prefix_foreach_cb)(struct slurm_prefix *, void *);
typedef int (*bgpsec_foreach_cb)(struct slurm_bgpsec *, void *);

int db_slurm_create(struct db_slurm **);

int db_slurm_add_prefix_filter(struct db_slurm *, struct slurm_prefix *, int);
int db_slurm_add_prefix_assertion(struct db_slurm *, struct slurm_prefix *,
    int);
int db_slurm_add_bgpsec_filter(struct db_slurm *, struct slurm_bgpsec *, int);
int db_slurm_add_bgpsec_assertion(struct db_slurm *, struct slurm_bgpsec *,
    int);

bool db_slurm_vrp_is_filtered(struct db_slurm *, struct vrp const *);
bool db_slurm_bgpsec_is_filtered(struct db_slurm *, struct router_key const *);

int db_slurm_foreach_filter_prefix(struct db_slurm *, prefix_foreach_cb,
    void *);
int db_slurm_foreach_filter_bgpsec(struct db_slurm *, bgpsec_foreach_cb,
    void *);
int db_slurm_foreach_assertion_prefix(struct db_slurm *, prefix_foreach_cb,
    void *);
int db_slurm_foreach_assertion_bgpsec(struct db_slurm *, bgpsec_foreach_cb,
    void *);

/* Set the last update to current datetime */
void db_slurm_update_time(struct db_slurm *);
/* Log the DB in human readable form at INFO level */
void db_slurm_log(struct db_slurm *);

void db_slurm_destroy(struct db_slurm *);

#endif /* SRC_SLURM_db_slurm_H_ */
