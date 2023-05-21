#include <postgres.h>
#include <miscadmin.h>
#include <storage/ipc.h>
#include <storage/shmem.h>
#include <storage/lwlock.h>
#include <utils/builtins.h>

static shmem_request_hook_type prev_shmem_request_hook = NULL;
static shmem_startup_hook_type prev_shmem_startup_hook = NULL;

PG_MODULE_MAGIC;
PG_FUNCTION_INFO_V1(experiment_get_message);
PG_FUNCTION_INFO_V1(experiment_set_message);
PG_FUNCTION_INFO_V1(experiment_lock_and_throw_error);

#define MESSAGE_BUFF_SIZE 128
typedef struct SharedStruct {
	char message[MESSAGE_BUFF_SIZE];
} SharedStruct;

LWLock* sharedStructLock = NULL;
SharedStruct *sharedStruct = NULL;

static void
experiment_shmem_request(void)
{
	elog(LOG, "experiment_shmem_request(): pid = %d, postmaster = %d\n",
		MyProcPid, !IsUnderPostmaster);

	if(prev_shmem_request_hook)
		prev_shmem_request_hook();

	RequestAddinShmemSpace(MAXALIGN(sizeof(SharedStruct)));

	RequestNamedLWLockTranche("experiment", 1);
}

static void
experiment_shmem_startup(void)
{
	bool found;

	elog(LOG, "experiment_shmem_startup(): pid = %d, postmaster = %d\n",
		MyProcPid, !IsUnderPostmaster);

	if(prev_shmem_startup_hook)
		prev_shmem_startup_hook();

	/*
	 * The documentation recommends taking AddinShmemInitLock before calling
	 * ShmemInitStruct() and so does the example code in pg_stat_statements.c.
	 *
	 * I suspect this may be redundant in this particular case since actually
	 * shmem_startup_hook is called by postmaster.
	 */
	LWLockAcquire(AddinShmemInitLock, LW_EXCLUSIVE);

	sharedStruct = ShmemInitStruct("SharedStruct", sizeof(SharedStruct), &found);
	if(!found) {
		sharedStruct->message[0] = '\0';
		/*
		 * If in doubt, better place LWLock* within a structure in shared memory.
		 * To my knowledge the presented code is safe, at least at the moment of
		 * writing. However this is not how the documentation recommends doing this.
		 *
		 * See the discussion:
		 * https://postgr.es/m/CAJ7c6TPKhFgL%2B54cdTD9yGpG4%2BsNcyJ%2BN1GvQqAxgWENAOa3VA%40mail.gmail.com
		 */
		sharedStructLock = &(GetNamedLWLockTranche("experiment"))->lock;
	}

	LWLockRelease(AddinShmemInitLock);
}

void
_PG_init(void)
{
	if(!process_shared_preload_libraries_in_progress)
		elog(FATAL, "Please use shared_preload_libraries");

	elog(LOG, "_PG_init(): extension loaded");

	prev_shmem_request_hook = shmem_request_hook;
	shmem_request_hook = experiment_shmem_request;

	prev_shmem_startup_hook = shmem_startup_hook;
	shmem_startup_hook = experiment_shmem_startup;
}

Datum
experiment_get_message(PG_FUNCTION_ARGS)
{
	text* result;

	Assert(sharedStructLock != NULL);
	Assert(sharedStruct != NULL);

	LWLockAcquire(sharedStructLock, LW_SHARED);
	result = cstring_to_text(sharedStruct->message);
	LWLockRelease(sharedStructLock);

	PG_RETURN_TEXT_P(result);
}

Datum
experiment_set_message(PG_FUNCTION_ARGS)
{
	const char* msg = TextDatumGetCString(PG_GETARG_DATUM(0));

	Assert(sharedStructLock != NULL);
	Assert(sharedStruct != NULL);

	LWLockAcquire(sharedStructLock, LW_EXCLUSIVE);
	strncpy(sharedStruct->message, msg, MESSAGE_BUFF_SIZE-1);
	LWLockRelease(sharedStructLock);

	PG_RETURN_VOID();
}

/* Make sure the lock will be released if we throw an error */
Datum
experiment_lock_and_throw_error(PG_FUNCTION_ARGS)
{
	Assert(sharedStructLock != NULL);
	LWLockAcquire(sharedStructLock, LW_EXCLUSIVE);

	elog(ERROR, "error");

	PG_RETURN_VOID();
}
