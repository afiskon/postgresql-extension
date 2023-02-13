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
PG_FUNCTION_INFO_V1(phonebook_insert);
PG_FUNCTION_INFO_V1(phonebook_lookup_seqscan);
PG_FUNCTION_INFO_V1(phonebook_update_simple);
PG_FUNCTION_INFO_V1(phonebook_delete_simple);
PG_FUNCTION_INFO_V1(phonebook_lookup_seqscan_deform);
PG_FUNCTION_INFO_V1(phonebook_lookup_index);

#define PHONEBOOK_TABLE_NAME "phonebook"
#define PHONEBOOK_PKEY_SEQ_NAME "phonebook_id_seq"
#define PHONEBOOK_NAME_IDX_NAME "phonebook_name_idx"

typedef struct FormData_phonebook
{
	int32 id;
	NameData name;
	int32 phone;
} FormData_phonebook;

typedef FormData_phonebook* Form_phonebook;

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
phonebook_insert(PG_FUNCTION_ARGS)
{
	Relation rel;
	HeapTuple tup;
	Datum values[Natts_phonebook];
	bool nulls[Natts_phonebook];
	Name name = PG_GETARG_NAME(0);
	int32 phone = PG_GETARG_INT32(1);
	Oid tbl_oid = name_to_oid(PHONEBOOK_TABLE_NAME);
	Oid pkey_seq_oid = name_to_oid(PHONEBOOK_PKEY_SEQ_NAME);
	int32 next_id = (int32)DatumGetInt64(DirectFunctionCall1(nextval_oid, ObjectIdGetDatum(pkey_seq_oid)));

	memset(nulls, false, sizeof(nulls));

	rel = table_open(tbl_oid, RowExclusiveLock);

	// or: AttrNumberGetAttrOffset(Anum_phonebook_id)
	values[Anum_phonebook_id - 1] = Int32GetDatum(next_id);
	values[Anum_phonebook_name - 1] = NameGetDatum(name);
	values[Anum_phonebook_phone - 1] = Int32GetDatum(phone);

	tup = heap_form_tuple(RelationGetDescr(rel), values, nulls);

	/* inserts a new heap tuple, keeping indexes current */
	CatalogTupleInsert(rel, tup);
	heap_freetuple(tup);

	table_close(rel, RowExclusiveLock);
	PG_RETURN_INT32(next_id);
}

Datum
phonebook_lookup_seqscan(PG_FUNCTION_ARGS)
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
		Form_phonebook record = (Form_phonebook) GETSTRUCT(tup);

		if(strcmp(record->name.data, name->data) == 0)
		{
			found_phone = record->phone;
			break;
		}		
	}

	table_endscan(scan);
	table_close(rel, AccessShareLock);
	PG_RETURN_INT32(found_phone);
}

Datum
phonebook_update_simple(PG_FUNCTION_ARGS)
{
	Name name = PG_GETARG_NAME(0);
	int32 newphone = PG_GETARG_INT32(1);
	Relation rel;
	HeapTuple tup;
	TableScanDesc scan;
	int32 found_phone = -1;
	Oid tbl_oid = name_to_oid(PHONEBOOK_TABLE_NAME);

	rel = table_open(tbl_oid, RowExclusiveLock);
	scan = table_beginscan(rel, GetTransactionSnapshot(), 0, NULL);

	while ((tup = heap_getnext(scan, ForwardScanDirection)) != NULL)
	{
		Form_phonebook record = (Form_phonebook) GETSTRUCT(tup);
		if(strcmp(record->name.data, name->data) == 0)
		{
			HeapTuple newtup = heap_copytuple(tup);
			found_phone = record->phone;
			((Form_phonebook) GETSTRUCT(newtup))->phone = newphone;

			/* updates a heap tuple, keeping indexes current */
			CatalogTupleUpdate(rel, &newtup->t_self, newtup);
			heap_freetuple(newtup);
			break;
		}		
	}

	table_endscan(scan);
	table_close(rel, RowExclusiveLock);
	PG_RETURN_INT32(found_phone);
}

Datum
phonebook_delete_simple(PG_FUNCTION_ARGS)
{
	Name name = PG_GETARG_NAME(0);
	Relation rel;
	HeapTuple tup;
	TableScanDesc scan;
	int32 found_phone = -1;
	Oid tbl_oid = name_to_oid(PHONEBOOK_TABLE_NAME);

	rel = table_open(tbl_oid, RowExclusiveLock);
	scan = table_beginscan(rel, GetTransactionSnapshot(), 0, NULL);

	while ((tup = heap_getnext(scan, ForwardScanDirection)) != NULL)
	{
		Form_phonebook record = (Form_phonebook) GETSTRUCT(tup);
		if(strcmp(record->name.data, name->data) == 0)
		{
			found_phone = record->phone;
			/* deletes a heap tuple, keeping indexes current */
			CatalogTupleDelete(rel, &tup->t_self);
			break;
		}		
	}

	table_endscan(scan);
	table_close(rel, RowExclusiveLock);
	PG_RETURN_INT32(found_phone);
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
			found_phone = DatumGetInt32(values[Anum_phonebook_phone - 1]);
			break;
		}		
	}

	table_endscan(scan);
	table_close(rel, AccessShareLock);
	PG_RETURN_INT32(found_phone);
}

Datum
phonebook_lookup_index(PG_FUNCTION_ARGS)
{
	Relation rel, idxrel;
	IndexScanDesc scan;
	TupleTableSlot* slot;
	HeapTuple tup;
	ScanKeyData skey[1];
	int32 found_phone = -1;
	Name name = PG_GETARG_NAME(0);
	Oid tbl_oid = name_to_oid(PHONEBOOK_TABLE_NAME);
	Oid idx_oid = name_to_oid(PHONEBOOK_NAME_IDX_NAME);

	rel = table_open(tbl_oid, AccessShareLock);
	idxrel = index_open(idx_oid, AccessShareLock);
	
	scan = index_beginscan(rel, idxrel, GetTransactionSnapshot(), 1 /* nkeys */, 0 /* norderbys */);

	ScanKeyInit(&skey[0],
				Anum_phonebook_name_idx_name, /* numeration starts from 1; idx, not rel! */
				BTEqualStrategyNumber, F_NAMEEQ,
				NameGetDatum(name));
	index_rescan(scan, skey, 1, NULL /* orderbys */, 0 /* norderbys */);

	slot = table_slot_create(rel, NULL);
	while (index_getnext_slot(scan, ForwardScanDirection, slot))
	{
		Form_phonebook record;
		bool should_free;

		tup = ExecFetchSlotHeapTuple(slot, false, &should_free);
		record = (Form_phonebook) GETSTRUCT(tup);

		if(strcmp(record->name.data, name->data) == 0)
		{
			found_phone = record->phone;
			if(should_free) heap_freetuple(tup);
			break;
		}

		if(should_free) heap_freetuple(tup);
	}

	index_endscan(scan);
	ExecDropSingleTupleTableSlot(slot);
	table_close(idxrel, AccessShareLock);
	table_close(rel, AccessShareLock);
	PG_RETURN_INT32(found_phone);
}
