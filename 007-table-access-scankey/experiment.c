#include <postgres.h>
#include <access/amapi.h>
#include <access/heapam.h>
#include <access/htup_details.h>
#include <access/table.h>
#include <access/tableam.h>
#include <catalog/indexing.h>
#include <utils/builtins.h>
#include <utils/fmgroids.h>
#include <utils/rel.h>
#include <utils/snapmgr.h>

PG_MODULE_MAGIC;
PG_FUNCTION_INFO_V1(phonebook_lookup_seqscan_deform);
PG_FUNCTION_INFO_V1(phonebook_find_first_null_phone);

#define PHONEBOOK_TABLE_NAME "phonebook"
#define PHONEBOOK_PKEY_SEQ_NAME "phonebook_id_seq"
#define PHONEBOOK_NAME_IDX_NAME "phonebook_name_idx"

typedef enum Anum_phonebook
{
	Anum_phonebook_id = 1,
	Anum_phonebook_name,
	Anum_phonebook_phone,
	_Anum_phonebook_max,
} Anum_phonebook;

#define Natts_phonebook (_Anum_phonebook_max - 1)

typedef enum Anum_phonebook_name_idx
{
	Anum_phonebook_name_idx_name = 1,
	_Anum_phonebook_name_idx_max,
} Anum_phonebook_name_idx;

#define Natts_phonebook_name_idx (_Anum_phonebook_name_idx_max - 1)

static Oid
name_to_oid(const char* name)
{
	return DatumGetObjectId(DirectFunctionCall1(to_regclass, CStringGetTextDatum(name)));
}

Datum
phonebook_lookup_seqscan_deform(PG_FUNCTION_ARGS)
{
	Relation rel;
	HeapTuple tup;
	TableScanDesc scan;
	int32 found_phone = -1;
	Name name = PG_GETARG_NAME(0);
	Oid tbl_oid = name_to_oid(PHONEBOOK_TABLE_NAME);

	rel = table_open(tbl_oid, AccessShareLock);
	scan = table_beginscan(rel, GetTransactionSnapshot(), 0, NULL);

	while ((tup = heap_getnext(scan, ForwardScanDirection)) != NULL)
	{
		Name rec_name;
		Datum values[Natts_phonebook];
		bool isnull[Natts_phonebook];

		heap_deform_tuple(tup, RelationGetDescr(rel), values, isnull);
		rec_name = DatumGetName(values[Anum_phonebook_name - 1]);

		if(strcmp(rec_name->data, name->data) == 0)
		{
			if(isnull[Anum_phonebook_phone - 1] == true)
			{
				found_phone = -2;
				break;
			}

			found_phone = DatumGetInt32(values[Anum_phonebook_phone - 1]);
			break;
		}		
	}

	table_endscan(scan);
	table_close(rel, AccessShareLock);

	if(found_phone == -2)
		PG_RETURN_NULL();

	PG_RETURN_INT32(found_phone);
}

Datum
phonebook_find_first_null_phone(PG_FUNCTION_ARGS)
{
	Relation rel;
	HeapTuple tup;
	TableScanDesc scan;
	int32 found_id = -1;
	Name name = PG_GETARG_NAME(0);
	Oid tbl_oid = name_to_oid(PHONEBOOK_TABLE_NAME);
	ScanKeyData scanKey;


	ScanKeyEntryInitialize(
		&scanKey,
		SK_ISNULL | SK_SEARCHNULL, /* see skey.h */
		Anum_phonebook_phone,
		InvalidStrategy, /* no strategy */
		InvalidOid,		/* no strategy subtype */
		InvalidOid,		/* no collation */
		InvalidOid,		/* no reg proc for this */
		(Datum) 0);		/* constant */

	rel = table_open(tbl_oid, AccessShareLock);
	scan = table_beginscan(rel, GetTransactionSnapshot(), 1, &scanKey);

	while ((tup = heap_getnext(scan, ForwardScanDirection)) != NULL)
	{
		Name rec_name;
		Datum values[Natts_phonebook];
		bool isnull[Natts_phonebook];

		heap_deform_tuple(tup, RelationGetDescr(rel), values, isnull);
		rec_name = DatumGetName(values[Anum_phonebook_name - 1]);

		if(strcmp(rec_name->data, name->data) == 0)
		{
			found_id = DatumGetInt32(values[Anum_phonebook_id - 1]);
			break;
		}		
	}

	table_endscan(scan);
	table_close(rel, AccessShareLock);

	/*
	 * serial is a 32-bit integer, see:
	 * https://www.postgresql.org/docs/current/datatype-numeric.html
	 */
	PG_RETURN_INT32(found_id);
}