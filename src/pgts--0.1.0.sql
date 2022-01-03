\echo Use "CREATE EXTENSION gpts;" to load this file. \quit

create schema if not exists ts;

-- bigint or timestamp
create or replace function ts.u8_encode(v bigint[]) returns bytea strict as 'MODULE_PATHNAME' language c;
create or replace function ts.u8_decode(v bytea) returns bigint[] strict as 'MODULE_PATHNAME' language c;
create or replace function ts.timestamp_encode(v timestamp[]) returns bytea strict as 'MODULE_PATHNAME' language c;
create or replace function ts.timestamp_decode(v bytea) returns timestamp[] strict as 'MODULE_PATHNAME' language c;

-- double precision
create or replace function ts.f8_encode(v double precision[]) returns bytea strict as 'MODULE_PATHNAME' language c;
create or replace function ts.f8_decode(v bytea) returns table (v double precision) strict as 'MODULE_PATHNAME' language c;
