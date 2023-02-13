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

#define PHONEBOOK_TABLE_NAME "phonebook"
#define PHONEBOOK_PKEY_SEQ_NAME "phonebook_id_seq"
#define PHONEBOOK_NAME_IDX_NAME "phonebook_name_idx"

/*
typedef struct FormData_phonebook
{
	int32 id;
	NameData name;
	int32 phone;
} FormData_phonebook;

typedef FormData_phonebook* Form_phonebook;
*/

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
	PG_RETURN_INT32(found_phone);
}
