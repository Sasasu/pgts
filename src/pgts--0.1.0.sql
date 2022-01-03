\echo Use "CREATE EXTENSION gpts;" to load this file. \quit

create schema if not exists ts;

-- bigint or timestamp
create or replace function ts.i8_encode(v bigint[]) returns bytea strict as 'MODULE_PATHNAME' language c;
create or replace function ts.i8_decode(v bytea) returns table (v bigint) strict as 'MODULE_PATHNAME' language c;
create or replace function ts.timestamp_encode(v timestamp[]) returns bytea strict as 'MODULE_PATHNAME' language c;
create or replace function ts.timestamp_decode(v bytea) returns table (v timestamp[]) strict as 'MODULE_PATHNAME' language c;

-- real
create or replace function ts.f4_encode(v real[]) returns bytea strict as 'MODULE_PATHNAME' language c;
create or replace function ts.f4_decode(v bytea) returns table (v real) strict as 'MODULE_PATHNAME' language c;

-- double precision
create or replace function ts.f8_encode(v double precision[]) returns bytea strict as 'MODULE_PATHNAME' language c;
create or replace function ts.f8_decode(v bytea) returns table (v double precision) strict as 'MODULE_PATHNAME' language c;

