#include "c.h"
#include "postgres.h"

#include "datatype/timestamp.h" // for timestamp type
#include "fmgr.h"		// for PG_FUNCTION_*

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(i8_encode);
Datum i8_encode(PG_FUNCTION_ARGS)
{
	//
	PG_RETURN_BYTEA_P(0);
}

PG_FUNCTION_INFO_V1(i8_decode);
Datum i8_decode(PG_FUNCTION_ARGS)
{
	//
	PG_RETURN_DATUM(0);
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

PG_FUNCTION_INFO_V1(f4_encode);
Datum f4_encode(PG_FUNCTION_ARGS)
{
	//
	PG_RETURN_BYTEA_P(0);
}

PG_FUNCTION_INFO_V1(f4_decode);
Datum f4_decode(PG_FUNCTION_ARGS)
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
