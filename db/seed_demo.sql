CREATE TEMP TABLE IF NOT EXISTS demo_seed (
    kind TEXT NOT NULL,
    first_value TEXT NOT NULL,
    second_value TEXT NOT NULL DEFAULT '',
    third_value TEXT NOT NULL DEFAULT ''
);
DELETE FROM temp.demo_seed;

INSERT INTO temp.demo_seed (kind, first_value) VALUES ('D', 'Computer Science');
INSERT INTO temp.demo_seed (kind, first_value) VALUES ('D', 'Mathematics');
INSERT INTO temp.demo_seed (kind, first_value) VALUES ('D', 'Physics');

INSERT INTO temp.demo_seed (kind, first_value, second_value, third_value) VALUES ('U', 'admin', 'ADMIN', '');
INSERT INTO temp.demo_seed (kind, first_value, second_value, third_value) VALUES ('U', 'hod_cs', 'HOD', 'Computer Science');
INSERT INTO temp.demo_seed (kind, first_value, second_value, third_value) VALUES ('U', 'lecturer1', 'LECTURER', 'Computer Science');
INSERT INTO temp.demo_seed (kind, first_value, second_value, third_value) VALUES ('U', 'lecturer2', 'LECTURER', 'Mathematics');
INSERT INTO temp.demo_seed (kind, first_value, second_value, third_value) VALUES ('U', 'student1', 'STUDENT', 'Computer Science');
INSERT INTO temp.demo_seed (kind, first_value, second_value, third_value) VALUES ('U', 'student2', 'STUDENT', 'Computer Science');
INSERT INTO temp.demo_seed (kind, first_value, second_value, third_value) VALUES ('U', 'student3', 'STUDENT', 'Mathematics');

INSERT INTO temp.demo_seed (kind, first_value, second_value, third_value) VALUES ('C', 'CS101', 'Introduction to Programming', 'Computer Science');
INSERT INTO temp.demo_seed (kind, first_value, second_value, third_value) VALUES ('C', 'CS201', 'Data Structures', 'Computer Science');
INSERT INTO temp.demo_seed (kind, first_value, second_value, third_value) VALUES ('C', 'MATH101', 'Calculus I', 'Mathematics');

INSERT INTO temp.demo_seed (kind, first_value, second_value) VALUES ('L', 'CS101', 'lecturer1');
INSERT INTO temp.demo_seed (kind, first_value, second_value) VALUES ('L', 'CS201', 'lecturer1');
INSERT INTO temp.demo_seed (kind, first_value, second_value) VALUES ('L', 'MATH101', 'lecturer2');
INSERT INTO temp.demo_seed (kind, first_value, second_value) VALUES ('E', 'CS101', 'student1');
INSERT INTO temp.demo_seed (kind, first_value, second_value) VALUES ('E', 'CS101', 'student2');
INSERT INTO temp.demo_seed (kind, first_value, second_value) VALUES ('E', 'CS201', 'student1');
INSERT INTO temp.demo_seed (kind, first_value, second_value) VALUES ('E', 'MATH101', 'student3');
