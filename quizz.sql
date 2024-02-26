CREATE TABLE intrebari
(
    id INTEGER PRIMARY KEY,
    text TEXT NOT NULL, 
    raspuns1 TEXT NOT NULL,
    raspuns2 TEXT NOT NULL,
    raspuns3 TEXT NOT NULL,
    raspuns_corect INTEGER NOT NULL
);

INSERT INTO intrebari (text, raspuns1, raspuns2, raspuns3, raspuns_corect)
VALUES
('Care este capitala Romaniei?', 'Bucuresti', 'Sofia', 'Budapesta', 1),
('Care este cel mai inalt munte din lume?', 'Mont Blanc', 'Everest', 'Kilimanjaro', 2),
('Care este primul element chimic al tabelului periodic?', 'Oxigen', 'Heliu', 'Hidrogen', 3);