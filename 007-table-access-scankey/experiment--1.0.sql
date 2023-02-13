-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION experiment" to load this file. \quit

CREATE TABLE phonebook(id SERIAL PRIMARY KEY NOT NULL, name NAME NOT NULL, phone INT NOT NULL);
CREATE INDEX phonebook_name_idx ON phonebook USING btree(name);

CREATE OR REPLACE FUNCTION phonebook_insert(name NAME, phone INT) RETURNS INT
AS 'MODULE_PATHNAME', 'phonebook_insert' LANGUAGE C;

CREATE OR REPLACE FUNCTION phonebook_lookup_seqscan(name NAME) RETURNS INT
AS 'MODULE_PATHNAME', 'phonebook_lookup_seqscan' LANGUAGE C;

CREATE OR REPLACE FUNCTION phonebook_update_simple(name NAME, new_phone INT) RETURNS INT
AS 'MODULE_PATHNAME', 'phonebook_update_simple' LANGUAGE C;

CREATE OR REPLACE FUNCTION phonebook_delete_simple(name NAME) RETURNS INT
AS 'MODULE_PATHNAME', 'phonebook_delete_simple' LANGUAGE C;

CREATE OR REPLACE FUNCTION phonebook_lookup_seqscan_deform(name NAME) RETURNS INT
AS 'MODULE_PATHNAME', 'phonebook_lookup_seqscan_deform' LANGUAGE C;

CREATE OR REPLACE FUNCTION phonebook_lookup_index(name NAME) RETURNS INT
AS 'MODULE_PATHNAME', 'phonebook_lookup_index' LANGUAGE C;

-- TODO: delete