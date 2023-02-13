CREATE EXTENSION experiment;

INSERT INTO phonebook (id, name, phone) VALUES
(1, 'Alice', 123456),
(2, 'Bob', NULL);

SELECT phonebook_lookup_seqscan_deform('Alice');
SELECT phonebook_lookup_seqscan_deform('Bob');

DROP EXTENSION experiment;