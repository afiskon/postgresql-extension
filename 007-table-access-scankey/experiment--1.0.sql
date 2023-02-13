-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION experiment" to load this file. \quit

CREATE TABLE phonebook(id SERIAL PRIMARY KEY NOT NULL, name NAME NOT NULL, phone INT /* nullable */);
CREATE INDEX phonebook_name_idx ON phonebook USING btree(name);

CREATE OR REPLACE FUNCTION phonebook_lookup_seqscan_deform(name NAME) RETURNS INT
AS 'MODULE_PATHNAME', 'phonebook_lookup_seqscan_deform' LANGUAGE C;

CREATE OR REPLACE FUNCTION phonebook_find_first_null_phone() RETURNS INT
AS 'MODULE_PATHNAME', 'phonebook_find_first_null_phone' LANGUAGE C;

CREATE OR REPLACE FUNCTION phonebook_find_first_notnull_phone() RETURNS INT
AS 'MODULE_PATHNAME', 'phonebook_find_first_notnull_phone' LANGUAGE C;