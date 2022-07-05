#include "c.h"
#include "postgres.h"

#include "catalog/pg_type_d.h"
#include "datatype/timestamp.h" // for timestamp type
#include "fmgr.h"		// for PG_FUNCTION_*
#include "utils/array.h"

#define ARRNELEMS(x) ArrayGetNItems(ARR_NDIM(x), ARR_DIMS(x))
#define ARRPTR(x) ((uint64_t *)ARR_DATA_PTR(x))

PG_MODULE_MAGIC;

#include "encode.h"

static void *_realloc(void *p, size_t o, size_t n) { return repalloc(p, n); }

PG_FUNCTION_INFO_V1(u8_encode);
Datum u8_encode(PG_FUNCTION_ARGS)
{
	ArrayType *in = PG_GETARG_ARRAYTYPE_P(0);
	int32_t inn = ARRNELEMS(in);

	uint8_t *out = NULL;
	size_t outn = 0;

	_u8_encode(ARRPTR(in), inn, &out, &outn, _realloc);

	bytea *ret = palloc(VARHDRSZ + outn);
	SET_VARSIZE(ret, VARHDRSZ + outn);
	memcpy(VARDATA(ret), VARDATA_ANY(out), outn);

	PG_RETURN_BYTEA_P(ret);
}

PG_FUNCTION_INFO_V1(u8_decode);
Datum u8_decode(PG_FUNCTION_ARGS)
{
	bytea *in = PG_GETARG_BYTEA_P(0);
	int32_t inn = VARSIZE_ANY_EXHDR(in);

	uint64_t *out = NULL;
	size_t outn = 0;

	_u8_decode((uint8_t *)VARDATA_ANY(in), inn, &out, &outn, _realloc);

	int32_t nbytes = ARR_OVERHEAD_NONULLS(1) + sizeof(uint64_t) * outn;
	ArrayType *ret = (ArrayType *) palloc0(nbytes);
	SET_VARSIZE(ret, nbytes);
	ARR_NDIM(ret) = 1;
	ret->dataoffset = 0;
	ARR_ELEMTYPE(ret) = INT8OID;
	ARR_DIMS(ret)[0] = outn;
	ARR_LBOUND(ret)[0] = 1;

	memcpy(ARRPTR(ret), out, outn * sizeof(uint64_t));

	PG_RETURN_ARRAYTYPE_P(ret);
}

PG_FUNCTION_INFO_V1(timestamp_encode);
Datum timestamp_encode(PG_FUNCTION_ARGS)
{
	//
	PG_RETURN_BYTEA_P(0);
}

PG_FUNCTION_INFO_V1(timestamp_decode);
Datum timestamp_decode(PG_FUNCTION_ARGS)
{
	//
	PG_RETURN_DATUM(0);
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
