
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004
 *
 */


#ifndef _TCSD_H_
#define _TCSD_H_

#include <signal.h>

/* config structures */
struct tcsd_config
{
	int port;		/* port the TCSD will listen on */
	int num_threads;	/* max number of threads the TCSD allows simultaneously */
	char *system_ps_dir;	/* the directory the system PS file sits in */
	char *system_ps_file;	/* the name of the system PS file */
	char *firmware_log_file;/* the name of the firmware PCR event file */
	char *kernel_log_file;	/* the name of the kernel PCR event file */
	unsigned int kernel_pcrs;	/* bitmask of PCRs the kernel controls */
	unsigned int firmware_pcrs;	/* bitmask of PCRs the firmware controls */
	int remote_ops[TCSD_MAX_NUM_ORDS];	/* array of integer ordinals allow to be used by external hosts */
	unsigned int unset;	/* bitmask of options which are still unset */
};

#define TCSD_CONFIG_FILE	ETC_PREFIX "/tcsd.conf"

#define TSS_USER_NAME		"tss"
#define TSS_GROUP_NAME		"tss"

#define TCSD_DEFAULT_PORT		30003
#define TCSD_DEFAULT_MAX_THREADS	10
#define TCSD_DEFAULT_SYSTEM_PS_FILE	VAR_PREFIX "/tpm/system.data"
#define TCSD_DEFAULT_SYSTEM_PS_DIR	VAR_PREFIX "/tpm"
#define TCSD_DEFAULT_FIRMWARE_LOG_FILE	"/proc/tpm/firmware_events"
#define TCSD_DEFAULT_KERNEL_LOG_FILE	"/proc/tcg/measurement_events"
/* PCR's 0-7 */
#define TCSD_DEFAULT_FIRMWARE_PCRS	0x000000ff
/* PCR 10 only */
#define TCSD_DEFAULT_KERNEL_PCRS	0x00000400
/* This will change when a system with more than 32 PCR's exists */
#define TCSD_MAX_PCRS			32

/* this is the 2nd param passed to the listen() system call */
#define TCSD_MAX_SOCKETS_QUEUED		10
#define TCSD_TXBUF_SIZE			1024

/* for detecting whether an option has been set */
#define TCSD_OPTION_PORT		0x0001
#define TCSD_OPTION_MAX_THREADS		0x0002
#define TCSD_OPTION_FIRMWARE_PCRS	0x0004
#define TCSD_OPTION_KERNEL_PCRS		0x0008
#define TCSD_OPTION_SYSTEM_PSFILE	0x0010
#define TCSD_OPTION_KERNEL_LOGFILE	0x0020
#define TCSD_OPTION_FIRMWARE_LOGFILE	0x0040
#define TCSD_OPTION_REMOTE_OPS		0x0080

enum tcsd_config_option_code {
	opt_port = 1,
	opt_max_threads,
	opt_system_ps_file,
	opt_firmware_log,
	opt_kernel_log,
	opt_firmware_pcrs,
	opt_kernel_pcrs,
	opt_remote_ops
};

struct tcsd_config_options {
	char *name;
	enum tcsd_config_option_code option;
};

extern struct tcsd_config tcsd_options;

TSS_RESULT conf_file_init(struct tcsd_config *);
void	   conf_file_final(struct tcsd_config *);
TSS_RESULT ps_dirs_init();
void	   tcsd_signal_handler(int);

/* threading structures */
struct tcsd_thread_data
{
	BYTE *buf;
	int buf_size;
	int sock;
	UINT32 context;
	pthread_t thread_id;
	char hostname[80];
};

struct tcsd_thread_mgr
{
	pthread_mutex_t lock;
	struct tcsd_thread_data *thread_data;

	int shutdown;
	UINT32 num_active_threads;
	UINT32 max_threads;
};

TSS_RESULT tcsd_threads_init();
TSS_RESULT tcsd_threads_final();
TSS_RESULT tcsd_thread_create(int, char *);
void	   *tcsd_thread_run(void *);
void	   thread_signal_init();

/* signal handling */
struct sigaction tcsd_sa_int;
struct sigaction tcsd_sa_chld;

TSS_RESULT getTCSDPacket(struct tcsd_thread_data *, struct tcsd_packet_hdr **);

#endif
