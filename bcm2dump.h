#ifndef RAM12P_H
#define RAM12P_H
#include <arpa/inet.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <time.h>
#include "profile.h"

// pointer to buffer, offset, length
#define CODE_DUMP_PARAMS_PBOL 0
// buffer, offset, length
#define CODE_DUMP_PARAMS_BOL (1 << 0)
// offset, buffer, length
#define CODE_DUMP_PARAMS_OBL (1 << 1)

int ser_open(const char *dev, unsigned speed);
bool ser_write(int fd, const char *str);
bool ser_read(int fd, char *buf, size_t len);
bool ser_iflush(int fd);
int ser_select(int fd, unsigned msec);

extern bool ser_debug;

bool bl_readw_begin(int fd);
bool bl_readw_end(int fd);
bool bl_readw(int fd, unsigned addr, char *word);
bool bl_read(int fd, unsigned addr, void *buf, size_t len);
bool bl_writew(int fd, unsigned addr, const char *word);
bool bl_write(int fd, unsigned addr, const void *buf, size_t len);
bool bl_jump(int fd, unsigned addr);
bool bl_menu_wait(int fd, bool write);

struct progress {
	unsigned min;
	unsigned max;
	time_t beg;
	time_t last;
	unsigned cur;
	unsigned tmp;
	unsigned speed_now;
	unsigned speed_avg;
	float percentage;
	struct tm eta;
	unsigned eta_days;
};

bool nand_dump(int fd, unsigned off, unsigned len);

void progress_init(struct progress *p, unsigned min, unsigned len);
void progress_add(struct progress *p, unsigned n);
void progress_print(struct progress *p, FILE *fp);

struct code_cfg {
	struct bcm2_profile *profile;
	struct bcm2_addrspace *addrspace;
	uint32_t buffer;
	uint32_t offset;
	uint32_t length;
	uint32_t chunklen;
	uint32_t entry;
	uint32_t *code;
	uint32_t codesize;
};

typedef void (*code_upload_callback)(struct progress*, bool, void*);

bool code_init_and_upload(int fd, struct code_cfg *cfg, code_upload_callback callback, void *arg);
bool code_run(int fd, struct code_cfg *cfg);
bool code_parse_values(const char *line, uint32_t *val, bool *parseable);

#endif

