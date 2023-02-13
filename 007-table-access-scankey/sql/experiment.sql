CREATE EXTENSION experiment;

SELECT phonebook_find_first_null_phone();

INSERT INTO phonebook (id, name, phone) VALUES
(1, 'Alice', 123456),
(2, 'Bob', NULL);

SELECT phonebook_lookup_seqscan_deform('Alice');
SELECT phonebook_lookup_seqscan_deform('Bob');
SELECT phonebook_lookup_seqscan_deform('Bob') IS NULL AS "is_null";

SELECT phonebook_find_first_null_phone();

DROP EXTENSION experiment;