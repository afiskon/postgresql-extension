CREATE EXTENSION experiment;

SELECT phonebook_insert('Alice', 123);
SELECT * FROM phonebook;

SELECT phonebook_insert('Bob', 456);
SELECT * FROM phonebook;

SELECT phonebook_lookup_seqscan('Alice');
SELECT phonebook_lookup_seqscan('Bob');
SELECT phonebook_lookup_seqscan('Charlie');

SELECT phonebook_lookup_seqscan_deform('Alice');
SELECT phonebook_lookup_seqscan_deform('Bob');
SELECT phonebook_lookup_seqscan_deform('Charlie');

SELECT phonebook_lookup_index('Alice');
SELECT phonebook_lookup_index('Bob');
SELECT phonebook_lookup_index('Charlie');

SELECT phonebook_update_simple('Alice', 789);
SELECT phonebook_lookup_index('Alice');

SELECT phonebook_delete_simple('Alice');
SELECT phonebook_lookup_index('Alice');
SELECT phonebook_delete_simple('Charlie');

DROP EXTENSION experiment;