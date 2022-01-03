#include "c.h"
#include "postgres.h"

#include "catalog/pg_type_d.h"
#include "datatype/timestamp.h" // for timestamp type
#include "fmgr.h"		// for PG_FUNCTION_*
#include "utils/array.h"
#include "utils/timestamp.h" // for timestamptz_to_time_t

#define ARRNELEMS(x) ArrayGetNItems(ARR_NDIM(x), ARR_DIMS(x))
#define ARRPTR(x) ((uint64_t *)ARR_DATA_PTR(x))

PG_MODULE_MAGIC;

#include "encode.h"

static void *_realloc(void *p, size_t o, size_t n)
{
	if (p == NULL)
		return palloc(n);

	return repalloc(p, n);
}

PG_FUNCTION_INFO_V1(u8_encode);
Datum u8_encode(PG_FUNCTION_ARGS)
{
	ArrayType *in = PG_GETARG_ARRAYTYPE_P(0);
	int32_t inn = ARRNELEMS(in);

	uint8_t *out = NULL;
	size_t outn = 0;

	_u8_encode(ARRPTR(in), inn, &out, &outn, _realloc);

	{
		uint8_t *a = NULL;
		size_t an = 0;
		_zstd_encode(out, outn, &a, &an, _realloc);
		outn = an;
		out = a;
	}

	bytea *ret = palloc(VARHDRSZ + outn);
	SET_VARSIZE(ret, VARHDRSZ + outn);
	memcpy(VARDATA_EXTERNAL(ret), out, outn);

	PG_RETURN_BYTEA_P(ret);
}

PG_FUNCTION_INFO_V1(u8_decode);
Datum u8_decode(PG_FUNCTION_ARGS)
{
	bytea *inb = PG_GETARG_BYTEA_P(0);
	size_t inn = VARSIZE_ANY_EXHDR(inb);

	uint8_t *in = (uint8_t *)VARDATA_ANY(inb);
	uint64_t *out = NULL;
	size_t outn = 0;

	{
		uint8_t *a = NULL;
		size_t an = 0;
		_zstd_decode(in, inn, &a, &an, _realloc);
		inn = an;
		in = a;
	}

	_u8_decode(in, inn, &out, &outn, _realloc);

	int32_t nbytes = ARR_OVERHEAD_NONULLS(1) + outn;

	ArrayType *ret = (ArrayType *)palloc0(nbytes);

	SET_VARSIZE(ret, nbytes);
	ARR_NDIM(ret) = 1;
	ret->dataoffset = 0;
	ARR_ELEMTYPE(ret) = INT8OID;
	ARR_DIMS(ret)[0] = outn / sizeof(int64_t);

	for (int i = 0; i < outn / sizeof(int64_t); i++) {
		((int64_t *)ARRPTR(ret))[i] = Int64GetDatum(out[i]);
	}

	PG_RETURN_ARRAYTYPE_P(ret);
}

PG_FUNCTION_INFO_V1(timestamp_encode);
Datum timestamp_encode(PG_FUNCTION_ARGS)
{
	ArrayType *in_ts = PG_GETARG_ARRAYTYPE_P(0);
	int32_t inn = ARRNELEMS(in_ts);

	uint64_t *in = palloc(inn * sizeof(int64_t));
	for (int i = 0; i < inn; i++) {
		in[i] = ARRPTR(in_ts)[i];
	}

	uint8_t *out = NULL;
	size_t outn = 0;

	_u8_encode(in, inn, &out, &outn, _realloc);

	{
		// TODO remove 4 bytes magic
		uint8_t *a = NULL;
		size_t an = 0;
		_zstd_encode(out, outn, &a, &an, _realloc);
		out = a;
		outn = an;
	}

	bytea *ret = palloc(VARHDRSZ + outn);
	SET_VARSIZE(ret, VARHDRSZ + outn);

	memcpy(VARDATA_EXTERNAL(ret), out, outn);

	PG_RETURN_BYTEA_P(ret);
}

PG_FUNCTION_INFO_V1(timestamp_decode);
Datum timestamp_decode(PG_FUNCTION_ARGS)
{
	bytea *inb = PG_GETARG_BYTEA_P(0);
	int32_t inn = VARSIZE_ANY_EXHDR(inb);

	uint8_t *in = (uint8_t *)VARDATA_ANY(inb);
	uint64_t *out = NULL;
	size_t outn = 0;

	{
		uint8_t *a = NULL;
		size_t an = 0;
		_zstd_decode(in, inn, &a, &an, _realloc);
		inn = an;
		in = a;
	}

	_u8_decode(in, inn, &out, &outn, _realloc);

	int32_t nbytes = ARR_OVERHEAD_NONULLS(1) + outn;

	ArrayType *ret = (ArrayType *)palloc0(nbytes);

	SET_VARSIZE(ret, nbytes);
	ARR_NDIM(ret) = 1;
	ret->dataoffset = 0;
	ARR_ELEMTYPE(ret) = INT8OID;
	ARR_DIMS(ret)[0] = outn / sizeof(Timestamp);

	for (int i = 0; i < outn / sizeof(Timestamp); i++) {
		((Timestamp *)ARRPTR(ret))[i] = TimestampTzGetDatum(out[i]);
	}

	PG_RETURN_ARRAYTYPE_P(ret);
}

PG_FUNCTION_INFO_V1(f8_encode);
Datum f8_encode(PG_FUNCTION_ARGS)
{
	//
	PG_RETURN_BYTEA_P(0);
}

PG_FUNCTION_INFO_V1(f8_decode);
Datum f8_decode(PG_FUNCTION_ARGS)
{
	//
	PG_RETURN_DATUM(0);
}
