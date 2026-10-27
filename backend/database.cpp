#include "database.hpp"
#include <iostream>
#include <sstream>
#include <ctime>
#include <iomanip>
#include <algorithm>

Database::Database() = default;

Database::~Database() {
    close();
}

void Database::close() {
    std::lock_guard<std::mutex> lock(db_mutex_);
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

std::string Database::current_timestamp() {
    std::time_t now = std::time(nullptr);
    std::tm tm_now{};
#if defined(_WIN32)
    localtime_s(&tm_now, &now);
#else
    localtime_r(&now, &tm_now);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm_now, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

bool Database::init(const std::string& db_file_path) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    if (sqlite3_open(db_file_path.c_str(), &db_) != SQLITE_OK) {
        std::cerr << "Failed to open SQLite database: " << (db_ ? sqlite3_errmsg(db_) : "unknown error") << std::endl;
        return false;
    }

    // Enable foreign keys
    char* err_msg = nullptr;
    if (sqlite3_exec(db_, "PRAGMA foreign_keys = ON;", nullptr, nullptr, &err_msg) != SQLITE_OK) {
        std::cerr << "Failed to enable foreign keys: " << (err_msg ? err_msg : "") << std::endl;
        sqlite3_free(err_msg);
    }

    // Enable WAL mode for performance and concurrency
    sqlite3_exec(db_, "PRAGMA journal_mode = WAL;", nullptr, nullptr, nullptr);

    if (!create_tables()) {
        return false;
    }

    seed_default_data();
    return true;
}

bool Database::create_tables() {
    const char* schema_sql = R"(
        CREATE TABLE IF NOT EXISTS courses (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            course_name TEXT UNIQUE NOT NULL,
            department TEXT NOT NULL,
            duration TEXT NOT NULL,
            description TEXT
        );

        CREATE TABLE IF NOT EXISTS students (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            student_id TEXT UNIQUE NOT NULL,
            full_name TEXT NOT NULL,
            email TEXT UNIQUE NOT NULL,
            phone TEXT,
            course_id INTEGER NOT NULL,
            semester INTEGER NOT NULL,
            marks_obtained REAL NOT NULL,
            maximum_marks REAL NOT NULL,
            percentage REAL NOT NULL,
            grade TEXT NOT NULL,
            status TEXT NOT NULL,
            admission_date TEXT NOT NULL,
            created_at TEXT NOT NULL,
            updated_at TEXT NOT NULL,
            FOREIGN KEY (course_id) REFERENCES courses(id) ON DELETE RESTRICT
        );

        CREATE INDEX IF NOT EXISTS idx_student_id ON students(student_id);
        CREATE INDEX IF NOT EXISTS idx_student_email ON students(email);
        CREATE INDEX IF NOT EXISTS idx_student_course ON students(course_id);
    )";

    char* err_msg = nullptr;
    if (sqlite3_exec(db_, schema_sql, nullptr, nullptr, &err_msg) != SQLITE_OK) {
        std::cerr << "Failed to create schema: " << (err_msg ? err_msg : "") << std::endl;
        sqlite3_free(err_msg);
        return false;
    }
    return true;
}

void Database::seed_default_data() {
    // Check if courses table is empty
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, "SELECT COUNT(*) FROM courses;", -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            int count = sqlite3_column_int(stmt, 0);
            if (count == 0) {
                const char* insert_courses = R"(
                    INSERT INTO courses (course_name, department, duration, description) VALUES
                    ('B.Tech Computer Science', 'Computer Engineering', '4 Years', 'Bachelor of Technology in Computer Science & Engineering'),
                    ('B.Tech Electronics & Comm', 'Electronics & Comm', '4 Years', 'Bachelor of Technology in Electronics and Communication Engineering'),
                    ('B.Tech Mechanical Eng', 'Mechanical Engineering', '4 Years', 'Bachelor of Technology in Mechanical Systems & Design'),
                    ('Master of Business Admin (MBA)', 'Management Studies', '2 Years', 'Postgraduate program in Business Administration and Leadership'),
                    ('B.Sc Data Science & AI', 'Information Technology', '3 Years', 'Undergraduate degree focusing on Data Analytics and Machine Learning');
                )";
                sqlite3_exec(db_, insert_courses, nullptr, nullptr, nullptr);
            }
        }
        sqlite3_finalize(stmt);
    }

    // Check if students table is empty
    stmt = nullptr;
    if (sqlite3_prepare_v2(db_, "SELECT COUNT(*) FROM students;", -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            int count = sqlite3_column_int(stmt, 0);
            if (count == 0) {
                const char* insert_students = R"(
                    INSERT INTO students (student_id, full_name, email, phone, course_id, semester, marks_obtained, maximum_marks, percentage, grade, status, admission_date, created_at, updated_at) VALUES
                    ('STU2024001', 'Aarav Sharma', 'aarav.sharma@example.edu', '+91 9876543210', 1, 4, 465.0, 500.0, 93.00, 'A+', 'Pass', '2022-08-01', datetime('now'), datetime('now')),
                    ('STU2024002', 'Diya Patel', 'diya.patel@example.edu', '+91 9876543211', 1, 4, 420.0, 500.0, 84.00, 'A', 'Pass', '2022-08-01', datetime('now'), datetime('now')),
                    ('STU2024003', 'Rohan Mehta', 'rohan.mehta@example.edu', '+91 9876543212', 2, 6, 375.0, 500.0, 75.00, 'B', 'Pass', '2021-08-01', datetime('now'), datetime('now')),
                    ('STU2024004', 'Ananya Singh', 'ananya.singh@example.edu', '+91 9876543213', 3, 2, 325.0, 500.0, 65.00, 'C', 'Pass', '2023-08-01', datetime('now'), datetime('now')),
                    ('STU2024005', 'Vikram Malhotra', 'vikram.m@example.edu', '+91 9876543214', 4, 2, 260.0, 500.0, 52.00, 'D', 'Pass', '2023-09-01', datetime('now'), datetime('now')),
                    ('STU2024006', 'Sneha Kapoor', 'sneha.k@example.edu', '+91 9876543215', 5, 2, 175.0, 500.0, 35.00, 'F', 'Fail', '2023-08-01', datetime('now'), datetime('now')),
                    ('STU2024007', 'Kabir Verma', 'kabir.verma@example.edu', '+91 9876543216', 1, 2, 480.0, 500.0, 96.00, 'A+', 'Pass', '2023-08-01', datetime('now'), datetime('now')),
                    ('STU2024008', 'Priya Joshi', 'priya.joshi@example.edu', '+91 9876543217', 2, 4, 388.0, 500.0, 77.60, 'B', 'Pass', '2022-08-01', datetime('now'), datetime('now'));
                )";
                sqlite3_exec(db_, insert_students, nullptr, nullptr, nullptr);
            }
        }
        sqlite3_finalize(stmt);
    }
}

// ----------------- COURSE METHODS -----------------

std::vector<Course> Database::get_all_courses() {
    std::lock_guard<std::mutex> lock(db_mutex_);
    std::vector<Course> courses;

    const char* sql = R"(
        SELECT c.id, c.course_name, c.department, c.duration, c.description,
               (SELECT COUNT(*) FROM students s WHERE s.course_id = c.id) AS student_count
        FROM courses c
        ORDER BY c.course_name ASC;
    )";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            Course c;
            c.id = sqlite3_column_int(stmt, 0);
            c.course_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            c.department = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            c.duration = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            const char* desc = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
            c.description = desc ? desc : "";
            c.student_count = sqlite3_column_int(stmt, 5);
            courses.push_back(c);
        }
        sqlite3_finalize(stmt);
    }
    return courses;
}

std::optional<Course> Database::get_course_by_id(int id) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    const char* sql = R"(
        SELECT c.id, c.course_name, c.department, c.duration, c.description,
               (SELECT COUNT(*) FROM students s WHERE s.course_id = c.id) AS student_count
        FROM courses c
        WHERE c.id = ?;
    )";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, id);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            Course c;
            c.id = sqlite3_column_int(stmt, 0);
            c.course_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            c.department = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            c.duration = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            const char* desc = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
            c.description = desc ? desc : "";
            c.student_count = sqlite3_column_int(stmt, 5);
            sqlite3_finalize(stmt);
            return c;
        }
        sqlite3_finalize(stmt);
    }
    return std::nullopt;
}

bool Database::add_course(const Course& course, std::string& error_msg) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    const char* sql = "INSERT INTO courses (course_name, department, duration, description) VALUES (?, ?, ?, ?);";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        error_msg = "Database statement preparation failed.";
        return false;
    }

    sqlite3_bind_text(stmt, 1, course.course_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, course.department.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, course.duration.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, course.description.c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        if (rc == SQLITE_CONSTRAINT) {
            error_msg = "A course with this name already exists.";
        } else {
            error_msg = "Failed to create course in database.";
        }
        return false;
    }
    return true;
}

bool Database::update_course(int id, const Course& course, std::string& error_msg) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    const char* sql = "UPDATE courses SET course_name = ?, department = ?, duration = ?, description = ? WHERE id = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        error_msg = "Database statement preparation failed.";
        return false;
    }

    sqlite3_bind_text(stmt, 1, course.course_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, course.department.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, course.duration.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, course.description.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 5, id);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        if (rc == SQLITE_CONSTRAINT) {
            error_msg = "Course name must be unique. Another course has this name.";
        } else {
            error_msg = "Failed to update course.";
        }
        return false;
    }

    if (sqlite3_changes(db_) == 0) {
        error_msg = "Course not found.";
        return false;
    }
    return true;
}

bool Database::delete_course(int id, std::string& error_msg) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    // Check if any students are enrolled in this course
    const char* check_sql = "SELECT COUNT(*) FROM students WHERE course_id = ?;";
    sqlite3_stmt* check_stmt = nullptr;
    if (sqlite3_prepare_v2(db_, check_sql, -1, &check_stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(check_stmt, 1, id);
        if (sqlite3_step(check_stmt) == SQLITE_ROW) {
            int enrolled = sqlite3_column_int(check_stmt, 0);
            if (enrolled > 0) {
                sqlite3_finalize(check_stmt);
                error_msg = "Cannot delete course: " + std::to_string(enrolled) + " student(s) currently enrolled.";
                return false;
            }
        }
        sqlite3_finalize(check_stmt);
    }

    const char* sql = "DELETE FROM courses WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        error_msg = "Database statement error.";
        return false;
    }

    sqlite3_bind_int(stmt, 1, id);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        error_msg = "Failed to delete course.";
        return false;
    }

    if (sqlite3_changes(db_) == 0) {
        error_msg = "Course not found.";
        return false;
    }
    return true;
}

// ----------------- STUDENT METHODS -----------------

std::vector<Student> Database::get_students(
    const std::string& search,
    int course_id,
    const std::string& grade,
    const std::string& sort_by,
    const std::string& sort_dir,
    int page,
    int limit,
    int& total_count
) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    std::vector<Student> students;
    total_count = 0;

    std::string where_clause = " WHERE 1=1";
    std::vector<std::pair<int, std::string>> text_params;
    std::vector<std::pair<int, int>> int_params;
    int param_idx = 1;

    if (!search.empty()) {
        where_clause += " AND (s.student_id LIKE ? OR s.full_name LIKE ? OR s.email LIKE ?)";
        std::string pattern = "%" + search + "%";
        text_params.emplace_back(param_idx++, pattern);
        text_params.emplace_back(param_idx++, pattern);
        text_params.emplace_back(param_idx++, pattern);
    }

    if (course_id > 0) {
        where_clause += " AND s.course_id = ?";
        int_params.emplace_back(param_idx++, course_id);
    }

    if (!grade.empty()) {
        where_clause += " AND s.grade = ?";
        text_params.emplace_back(param_idx++, grade);
    }

    // First count total matching records
    std::string count_sql = "SELECT COUNT(*) FROM students s " + where_clause + ";";
    sqlite3_stmt* count_stmt = nullptr;
    if (sqlite3_prepare_v2(db_, count_sql.c_str(), -1, &count_stmt, nullptr) == SQLITE_OK) {
        for (const auto& tp : text_params) {
            sqlite3_bind_text(count_stmt, tp.first, tp.second.c_str(), -1, SQLITE_TRANSIENT);
        }
        for (const auto& ip : int_params) {
            sqlite3_bind_int(count_stmt, ip.first, ip.second);
        }
        if (sqlite3_step(count_stmt) == SQLITE_ROW) {
            total_count = sqlite3_column_int(count_stmt, 0);
        }
        sqlite3_finalize(count_stmt);
    }

    // Determine sorting column safely
    std::string order_col = "s.id";
    if (sort_by == "student_id") order_col = "s.student_id";
    else if (sort_by == "full_name") order_col = "s.full_name";
    else if (sort_by == "marks_obtained") order_col = "s.marks_obtained";
    else if (sort_by == "percentage") order_col = "s.percentage";
    else if (sort_by == "admission_date") order_col = "s.admission_date";
    else if (sort_by == "grade") order_col = "s.grade";

    std::string order_direction = (sort_dir == "asc" || sort_dir == "ASC") ? "ASC" : "DESC";

    std::string query_sql = R"(
        SELECT s.id, s.student_id, s.full_name, s.email, s.phone, s.course_id,
               c.course_name, c.department, s.semester, s.marks_obtained, s.maximum_marks,
               s.percentage, s.grade, s.status, s.admission_date, s.created_at, s.updated_at
        FROM students s
        LEFT JOIN courses c ON s.course_id = c.id
    )" + where_clause + " ORDER BY " + order_col + " " + order_direction;

    if (limit > 0) {
        int offset = (page > 0) ? (page - 1) * limit : 0;
        query_sql += " LIMIT ? OFFSET ?";
    }

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, query_sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        for (const auto& tp : text_params) {
            sqlite3_bind_text(stmt, tp.first, tp.second.c_str(), -1, SQLITE_TRANSIENT);
        }
        for (const auto& ip : int_params) {
            sqlite3_bind_int(stmt, ip.first, ip.second);
        }
        if (limit > 0) {
            int offset = (page > 0) ? (page - 1) * limit : 0;
            sqlite3_bind_int(stmt, param_idx++, limit);
            sqlite3_bind_int(stmt, param_idx++, offset);
        }

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            Student s;
            s.id = sqlite3_column_int(stmt, 0);
            s.student_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            s.full_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            s.email = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            const char* phone = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
            s.phone = phone ? phone : "";
            s.course_id = sqlite3_column_int(stmt, 5);
            const char* c_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
            s.course_name = c_name ? c_name : "Unassigned";
            const char* dept = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
            s.department = dept ? dept : "";
            s.semester = sqlite3_column_int(stmt, 8);
            s.marks_obtained = sqlite3_column_double(stmt, 9);
            s.maximum_marks = sqlite3_column_double(stmt, 10);
            s.percentage = sqlite3_column_double(stmt, 11);
            s.grade = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 12));
            s.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 13));
            s.admission_date = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 14));
            s.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 15));
            s.updated_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 16));
            students.push_back(s);
        }
        sqlite3_finalize(stmt);
    }
    return students;
}

std::optional<Student> Database::get_student_by_id(int id) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    const char* sql = R"(
        SELECT s.id, s.student_id, s.full_name, s.email, s.phone, s.course_id,
               c.course_name, c.department, s.semester, s.marks_obtained, s.maximum_marks,
               s.percentage, s.grade, s.status, s.admission_date, s.created_at, s.updated_at
        FROM students s
        LEFT JOIN courses c ON s.course_id = c.id
        WHERE s.id = ?;
    )";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, id);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            Student s;
            s.id = sqlite3_column_int(stmt, 0);
            s.student_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            s.full_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            s.email = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            const char* phone = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
            s.phone = phone ? phone : "";
            s.course_id = sqlite3_column_int(stmt, 5);
            const char* c_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
            s.course_name = c_name ? c_name : "Unassigned";
            const char* dept = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
            s.department = dept ? dept : "";
            s.semester = sqlite3_column_int(stmt, 8);
            s.marks_obtained = sqlite3_column_double(stmt, 9);
            s.maximum_marks = sqlite3_column_double(stmt, 10);
            s.percentage = sqlite3_column_double(stmt, 11);
            s.grade = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 12));
            s.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 13));
            s.admission_date = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 14));
            s.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 15));
            s.updated_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 16));
            sqlite3_finalize(stmt);
            return s;
        }
        sqlite3_finalize(stmt);
    }
    return std::nullopt;
}

bool Database::add_student(Student& student, std::string& error_msg) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    // Check duplicate student_id
    {
        const char* chk_sql = "SELECT id FROM students WHERE student_id = ?;";
        sqlite3_stmt* chk_stmt = nullptr;
        if (sqlite3_prepare_v2(db_, chk_sql, -1, &chk_stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(chk_stmt, 1, student.student_id.c_str(), -1, SQLITE_TRANSIENT);
            if (sqlite3_step(chk_stmt) == SQLITE_ROW) {
                sqlite3_finalize(chk_stmt);
                error_msg = "Student ID '" + student.student_id + "' is already registered.";
                return false;
            }
            sqlite3_finalize(chk_stmt);
        }
    }

    // Check duplicate email
    {
        const char* chk_sql = "SELECT id FROM students WHERE email = ?;";
        sqlite3_stmt* chk_stmt = nullptr;
        if (sqlite3_prepare_v2(db_, chk_sql, -1, &chk_stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(chk_stmt, 1, student.email.c_str(), -1, SQLITE_TRANSIENT);
            if (sqlite3_step(chk_stmt) == SQLITE_ROW) {
                sqlite3_finalize(chk_stmt);
                error_msg = "Email address '" + student.email + "' is already in use.";
                return false;
            }
            sqlite3_finalize(chk_stmt);
        }
    }

    // Check course existence
    {
        const char* chk_sql = "SELECT course_name FROM courses WHERE id = ?;";
        sqlite3_stmt* chk_stmt = nullptr;
        if (sqlite3_prepare_v2(db_, chk_sql, -1, &chk_stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(chk_stmt, 1, student.course_id);
            if (sqlite3_step(chk_stmt) != SQLITE_ROW) {
                sqlite3_finalize(chk_stmt);
                error_msg = "Selected course does not exist.";
                return false;
            }
            sqlite3_finalize(chk_stmt);
        }
    }

    // Calculate percentage, grade, status in backend
    student.calculate_grade_and_status();
    std::string now_str = current_timestamp();
    student.created_at = now_str;
    student.updated_at = now_str;

    const char* sql = R"(
        INSERT INTO students (
            student_id, full_name, email, phone, course_id, semester,
            marks_obtained, maximum_marks, percentage, grade, status,
            admission_date, created_at, updated_at
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);
    )";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        error_msg = "Failed to prepare student insert statement.";
        return false;
    }

    sqlite3_bind_text(stmt, 1, student.student_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, student.full_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, student.email.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, student.phone.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 5, student.course_id);
    sqlite3_bind_int(stmt, 6, student.semester);
    sqlite3_bind_double(stmt, 7, student.marks_obtained);
    sqlite3_bind_double(stmt, 8, student.maximum_marks);
    sqlite3_bind_double(stmt, 9, student.percentage);
    sqlite3_bind_text(stmt, 10, student.grade.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 11, student.status.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 12, student.admission_date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 13, student.created_at.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 14, student.updated_at.c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        if (rc == SQLITE_CONSTRAINT) {
            error_msg = "Student ID or Email already exists.";
        } else {
            error_msg = "Database insert error.";
        }
        return false;
    }

    student.id = static_cast<int>(sqlite3_last_insert_rowid(db_));
    return true;
}

bool Database::update_student(int id, Student& student, std::string& error_msg) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    // Check duplicate student_id on other records
    {
        const char* chk_sql = "SELECT id FROM students WHERE student_id = ? AND id != ?;";
        sqlite3_stmt* chk_stmt = nullptr;
        if (sqlite3_prepare_v2(db_, chk_sql, -1, &chk_stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(chk_stmt, 1, student.student_id.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(chk_stmt, 2, id);
            if (sqlite3_step(chk_stmt) == SQLITE_ROW) {
                sqlite3_finalize(chk_stmt);
                error_msg = "Student ID '" + student.student_id + "' is already in use by another student.";
                return false;
            }
            sqlite3_finalize(chk_stmt);
        }
    }

    // Check duplicate email on other records
    {
        const char* chk_sql = "SELECT id FROM students WHERE email = ? AND id != ?;";
        sqlite3_stmt* chk_stmt = nullptr;
        if (sqlite3_prepare_v2(db_, chk_sql, -1, &chk_stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(chk_stmt, 1, student.email.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(chk_stmt, 2, id);
            if (sqlite3_step(chk_stmt) == SQLITE_ROW) {
                sqlite3_finalize(chk_stmt);
                error_msg = "Email address '" + student.email + "' is already in use by another student.";
                return false;
            }
            sqlite3_finalize(chk_stmt);
        }
    }

    // Check course existence
    {
        const char* chk_sql = "SELECT course_name FROM courses WHERE id = ?;";
        sqlite3_stmt* chk_stmt = nullptr;
        if (sqlite3_prepare_v2(db_, chk_sql, -1, &chk_stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(chk_stmt, 1, student.course_id);
            if (sqlite3_step(chk_stmt) != SQLITE_ROW) {
                sqlite3_finalize(chk_stmt);
                error_msg = "Selected course does not exist.";
                return false;
            }
            sqlite3_finalize(chk_stmt);
        }
    }

    student.calculate_grade_and_status();
    student.updated_at = current_timestamp();

    const char* sql = R"(
        UPDATE students SET
            student_id = ?,
            full_name = ?,
            email = ?,
            phone = ?,
            course_id = ?,
            semester = ?,
            marks_obtained = ?,
            maximum_marks = ?,
            percentage = ?,
            grade = ?,
            status = ?,
            admission_date = ?,
            updated_at = ?
        WHERE id = ?;
    )";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        error_msg = "Failed to prepare student update statement.";
        return false;
    }

    sqlite3_bind_text(stmt, 1, student.student_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, student.full_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, student.email.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, student.phone.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 5, student.course_id);
    sqlite3_bind_int(stmt, 6, student.semester);
    sqlite3_bind_double(stmt, 7, student.marks_obtained);
    sqlite3_bind_double(stmt, 8, student.maximum_marks);
    sqlite3_bind_double(stmt, 9, student.percentage);
    sqlite3_bind_text(stmt, 10, student.grade.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 11, student.status.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 12, student.admission_date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 13, student.updated_at.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 14, id);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        error_msg = "Database update error.";
        return false;
    }

    if (sqlite3_changes(db_) == 0) {
        error_msg = "Student not found.";
        return false;
    }

    student.id = id;
    return true;
}

bool Database::delete_student(int id, std::string& error_msg) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    const char* sql = "DELETE FROM students WHERE id = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        error_msg = "Failed to prepare student delete statement.";
        return false;
    }

    sqlite3_bind_int(stmt, 1, id);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        error_msg = "Failed to delete student.";
        return false;
    }

    if (sqlite3_changes(db_) == 0) {
        error_msg = "Student not found.";
        return false;
    }
    return true;
}

// ----------------- DASHBOARD & ANALYTICS -----------------

nlohmann::json Database::get_dashboard_stats() {
    std::lock_guard<std::mutex> lock(db_mutex_);
    nlohmann::json stats = {
        {"total_students", 0},
        {"total_courses", 0},
        {"average_marks", 0.0},
        {"average_percentage", 0.0},
        {"passing_count", 0},
        {"failing_count", 0},
        {"grade_distribution", {
            {"A+", 0}, {"A", 0}, {"B", 0}, {"C", 0}, {"D", 0}, {"F", 0}
        }},
        {"recent_students", nlohmann::json::array()}
    };

    // Course count
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, "SELECT COUNT(*) FROM courses;", -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            stats["total_courses"] = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }

    // Student aggregate stats
    const char* agg_sql = R"(
        SELECT
            COUNT(*),
            COALESCE(AVG(marks_obtained), 0.0),
            COALESCE(AVG(percentage), 0.0),
            COALESCE(SUM(CASE WHEN status = 'Pass' THEN 1 ELSE 0 END), 0),
            COALESCE(SUM(CASE WHEN status = 'Fail' THEN 1 ELSE 0 END), 0)
        FROM students;
    )";
    stmt = nullptr;
    if (sqlite3_prepare_v2(db_, agg_sql, -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            stats["total_students"] = sqlite3_column_int(stmt, 0);
            double avg_marks = sqlite3_column_double(stmt, 1);
            double avg_pct = sqlite3_column_double(stmt, 2);
            stats["average_marks"] = std::round(avg_marks * 100.0) / 100.0;
            stats["average_percentage"] = std::round(avg_pct * 100.0) / 100.0;
            stats["passing_count"] = sqlite3_column_int(stmt, 3);
            stats["failing_count"] = sqlite3_column_int(stmt, 4);
        }
        sqlite3_finalize(stmt);
    }

    // Grade distribution
    const char* grade_sql = "SELECT grade, COUNT(*) FROM students GROUP BY grade;";
    stmt = nullptr;
    if (sqlite3_prepare_v2(db_, grade_sql, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            std::string gr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            int cnt = sqlite3_column_int(stmt, 1);
            stats["grade_distribution"][gr] = cnt;
        }
        sqlite3_finalize(stmt);
    }

    // Recent 5 students
    const char* rec_sql = R"(
        SELECT s.id, s.student_id, s.full_name, c.course_name, s.percentage, s.grade, s.status, s.admission_date
        FROM students s
        LEFT JOIN courses c ON s.course_id = c.id
        ORDER BY s.id DESC
        LIMIT 5;
    )";
    stmt = nullptr;
    if (sqlite3_prepare_v2(db_, rec_sql, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* cname = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            nlohmann::json st_item = {
                {"id", sqlite3_column_int(stmt, 0)},
                {"student_id", reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1))},
                {"full_name", reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2))},
                {"course_name", cname ? cname : "Unassigned"},
                {"percentage", sqlite3_column_double(stmt, 4)},
                {"grade", reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5))},
                {"status", reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6))},
                {"admission_date", reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7))}
            };
            stats["recent_students"].push_back(st_item);
        }
        sqlite3_finalize(stmt);
    }

    return stats;
}

nlohmann::json Database::get_results(int course_id, const std::string& grade, const std::string& status, const std::string& search) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    std::string where = " WHERE 1=1";
    std::vector<std::pair<int, std::string>> text_params;
    std::vector<std::pair<int, int>> int_params;
    int p_idx = 1;

    if (!search.empty()) {
        where += " AND (s.student_id LIKE ? OR s.full_name LIKE ? OR s.email LIKE ?)";
        std::string pattern = "%" + search + "%";
        text_params.emplace_back(p_idx++, pattern);
        text_params.emplace_back(p_idx++, pattern);
        text_params.emplace_back(p_idx++, pattern);
    }
    if (course_id > 0) {
        where += " AND s.course_id = ?";
        int_params.emplace_back(p_idx++, course_id);
    }
    if (!grade.empty()) {
        where += " AND s.grade = ?";
        text_params.emplace_back(p_idx++, grade);
    }
    if (!status.empty()) {
        where += " AND s.status = ?";
        text_params.emplace_back(p_idx++, status);
    }

    std::string sql = R"(
        SELECT s.id, s.student_id, s.full_name, c.course_name, s.semester,
               s.marks_obtained, s.maximum_marks, s.percentage, s.grade, s.status
        FROM students s
        LEFT JOIN courses c ON s.course_id = c.id
    )" + where + " ORDER BY s.percentage DESC;";

    nlohmann::json result = {
        {"records", nlohmann::json::array()},
        {"summary", {
            {"total", 0},
            {"passed", 0},
            {"failed", 0},
            {"pass_rate", 0.0},
            {"average_percentage", 0.0}
        }}
    };

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        for (const auto& tp : text_params) {
            sqlite3_bind_text(stmt, tp.first, tp.second.c_str(), -1, SQLITE_TRANSIENT);
        }
        for (const auto& ip : int_params) {
            sqlite3_bind_int(stmt, ip.first, ip.second);
        }

        int total = 0;
        int passed = 0;
        int failed = 0;
        double sum_pct = 0.0;

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            total++;
            const char* c_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            std::string st = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
            double pct = sqlite3_column_double(stmt, 7);
            sum_pct += pct;
            if (st == "Pass") passed++; else failed++;

            nlohmann::json r = {
                {"id", sqlite3_column_int(stmt, 0)},
                {"student_id", reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1))},
                {"full_name", reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2))},
                {"course_name", c_name ? c_name : "Unassigned"},
                {"semester", sqlite3_column_int(stmt, 4)},
                {"marks_obtained", sqlite3_column_double(stmt, 5)},
                {"maximum_marks", sqlite3_column_double(stmt, 6)},
                {"percentage", pct},
                {"grade", reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8))},
                {"status", st}
            };
            result["records"].push_back(r);
        }
        sqlite3_finalize(stmt);

        result["summary"]["total"] = total;
        result["summary"]["passed"] = passed;
        result["summary"]["failed"] = failed;
        if (total > 0) {
            double pass_rate = (static_cast<double>(passed) / total) * 100.0;
            double avg_pct = sum_pct / total;
            result["summary"]["pass_rate"] = std::round(pass_rate * 100.0) / 100.0;
            result["summary"]["average_percentage"] = std::round(avg_pct * 100.0) / 100.0;
        }
    }

    return result;
}
